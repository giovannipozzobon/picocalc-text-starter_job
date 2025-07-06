//
//  PicoCalc SD Card driver for FAT32 formatted SD cards
//
//  This driver provides block-level access to SD cards and implements
//  basic FAT32 file system operations for reading and writing files.
//
//  Only Master Boot Record (MBR) disk layout is supported (not GPT).
//  FAT32 without the MBR partition table is supported.
//  Standard SD cards (SDSC) and SD High Capacity (SDHC) cards are supported.
//


#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h> // For strcasecmp

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/sem.h"

#include "sdcard.h"

#define DIR_ENTRY_SIZE (32)         // Size of a directory entry in bytes
#define DIR_ENTRY_FREE (0xE5)       // Free entry marker
#define DIR_ENTRY_END_MARKER (0x00) // End of directory entry marker
#define DIR_LFN_PART_SIZE (13)      // Size of each LFN part in bytes

// Global state
static bool sd_initialised = false;
static bool sd_mounted = false;
static bool is_sdhc = false;            // Set this in sd_card_init()
static sd_error_t mount_status = SD_OK; // Error code for mount operation

// FAT32 file system state
static fat32_boot_sector_t boot_sector;
static uint32_t volume_start_block = 0; // First block of the volume
static uint32_t first_data_sector;      // First sector of the data region
static uint32_t data_region_sectors;    // Total sectors in the data region
static uint32_t cluster_count;          // Total number of clusters in the data region

static uint32_t bytes_per_cluster;

// Working buffers
static uint8_t sector_buffer[SECTOR_SIZE] __attribute__((aligned(4)));

// Function prototypes
static sd_error_t sd_card_init(void);

static repeating_timer_t sd_card_detect_timer;

//
// Low-level SD card SPI functions
//

static inline void sd_cs_select(void)
{
    gpio_put(SD_CS, 0);
}

static inline void sd_cs_deselect(void)
{
    gpio_put(SD_CS, 1);
}

static uint8_t sd_spi_write_read(uint8_t data)
{
    uint8_t result;
    spi_write_read_blocking(SD_SPI, &data, &result, 1);
    return result;
}

static void sd_spi_write_buf(const uint8_t *src, size_t len)
{
    spi_write_blocking(SD_SPI, src, len);
}

static void sd_spi_read_buf(uint8_t *dst, size_t len)
{
    // Send dummy bytes while reading
    memset(dst, 0xFF, len);
    spi_write_read_blocking(SD_SPI, dst, dst, len);
}

static bool sd_wait_ready(void)
{
    uint8_t response;
    uint32_t timeout = 10000; // Add timeout to prevent infinite loop
    do
    {
        response = sd_spi_write_read(0xFF);
        timeout--;
        if (timeout == 0)
        {
            return false; // Timeout occurred
        }
    } while (response != 0xFF);
    return true; // Success
}

static uint8_t sd_send_command(uint8_t cmd, uint32_t arg)
{
    uint8_t response;
    uint8_t retry = 0;

    // For CMD0, don't wait for ready state since card might not be ready yet
    if (cmd != CMD0)
    {
        if (!sd_wait_ready())
        {
            return 0xFF; // Timeout waiting for ready
        }
    }

    // Prepare command packet
    uint8_t packet[6];
    packet[0] = 0x40 | cmd;
    packet[1] = (arg >> 24) & 0xFF;
    packet[2] = (arg >> 16) & 0xFF;
    packet[3] = (arg >> 8) & 0xFF;
    packet[4] = arg & 0xFF;

    // Calculate CRC for specific commands
    uint8_t crc = 0xFF;
    if (cmd == CMD0)
    {
        crc = 0x95;
    }
    if (cmd == CMD8)
    {
        crc = 0x87;
    }
    packet[5] = crc;

    // Send command
    sd_cs_select();
    sd_spi_write_buf(packet, 6);

    // Wait for response (R1) - but with timeout
    response = 0xFF;
    do
    {
        response = sd_spi_write_read(0xFF);
        retry++;
    } while ((response & 0x80) && (retry < 64)); // Increased timeout from 10 to 64

    // Don't deselect here - let caller handle it
    return response;
}

//
// Card detection and initialisation
//

bool sd_card_present(void)
{
    return !gpio_get(SD_DETECT); // Active low
}

bool sd_is_ready(void)
{
    return sd_initialised && sd_mounted && !gpio_get(SD_DETECT);
}

//
// Block-level read/write operations
//

