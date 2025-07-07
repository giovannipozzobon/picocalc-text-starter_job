#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SD_SPI (spi0)

// Raspberry Pi Pico board GPIO pins
#define SD_MISO (16)   // master in, slave out (MISO)
#define SD_CS (17)     // chip select (CS)
#define SD_SCK (18)    // serial clock (SCK)
#define SD_MOSI (19)   // master out, slave in (MOSI)
#define SD_DETECT (22) // card detect (CD)

// SD card interface definitions
#define SD_BAUDRATE (25000000) // 25 MHz SPI clock speed (SD spec max for SPI mode)

// SD card commands
#define CMD0 (0)    // GO_IDLE_STATE
#define CMD1 (1)    // SEND_OP_COND (MMC)
#define CMD8 (8)    // SEND_IF_COND
#define CMD9 (9)    // SEND_CSD
#define CMD10 (10)  // SEND_CID
#define CMD12 (12)  // STOP_TRANSMISSION
#define CMD16 (16)  // SET_BLOCKLEN
#define CMD17 (17)  // READ_SINGLE_BLOCK
#define CMD18 (18)  // READ_MULTIPLE_BLOCK
#define CMD23 (23)  // SET_BLOCK_COUNT
#define CMD24 (24)  // WRITE_BLOCK
#define CMD25 (25)  // WRITE_MULTIPLE_BLOCK
#define CMD55 (55)  // APP_CMD
#define CMD58 (58)  // READ_OCR
#define ACMD23 (23) // SET_WR_BLK_ERASE_COUNT
#define ACMD41 (41) // SD_SEND_OP_COND

// SD card response types
#define R1_IDLE_STATE (1 << 0)
#define R1_ERASE_RESET (1 << 1)
#define R1_ILLEGAL_COMMAND (1 << 2)
#define R1_COM_CRC_ERROR (1 << 3)
#define R1_ERASE_SEQUENCE_ERROR (1 << 4)
#define R1_ADDRESS_ERROR (1 << 5)
#define R1_PARAMETER_ERROR (1 << 6)

// Data tokens
#define DATA_START_BLOCK (0xFE)
#define DATA_START_BLOCK_MULT (0xFC)
#define DATA_STOP_MULT (0xFD)

// FAT32 constants
#define SECTOR_SIZE (512)
#define MAX_FILENAME_LEN (255)
#define MAX_PATH_LEN (260)

// File attributes
#define ATTR_READ_ONLY (0x01)
#define ATTR_HIDDEN (0x02)
#define ATTR_SYSTEM (0x04)
#define ATTR_VOLUME_ID (0x08)
#define ATTR_DIRECTORY (0x10)
#define ATTR_ARCHIVE (0x20)
#define ATTR_LONG_NAME (0x0F)
#define ATTR_MASK (0x3F) // Mask for valid attributes

// FAT32 Entry constants
#define FAT32_ENTRY_FREE (0x00)      // Free cluster
#define FAT32_ENTRY_EOC (0x0FFFFFF8) // End of cluster chain

// Error codes
typedef enum
{
    SD_OK = 0,
    SD_ERROR_NO_CARD,
    SD_ERROR_INIT_FAILED,
    SD_ERROR_INVALID_FORMAT,
    SD_ERROR_READ_FAILED,
    SD_ERROR_WRITE_FAILED,
    SD_ERROR_NOT_MOUNTED,
    SD_ERROR_FILE_NOT_FOUND,
    SD_ERROR_INVALID_PATH,
    SD_ERROR_NOT_A_DIRECTORY,
    SD_ERROR_NOT_A_FILE,
    SD_ERROR_DIR_NOT_EMPTY,
    SD_ERROR_DIR_NOT_FOUND,
    SD_ERROR_DISK_FULL,
    SD_ERROR_FILE_EXISTS,
    SD_ERROR_INVALID_PARAMETER,
    SD_ERROR_INVALID_SECTOR_SIZE,
    SD_ERROR_INVALID_CLUSTER_SIZE,
    SD_ERROR_INVALID_FATS,
    SD_ERROR_INVALID_RESERVED_SECTORS,
} sd_error_t;

// File handle structure
typedef struct
{
    bool is_open;
    uint32_t start_cluster;
    uint32_t current_cluster;
    uint32_t file_size;
    uint32_t position;
    uint8_t attributes;
} sd_file_t;

typedef struct
{
    bool is_open;
    uint32_t start_cluster;
    uint32_t current_cluster;
    uint32_t position;
    bool last_entry_read;
} sd_dir_t;

// Directory entry structure
typedef struct
{
    char name[256];
    uint32_t size;
    uint16_t date;
    uint16_t time;
    uint32_t start_cluster; // First cluster number (FAT32)
    uint8_t attr;
} sd_dir_entry_t;

// Partition entry structure
typedef struct
{
    uint8_t boot_indicator; // 0x80 for active partition
    uint8_t start_head;     // Starting head number
    uint16_t start_sector;  // Starting sector (cylinder, sector)
    uint8_t partition_type; // Partition type (0x0B for FAT32, 0x0C for FAT32 LBA)
    uint8_t end_head;       // Ending head number
    uint16_t end_sector;    // Ending sector (cylinder, sector)
    uint32_t start_lba;     // Starting LBA (Logical Block Addressing)
    uint32_t size;          // Size of the partition in sectors
} __attribute__((packed)) mbr_partition_entry_t;

// Boot sector structure (simplified)
typedef struct __attribute__((packed))
{
    uint8_t jump[3];             // Jump[0] must be 0xEB or 0xE9
    char oem_name[8];            // OEM name (e.g., "MSWIN4.1") (ignore)
    uint16_t bytes_per_sector;   // Bytes per sector (must be 512)
    uint8_t sectors_per_cluster; // Sectors per cluster (must be power of 2, e.g., 1, 2, 4, 8, 16, 32, 64, 128)
    uint16_t reserved_sectors;   // Number of reserved sectors (must not be 0, usually 32 for FAT32)
    uint8_t num_fats;            // Number of FATs (we require 1 or 2)
    uint16_t root_entries;       // Number of root directory entries (must be 0 for FAT32)
    uint16_t total_sectors_16;   // Total sectors (must be 0 for FAT32, use total_sectors_32)
    uint8_t media_type;          // Media type (ignored)
    uint16_t fat_size_16;        // Size of each FAT in sectors (must be 0 for FAT32)
    uint16_t sectors_per_track;  // Sectors per track (CHS, ignored)
    uint16_t num_heads;          // Number of heads (CHS, ignored)
    uint32_t hidden_sectors;     // Number of hidden sectors (CHS,ignored)
    uint32_t total_sectors_32;   // Total sectors (must be non-zero for FAT32)

    // FAT32 specific
    uint32_t fat_size_32;  // Size of **each** FAT in sectors (must be non-zero)
    uint16_t ext_flags;    // Extended flags (ignored)
    uint16_t fs_version;   // File system version (ignored)
    uint32_t root_cluster; // First cluster of the root directory
    uint16_t fs_info;      // FSInfo sector number (usually 1)
} fat32_boot_sector_t;

// FAT32 FSInfo sector structure
typedef struct
{
    uint32_t lead_sig;      // 0x41615252
    uint8_t reserved1[480]; // Reserved bytes
    uint32_t struc_sig;     // 0x61417272
    uint32_t free_count;    // Number of free clusters (0xFFFFFFFF if unknown)
    uint32_t next_free;     // Next free cluster (0xFFFFFFFF if unknown)
    uint8_t reserved2[12];  // Reserved bytes
    uint32_t trail_sig;     // 0xAA550000
} __attribute__((packed)) fat32_fsinfo_t;

typedef struct
{
    char name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t fst_clus_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t fst_clus_lo;
    uint32_t file_size;
} __attribute__((packed)) fat_dir_entry_t;

typedef struct
{
    uint8_t seq;         // Sequence number
    uint16_t name1[5];   // First 5 characters (UTF-16)
    uint8_t attr;        // Always 0x0F for LFN
    uint8_t type;        // Always 0 for LFN
    uint8_t checksum;    // Checksum of 8.3 name
    uint16_t name2[6];   // Next 6 characters (UTF-16)
    uint16_t first_clus; // Always 0 for LFN
    uint16_t name3[2];   // Last 2 characters (UTF-16)
} __attribute__((packed)) fat_lfn_entry_t;

// Function prototypes

// Low-level SD card functions
bool sd_card_present(void);
void sd_init(void);

// Block-level read/write functions
sd_error_t sd_read_block(uint32_t block, uint8_t *buffer);
sd_error_t sd_write_block(uint32_t block, const uint8_t *buffer);
sd_error_t sd_read_blocks(uint32_t start_block, uint32_t num_blocks, uint8_t *buffer);
sd_error_t sd_write_blocks(uint32_t start_block, uint32_t num_blocks, const uint8_t *buffer);

// File system functions
sd_error_t sd_mount(void);
void sd_unmount(void);
bool sd_is_mounted(void);
bool sd_is_sdhc(void);
sd_error_t sd_get_mount_status(void);
sd_error_t sd_get_free_space(uint64_t *free_space);
sd_error_t sd_get_total_space(uint64_t *total_space);
sd_error_t sd_get_volume_name(char *name, size_t name_len);

// File operations
sd_error_t sd_file_open(sd_file_t *file, const char *filename);
sd_error_t sd_file_create(sd_file_t *file, const char *filename);
sd_error_t sd_file_close(sd_file_t *file);
sd_error_t sd_file_read(sd_file_t *file, void *buffer, size_t size, size_t *bytes_read);
sd_error_t sd_file_write(sd_file_t *file, const void *buffer, size_t size);
sd_error_t sd_file_seek(sd_file_t *file, uint32_t position);
uint32_t sd_file_tell(sd_file_t *file);
uint32_t sd_file_size(sd_file_t *file);
bool sd_file_eof(sd_file_t *file);
sd_error_t sd_file_delete(const char *filename);

// Directory operations
sd_error_t sd_set_current_dir(const char *path);
sd_error_t sd_get_current_dir(char *path, size_t path_len);
sd_error_t sd_dir_open(sd_dir_t *dir, const char *path);
sd_error_t sd_dir_read(sd_dir_t *dir, sd_dir_entry_t *entry);
sd_error_t sd_dir_close(sd_dir_t *dir);
sd_error_t sd_dir_create(sd_dir_t *dir, const char *dirname);
sd_error_t sd_dir_delete(sd_dir_t *dir, const char *dirname);

// Utility functions
const char *sd_error_string(sd_error_t error);