sd_error_t sd_read_block(uint32_t block, uint8_t *buffer)
{
    int32_t addr = is_sdhc ? block : block * SECTOR_SIZE;
    uint8_t response = sd_send_command(CMD17, addr);
    if (response != 0)
    {
        sd_cs_deselect();
        return SD_ERROR_READ_FAILED;
    }

    // Wait for data token
    uint32_t timeout = 100000;
    do
    {
        response = sd_spi_write_read(0xFF);
        timeout--;
    } while (response != DATA_START_BLOCK && timeout > 0);

    if (timeout == 0)
    {
        sd_cs_deselect();
        return SD_ERROR_READ_FAILED;
    }

    // Read data
    sd_spi_read_buf(buffer, SECTOR_SIZE);

    // Read CRC (ignore it)
    sd_spi_write_read(0xFF);
    sd_spi_write_read(0xFF);

    sd_cs_deselect();
    return SD_OK;
}

sd_error_t sd_write_block(uint32_t block, const uint8_t *buffer)
{
    uint32_t addr = is_sdhc ? block : block * SECTOR_SIZE;
    uint8_t response = sd_send_command(CMD24, addr);
    if (response != 0)
    {
        sd_cs_deselect();
        return SD_ERROR_WRITE_FAILED;
    }

    // Send data token
    sd_spi_write_read(DATA_START_BLOCK);

    // Send data
    sd_spi_write_buf(buffer, SECTOR_SIZE);

    // Send dummy CRC
    sd_spi_write_read(0xFF);
    sd_spi_write_read(0xFF);

    // Check data response
    response = sd_spi_write_read(0xFF) & 0x1F;
    sd_cs_deselect();

    if (response != 0x05)
    {
        return SD_ERROR_WRITE_FAILED;
    }

    // Wait for programming to finish
    sd_cs_select();
    sd_wait_ready();
    sd_cs_deselect();

    return SD_OK;
}

sd_error_t sd_read_blocks(uint32_t start_block, uint32_t num_blocks, uint8_t *buffer)
{
    for (uint32_t i = 0; i < num_blocks; i++)
    {
        sd_error_t result = sd_read_block(start_block + i, buffer + (i * SECTOR_SIZE));
        if (result != SD_OK)
        {
            return result;
        }
    }
    return SD_OK;
}

sd_error_t sd_write_blocks(uint32_t start_block, uint32_t num_blocks, const uint8_t *buffer)
{
    for (uint32_t i = 0; i < num_blocks; i++)
    {
        sd_error_t result = sd_write_block(start_block + i, buffer + (i * SECTOR_SIZE));
        if (result != SD_OK)
        {
            return result;
        }
    }
    return SD_OK;
}

//
//  Sector-level access functions
//

static uint32_t cluster_to_sector(uint32_t cluster)
{
    return ((cluster - 2) * boot_sector.sectors_per_cluster) + first_data_sector;
}

static sd_error_t sd_read_sector(uint32_t sector, uint8_t *buffer)
{
    return sd_read_block(volume_start_block + sector, buffer);
}

static sd_error_t sd_write_sector(uint32_t sector, const uint8_t *buffer)
{
    return sd_write_block(volume_start_block + sector, buffer);
}

//
// FAT32 file system functions
//

static bool sd_is_mbr(const uint8_t *sector)
{
    // Check for 0x55AA signature
    if (sector[510] != 0x55 || sector[511] != 0xAA)
    {
        return false;
    }

    // Check for valid partition type in partition table
    for (int i = 0; i < 4; i++)
    {
        uint8_t part_type = sector[446 + i * 16 + 4];
        if (part_type != 0x00)
        {
            return true; // At least one valid partition
        }
    }
    return false;
}

static bool sd_is_fat_boot_sector(const uint8_t *sector)
{
    // Check for 0x55AA signature
    if (sector[510] != 0x55 || sector[511] != 0xAA)
    {
        return false;
    }

    // Check for valid jump instruction
    if (sector[0] != 0xEB && sector[0] != 0xE9)
    {
        return false;
    }

    // Check for reasonable bytes per sector (should be 512, 1024, 2048, or 4096)
    uint16_t bps = sector[11] | (sector[12] << 8);
    if (bps != 512 && bps != 1024 && bps != 2048 && bps != 4096)
    {
        return false;
    }

    return true;
}

static sd_error_t is_valid_fat32_boot_sector(const fat32_boot_sector_t *bs)
{
    // Check bytes per sector - this is critical
    if (bs->bytes_per_sector != SECTOR_SIZE)
    {
        return SD_ERROR_INVALID_FORMAT;
    }

    // Check sectors per cluster (must be power of 2)
    uint8_t spc = bs->sectors_per_cluster;
    if (spc == 0 || spc > 128 || (spc & (spc - 1)) != 0)
    {
        return SD_ERROR_INVALID_FORMAT;
    }

    // Check number of FATs
    if (bs->num_fats == 0 || bs->num_fats > 2)
    {
        return SD_ERROR_INVALID_FORMAT;
    }

    // Check if we have valid reserved sectors
    if (bs->reserved_sectors == 0)
    {
        return SD_ERROR_INVALID_FORMAT;
    }

    // Check if fat size is valid
    if (bs->fat_size_16 != 0 || bs->fat_size_32 == 0)
    {
        return SD_ERROR_INVALID_FORMAT;
    }

    if (bs->total_sectors_32 == 0)
    {
        return SD_ERROR_INVALID_FORMAT;
    }

    return SD_OK;
}

static sd_error_t read_cluster_fat_entry(uint32_t cluster, uint32_t *value)
{
    // The first data cluster is 2, less than 2 is invalid
    if (cluster < 2)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    uint32_t fat_offset = cluster * 4; // 4 bytes per entry in FAT32
    uint32_t fat_sector = boot_sector.reserved_sectors + (fat_offset / SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;

    // Read the FAT sector
    sd_error_t result = sd_read_sector(fat_sector, sector_buffer);
    if (result != SD_OK)
    {
        return result;
    }

    uint32_t entry = *(uint32_t *)(sector_buffer + entry_offset);
    *value = entry & 0x0FFFFFFF; // Mask out upper 4 bits for FAT32
    return SD_OK;
}

static sd_error_t write_cluster_fat_entry(uint32_t cluster, uint32_t value)
{
    // The first data cluster is 2, less than 2 is invalid
    if (cluster < 2)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    uint32_t fat_offset = cluster * 4; // 4 bytes per entry in FAT32
    uint32_t fat_sector = boot_sector.reserved_sectors + (fat_offset / SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % SECTOR_SIZE;

    // Read the FAT sector
    sd_error_t result = sd_read_sector(fat_sector, sector_buffer);
    if (result != SD_OK)
    {
        return result;
    }

    // Write the FAT entry
    *(uint32_t *)(sector_buffer + entry_offset) &= 0xF0000000;
    *(uint32_t *)(sector_buffer + entry_offset) |= value & 0x0FFFFFFF;

    // Write the modified sector back
    result = sd_write_sector(fat_sector, sector_buffer);
    if (result != SD_OK)
    {
        return result; // Error writing back
    }

    return SD_OK;
}

//
// Mount the SD Card functions
//

sd_error_t sd_mount(void)
{
    if (!sd_card_present())
    {
        sd_unmount(); // Unmount if card is not present
        return SD_ERROR_NO_CARD;
    }

    if (sd_mounted)
    {
        return SD_OK;
    }

    sd_error_t result = sd_card_init();
    if (result != SD_OK)
    {
        return result;
    }

    // Read boot sector
    result = sd_read_block(0, sector_buffer);
    if (result != SD_OK)
    {
        return result;
    }

    // Is this a Master Boot Record (MBR)?
    if (sd_is_mbr(sector_buffer))
    {
        volume_start_block = 0; // Set to zero to detect if no partitions are acceptable

        // Read partition table entries
        for (int i = 0; i < 4; i++)
        {
            // Read next partition table entry
            mbr_partition_entry_t *partition_entry = (mbr_partition_entry_t *)(sector_buffer + 446 + i * 16);

            // Check if this partition is active
            if (partition_entry->boot_indicator != 0x00 && partition_entry->boot_indicator != 0x80)
            {
                continue; // No partition here
            }
            if (partition_entry->partition_type == 0x0B || // FAT32 with CHS addressing
                partition_entry->partition_type == 0x0C)   // FAT32 with LBA addressing
            {
                // Align disk accesses with the partition we have decided to use
                volume_start_block = partition_entry->start_lba;

                // Read the boot sector from the partition
                result = sd_read_block(volume_start_block, sector_buffer);
                if (result != SD_OK)
                {
                    return result;
                }
                break;
            }
        }
        if (volume_start_block == 0)
        {
            return SD_ERROR_INVALID_FORMAT; // No valid FAT32 partition found
        }
    }
    else if (sd_is_fat_boot_sector(sector_buffer))
    {
        // No partition table, treat the entire disk as a single partition
        volume_start_block = 0;

        // We already have the boot sector in sector_buffer, no sd_read_block needed
    }
    else
    {
        return SD_ERROR_INVALID_FORMAT; // This is not a valid FAT32 boot sector
    }

    // Copy boot sector data
    memcpy(&boot_sector, sector_buffer, sizeof(fat32_boot_sector_t));

    // Validate boot sector
    result = is_valid_fat32_boot_sector(&boot_sector);
    if (result != SD_OK)
    {
        return result;
    }

    // Calculate important sectors/clusters
    bytes_per_cluster = boot_sector.sectors_per_cluster * SECTOR_SIZE;
    first_data_sector = boot_sector.reserved_sectors + (boot_sector.num_fats * boot_sector.fat_size_32);
    data_region_sectors = boot_sector.total_sectors_32 - (boot_sector.num_fats * boot_sector.fat_size_32);
    cluster_count = data_region_sectors / boot_sector.sectors_per_cluster;
    if (cluster_count < 65525)
    {
        return SD_ERROR_INVALID_FORMAT; // This is FAT12 or FAT16, not FAT32!
    }

    sd_mounted = true;
    return SD_OK;
}

void sd_unmount(void)
{
    sd_mounted = false;
    mount_status = SD_OK; // Reset mount status
    volume_start_block = 0;
}

bool sd_is_mounted(void)
{
    return sd_mounted;
}

bool sd_is_sdhc(void)
{
    return is_sdhc;
}

sd_error_t sd_get_mount_status(void)
{
    return mount_status;
}

sd_error_t sd_get_free_space(uint64_t *free_space)
{
    // We can only get free space for FAT32 using FSInfo
    // Computing free space will be too slow for us

    if (!sd_is_ready())
    {
        return mount_status;
    }

    sd_error_t result = sd_read_sector(boot_sector.fs_info, sector_buffer);
    if (result != SD_OK)
    {
        return result; // Error reading FSInfo sector
    }

    fat32_fsinfo_t *fsinfo = (fat32_fsinfo_t *)sector_buffer;

    // Verify FSInfo signatures
    if (fsinfo->lead_sig == 0x41615252 &&
        fsinfo->struc_sig == 0x61417272 &&
        fsinfo->trail_sig == 0xAA550000 &&
        fsinfo->free_count != 0xFFFFFFFF &&
        fsinfo->free_count <= cluster_count)
    {
        *free_space = ((uint64_t)fsinfo->free_count) * bytes_per_cluster;
        return SD_OK; // Successfully retrieved free space
    }

    // If FSInfo is not valid, we will count free clusters manually
    uint64_t free_clusters = 0;
    for (uint32_t sector = 0; sector < boot_sector.fat_size_32; sector++)
    {
        result = sd_read_sector(boot_sector.reserved_sectors + sector, sector_buffer);
        if (result != SD_OK)
        {
            return result; // Error reading FAT sector
        }
        for (int i = 0; i < SECTOR_SIZE; i += 4)
        {
            uint32_t entry = *(uint32_t *)(sector_buffer + i) & 0x0FFFFFFF;
            if (entry == 0)
            {
                free_clusters++;
            }
        }
    }

    *free_space = free_clusters * bytes_per_cluster;
    return SD_OK;
}

sd_error_t sd_get_total_space(uint64_t *total_space)
{
    if (!sd_is_ready())
    {
        return mount_status;
    }

    // Get the total number of sectors
    uint64_t total_sectors = boot_sector.total_sectors_32;

    // Calculate total space in bytes
    *total_space = total_sectors * SECTOR_SIZE;

    return SD_OK;
}

//
// File system naming utility functions
//

static void convert_filename_to_83(const char *filename, char *name83)
{
    memset(name83, ' ', 11);
    name83[11] = '\0';

    const char *dot = strrchr(filename, '.');
    int name_len = dot ? (dot - filename) : strlen(filename);

    // Copy name part (max 8 characters)
    for (int i = 0; i < name_len && i < 8; i++)
    {
        name83[i] = toupper(filename[i]);
    }

    // Copy extension part (max 3 characters)
    if (dot && strlen(dot + 1) > 0)
    {
        for (int i = 0; i < 3 && dot[1 + i]; i++)
        {
            name83[8 + i] = toupper(dot[1 + i]);
        }
    }
}

static void convert_83_to_filename(const char *name83, char *filename)
{
    int pos = 0;

    // Copy name part
    for (int i = 0; i < 8 && name83[i] != ' '; i++)
    {
        filename[pos++] = tolower(name83[i]);
    }

    // Copy extension part
    bool has_ext = false;
    for (int i = 8; i < 11; i++)
    {
        if (name83[i] != ' ')
        {
            if (!has_ext)
            {
                filename[pos++] = '.';
                has_ext = true;
            }
            filename[pos++] = tolower(name83[i]);
        }
    }

    filename[pos] = '\0';
}

static inline char utf16_to_utf8(uint16_t utf16)
{
    // Convert UTF-16 to UTF-8 (simplified - only handles ASCII range)
    return utf16 < 0x80 ? (char)utf16 : '?';
}

static uint8_t lfn_checksum(const char *name83)
{
    // Calculate checksum for 8.3 filename
    uint8_t sum = 0;
    for (uint8_t i = 11; i > 0; i--)
    {
        // NOTE: The operation is an unsigned char rotate right
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + (uint8_t)*name83++;
    }
    return sum;
}

static void lfn_entry_into_buffer(fat_lfn_entry_t *lfn_entry, char *buffer)
{
    // Convert UTF-16 parts to UTF-8
    *(buffer++) = utf16_to_utf8(lfn_entry->name1[0]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name1[1]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name1[2]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name1[3]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name1[4]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name2[0]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name2[1]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name2[2]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name2[3]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name2[4]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name2[5]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name3[0]);
    *(buffer++) = utf16_to_utf8(lfn_entry->name3[1]);
}

static sd_error_t find_directory_entry(sd_dir_entry_t *dir_entry, const char *filename)
{
    sd_dir_t dir;

    sd_error_t result = sd_dir_open(&dir, "/");
    if (result != SD_OK)
    {
        return result;
    }

    do
    {
        result = sd_dir_read(&dir, dir_entry);
        if (result != SD_OK)
        {
            return result;
        }
        if (dir_entry->name[0] && strcmp(dir_entry->name, filename) == 0)
        {
            sd_dir_close(&dir);
            return SD_OK; // Found the entry
        }
    } while (dir_entry->name[0]);

    sd_dir_close(&dir);
    return SD_ERROR_FILE_NOT_FOUND; // No more entries
}

//
// File operations (simplified implementation)
//

sd_error_t sd_file_open(sd_file_t *file, const char *filename)
{
    if (!file || !filename)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    if (!sd_is_ready())
    {
        return mount_status;
    }

    memset(file, 0, sizeof(sd_file_t));

    sd_dir_entry_t entry;
    sd_error_t result = find_directory_entry(&entry, filename);
    if (result != SD_OK)
    {
        return result; // File not found or error
    }

    // Found the file
    file->is_open = true;
    file->start_cluster = entry.start_cluster;
    file->current_cluster = file->start_cluster;
    file->file_size = entry.size;
    file->position = 0;
    file->attributes = entry.attr;

    return SD_OK;
}

sd_error_t sd_file_create(sd_file_t *file, const char *filename)
{
    // This is a placeholder implementation
    // Creating files requires finding free directory entries and clusters
    return SD_ERROR_INVALID_PARAMETER;
}

sd_error_t sd_file_close(sd_file_t *file)
{
    if (file && file->is_open)
    {
        memset(file, 0, sizeof(sd_file_t));
    }

    return SD_OK;
}

sd_error_t sd_file_read(sd_file_t *file, void *buffer, size_t size, size_t *bytes_read)
{
    if (!file || !file->is_open || !buffer)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    if (!sd_is_ready())
    {
        return mount_status;
    }

    if (bytes_read)
    {
        *bytes_read = 0;
    }

    if (file->position >= file->file_size)
    {
        return SD_OK; // EOF
    }

    size_t remaining = file->file_size - file->position;
    if (size > remaining)
    {
        size = remaining;
    }

    size_t total_read = 0;
    uint8_t *dest = (uint8_t *)buffer;

    while (total_read < size)
    {
        uint32_t cluster_offset = file->position % bytes_per_cluster;
        uint32_t sector_in_cluster = cluster_offset / SECTOR_SIZE;
        uint32_t byte_in_sector = cluster_offset % SECTOR_SIZE;

        uint32_t sector = cluster_to_sector(file->current_cluster) + sector_in_cluster;

        sd_error_t result = sd_read_sector(sector, sector_buffer);
        if (result != SD_OK)
        {
            return result;
        }

        size_t bytes_to_copy = SECTOR_SIZE - byte_in_sector;
        if (bytes_to_copy > size - total_read)
        {
            bytes_to_copy = size - total_read;
        }

        memcpy(dest + total_read, sector_buffer + byte_in_sector, bytes_to_copy);
        total_read += bytes_to_copy;
        file->position += bytes_to_copy;

        // Check if we need to move to the next cluster
        if ((file->position % bytes_per_cluster) == 0 && total_read < size)
        {
            uint32_t next_cluster;
            sd_error_t fat_result = read_cluster_fat_entry(file->current_cluster, &next_cluster);
            if (fat_result != SD_OK || next_cluster >= FAT32_ENTRY_EOC)
            {
                // End of cluster chain or error
                break;
            }
            file->current_cluster = next_cluster;
        }
    }

    if (bytes_read)
    {
        *bytes_read = total_read;
    }
    return SD_OK;
}

sd_error_t sd_file_write(sd_file_t *file, const void *buffer, size_t size)
{
    // This is a placeholder implementation
    // Writing files requires cluster allocation and FAT updates
    return SD_ERROR_INVALID_PARAMETER;
}

sd_error_t sd_file_seek(sd_file_t *file, uint32_t position)
{
    if (!file || !file->is_open)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    if (position > file->file_size)
    {
        position = file->file_size;
    }

    file->position = position;

    // Recalculate current cluster (simplified)
    uint32_t cluster_num = position / bytes_per_cluster;
    file->current_cluster = file->start_cluster + cluster_num;

    return SD_OK;
}

uint32_t sd_file_tell(sd_file_t *file)
{
    return file ? file->position : 0;
}

uint32_t sd_file_size(sd_file_t *file)
{
    return file ? file->file_size : 0;
}

bool sd_file_eof(sd_file_t *file)
{
    return file ? (file->position >= file->file_size) : true;
}

sd_error_t sd_file_delete(const char *filename)
{
    // This is a placeholder implementation
    return SD_ERROR_INVALID_PARAMETER;
}

//
// Directory operations (placeholder implementations)
//

sd_error_t sd_dir_open(sd_dir_t *dir, const char *path)
{
    if (!dir || !path)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    // Only support root directory for now
    if (strcmp(path, "/") != 0)
    {
        return SD_ERROR_INVALID_PATH;
    }

    if (!sd_is_ready())
    {
        return mount_status;
    }

    memset(dir, 0, sizeof(sd_dir_t));

    // Initialise directory structure
    dir->is_open = true;
    dir->start_cluster = boot_sector.root_cluster;
    dir->current_cluster = dir->start_cluster;
    dir->position = 0;

    return SD_OK;
}

sd_error_t sd_dir_read(sd_dir_t *dir, sd_dir_entry_t *dir_entry)
{
    char long_filename[MAX_FILENAME_LEN + 1];
    uint8_t expected_checksum = 0;
    uint32_t read_sector = 0xFFFFFFFF; // Invalid sector to start with

    if (!dir || !dir_entry)
    {
        return SD_ERROR_INVALID_PARAMETER;
    }

    if (!sd_is_ready())
    {
        return mount_status;
    }

    if (!dir->is_open)
    {
        return SD_ERROR_READ_FAILED;
    }

    memset(dir_entry, 0, sizeof(sd_dir_entry_t));

    if (dir->last_entry_read)
    {
        // If we have already read the last entry, return end of directory
        return SD_OK;
    }

    long_filename[0] = '\0'; // Reset long filename buffer

    // Search through all directory sectors
    while (!dir->last_entry_read && dir_entry->name[0] == '\0')
    {
        uint32_t cluster_offset = dir->position % bytes_per_cluster;
        uint32_t sector_in_cluster = cluster_offset / SECTOR_SIZE;
        uint32_t byte_in_sector = cluster_offset % SECTOR_SIZE;

        uint32_t sector = cluster_to_sector(dir->current_cluster) + sector_in_cluster;

        if (sector != read_sector)
        {
            sd_error_t result = sd_read_sector(sector, sector_buffer);
            if (result != SD_OK)
            {
                return result;
            }
            read_sector = sector;
        }

        fat_dir_entry_t *entry = (fat_dir_entry_t *)(sector_buffer + dir->position % SECTOR_SIZE);

        if (entry->name[0] == DIR_ENTRY_END_MARKER)
        {
            // End of directory
            dir->last_entry_read = true; // Mark that we reached the end
        }
        else if (entry->attr == ATTR_LONG_NAME)
        {
            // Populate long filename buffer with this entry's name contents
            fat_lfn_entry_t *lfn_entry = (fat_lfn_entry_t *)entry;
            if (lfn_entry->seq & 0x40)
            {
                // This is the last entry for the long filename and the first entry of the sequence
                // We are starting to build a new long filename, clear the long_filename buffer
                memset(long_filename, 0, sizeof(long_filename));
                expected_checksum = lfn_entry->checksum; // Save checksum for later comparison
            }

            if (lfn_entry->checksum == expected_checksum)
            {
                // Copy this entry's part of the long filename into the long_filename buffer
                int offset = ((lfn_entry->seq & 0x3F) - 1) * DIR_LFN_PART_SIZE;
                lfn_entry_into_buffer(lfn_entry, long_filename + offset);
            }
        }
        else if ((entry->attr & 0x0E) == 0 && entry->name[0] != DIR_ENTRY_FREE)
        {
            uint8_t checksum = lfn_checksum(entry->name);
            // Now check to see if this is the entry we are looking for
            if (long_filename[0] != '\0' && expected_checksum == checksum)
            {
                strcpy(dir_entry->name, long_filename);
            }
            else
            {
                convert_83_to_filename(entry->name, dir_entry->name);
            }
            dir_entry->attr = entry->attr;
            dir_entry->start_cluster = (entry->fst_clus_hi << 16) | entry->fst_clus_lo;
            dir_entry->size = entry->file_size;
            dir_entry->date = entry->wrt_date;
            dir_entry->time = entry->wrt_time;
        }

        dir->position += 32; // Move to next entry (32 bytes per entry)

        // Check if we need to move to the next cluster
        if ((dir->position % bytes_per_cluster) == 0)
        {
            uint32_t next_cluster;
            sd_error_t result = read_cluster_fat_entry(dir->current_cluster, &next_cluster);
            if (result != SD_OK)
            {
                return result; // Error reading FAT entry
            }
            if (next_cluster >= FAT32_ENTRY_EOC)
            {
                // End of cluster chain
                dir->last_entry_read = true; // Mark that we reached the end
                return SD_OK; // No more entries to read
            }
            dir->current_cluster = next_cluster;
        }
    }

    return SD_OK; // Successfully read a directory entry
}

sd_error_t sd_dir_close(sd_dir_t *dir)
{
    if (dir && dir->is_open)
    {
        memset(dir, 0, sizeof(sd_dir_t));
    }

    return SD_OK;
}

sd_error_t sd_dir_create(sd_dir_t *dir, const char *dirname)
{
    return SD_ERROR_INVALID_PARAMETER;
}

sd_error_t sd_dir_delete(sd_dir_t *dir, const char *dirname)
{
    return SD_ERROR_INVALID_PARAMETER;
}

//
// Utility functions
//

const char *sd_error_string(sd_error_t error)
{
    switch (error)
    {
    case SD_OK:
        return "Success";
    case SD_ERROR_NO_CARD:
        return "No SD card present";
    case SD_ERROR_INIT_FAILED:
        return "SD card initialization failed";
    case SD_ERROR_INVALID_FORMAT:
        return "Invalid SD card format";
    case SD_ERROR_READ_FAILED:
        return "Read operation failed";
    case SD_ERROR_WRITE_FAILED:
        return "Write operation failed";
    case SD_ERROR_NOT_MOUNTED:
        return "File system not mounted";
    case SD_ERROR_FILE_NOT_FOUND:
        return "File not found";
    case SD_ERROR_INVALID_PATH:
        return "Invalid path";
    case SD_ERROR_DISK_FULL:
        return "Disk full";
    case SD_ERROR_FILE_EXISTS:
        return "File already exists";
    case SD_ERROR_INVALID_PARAMETER:
        return "Invalid parameter";
    default:
        return "Unknown error";
    }
}

static sd_error_t sd_card_init(void)
{
    // Start with lower SPI speed for initialization (400kHz)
    spi_init(SD_SPI, 400000);

    // Ensure CS is high and wait for card to stabilize
    sd_cs_deselect();
    busy_wait_us(10000); // Wait for card to stabilize

    // Send 80+ clock pulses with CS high to put card in SPI mode
    for (int i = 0; i < 80; i++)
    {
        sd_spi_write_read(0xFF);
    }

    busy_wait_us(10000); // Wait for card to stabilize after clock pulses

    // Reset card to SPI mode (CMD0) - try multiple times
    uint8_t response;
    int cmd0_attempts = 0;
    do
    {
        response = sd_send_command(CMD0, 0);
        sd_cs_deselect();
        cmd0_attempts++;
        if (response != R1_IDLE_STATE && cmd0_attempts < 10)
        {
            busy_wait_us(10000); // Wait 10ms before retry
        }
    } while (response != R1_IDLE_STATE && cmd0_attempts < 10);

    if (response != R1_IDLE_STATE)
    {
        return SD_ERROR_INIT_FAILED;
    }

    // for (int i = 0; i < 80; i++) sd_spi_write_read(0xFF);

    // Check interface condition (CMD8)
    response = sd_send_command(CMD8, 0x1AA);
    if (response == R1_IDLE_STATE)
    {
        // Read the rest of R7 response
        uint8_t r7[4];
        sd_spi_read_buf(r7, 4);
        busy_wait_us(1000); // Wait for card to stabilize after CMD8
        sd_cs_deselect();

        // Check if voltage range is acceptable
        if ((r7[2] & 0x0F) != 0x01 || r7[3] != 0xAA)
        {
            return SD_ERROR_INIT_FAILED;
        }
    }
    else
    {
        sd_cs_deselect();
    }

    // for (int i = 0; i < 80; i++) sd_spi_write_read(0xFF);

    // Initialize card with ACMD41
    uint32_t timeout = 1000;
    do
    {
        // Send CMD55 (APP_CMD) followed by ACMD41
        response = sd_send_command(CMD55, 0);
        sd_cs_deselect();

        if (response > 1)
        {
            return SD_ERROR_INIT_FAILED;
        }

        response = sd_send_command(ACMD41, 0x40000000); // HCS bit for SDHC support
        busy_wait_us(1000);                             // Wait for card to stabilize after CMD8
        sd_cs_deselect();

        if (response == 0)
        {
            // Card is initialized
            break;
        }

        busy_wait_us(1000); // Wait before retry
        timeout--;
    } while (timeout > 0);

    if (timeout == 0)
    {
        return SD_ERROR_INIT_FAILED;
    }

    // Check if card is in SDHC mode (CMD58)
    response = sd_send_command(CMD58, 0);
    if (response != 0)
    {
        sd_cs_deselect();
        return SD_ERROR_INIT_FAILED;
    }
    uint8_t ocr[4] = {0};
    sd_spi_read_buf(ocr, 4);
    busy_wait_us(1000); // Wait for card to stabilize after CMD8
    sd_cs_deselect();

    is_sdhc = (ocr[0] & 0x40) != 0; // CCS bit in OCR

    // Set block length to 512 bytes only for SDSC
    if (!is_sdhc)
    {
        response = sd_send_command(CMD16, SECTOR_SIZE);
        busy_wait_us(1000); // Wait for card to stabilize after CMD8
        sd_cs_deselect();
        if (response != 0)
        {
            return SD_ERROR_INIT_FAILED;
        }
    }

    // Switch to higher speed for normal operation
    spi_set_baudrate(SD_SPI, SD_BAUDRATE);

    return SD_OK;
}

// Timer callback to check SD card presence and mount/unmount
bool on_sd_card_detect(repeating_timer_t *rt)
{
    static int delay_count = 2;

    // Check if SD card is present
    if (!sd_card_present() && sd_is_mounted())
    {
        sd_unmount(); // Unmount if card is not present
        delay_count = 2; // Reset retry count
        return true;  // Continue timer
    }

    if (sd_card_present() && delay_count > 0)
    {
        // If card is present but not mounted, wait a bit before trying to mount
        delay_count--;
        return true; // Continue timer
    }

    if (sd_card_present() && !sd_is_mounted() && mount_status == SD_OK && delay_count <= 0)
    {
        delay_count = 2; // Reset retry count after successful mount
        // Try to mount the SD card
        mount_status = sd_mount();
        if (mount_status != SD_OK)
        {
            return true; // Continue timer
        }
    }

    return true;
}

void sd_init(void)
{
    if (sd_initialised)
    {
        return;
    }

    // Initialize GPIO
    gpio_init(SD_MISO);
    gpio_init(SD_CS);
    gpio_init(SD_SCK);
    gpio_init(SD_MOSI);
    gpio_init(SD_DETECT);

    gpio_set_dir(SD_CS, GPIO_OUT);
    gpio_set_dir(SD_DETECT, GPIO_IN);
    gpio_pull_up(SD_DETECT);

    gpio_set_function(SD_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_MOSI, GPIO_FUNC_SPI);

    // Check if a SD card is present
    add_repeating_timer_ms(500, on_sd_card_detect, NULL, &sd_card_detect_timer);

    sd_initialised = true;
}
