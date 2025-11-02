#include "ds3231.h"
#include <stdio.h>

// Converte un valore BCD (Binary Coded Decimal) in decimale
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd % 16);
}

// Converte un valore decimale in BCD (Binary Coded Decimal)
uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) * 16) + (dec % 10);
}

bool ds3231_init(void) {
    // Inizializza I2C1 a 100 kHz
    i2c_init(DS3231_I2C_PORT, DS3231_I2C_BAUDRATE);

    // Configura i pin GPIO per I2C
    gpio_set_function(DS3231_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DS3231_SCL_PIN, GPIO_FUNC_I2C);

    // Abilita i pull-up interni sui pin I2C
    gpio_pull_up(DS3231_SDA_PIN);
    gpio_pull_up(DS3231_SCL_PIN);

    // Verifica che il dispositivo DS3231 sia presente sulla bus I2C
    uint8_t test_data;
    int result = i2c_read_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, &test_data, 1, false);

    if (result < 0) {
        printf("Errore: DS3231 non trovato sulla porta I2C1\n");
        return false;
    }

    printf("DS3231 inizializzato con successo\n");
    return true;
}

bool ds3231_read_time(ds3231_datetime_t *datetime) {
    if (datetime == NULL) {
        return false;
    }

    uint8_t buffer[7];
    uint8_t reg = DS3231_REG_SECONDS;

    // Scrivi l'indirizzo del registro da cui iniziare a leggere
    int result = i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, &reg, 1, true);
    if (result < 0) {
        printf("Errore nella scrittura dell'indirizzo del registro DS3231\n");
        return false;
    }

    // Leggi 7 byte (secondi, minuti, ore, giorno, data, mese, anno)
    result = i2c_read_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, buffer, 7, false);
    if (result < 0) {
        printf("Errore nella lettura dei dati dal DS3231\n");
        return false;
    }

    // Converti i valori BCD in decimali e memorizzali nella struttura
    datetime->seconds = bcd_to_dec(buffer[0] & 0x7F);  // Maschera il bit 7
    datetime->minutes = bcd_to_dec(buffer[1] & 0x7F);  // Maschera il bit 7
    datetime->hours   = bcd_to_dec(buffer[2] & 0x3F);  // Formato 24 ore, maschera i bit 6-7
    datetime->day     = bcd_to_dec(buffer[3] & 0x07);  // Maschera i bit 3-7
    datetime->date    = bcd_to_dec(buffer[4] & 0x3F);  // Maschera i bit 6-7
    datetime->month   = bcd_to_dec(buffer[5] & 0x1F);  // Maschera i bit 5-7
    datetime->year    = bcd_to_dec(buffer[6]);         // Anno (0-99)

    return true;
}

bool ds3231_write_time(const ds3231_datetime_t *datetime) {
    if (datetime == NULL) {
        return false;
    }

    // Validazione dei valori
    if (datetime->seconds > 59 || datetime->minutes > 59 || datetime->hours > 23 ||
        datetime->day < 1 || datetime->day > 7 ||
        datetime->date < 1 || datetime->date > 31 ||
        datetime->month < 1 || datetime->month > 12 ||
        datetime->year > 99) {
        printf("Errore: valori di data/ora non validi\n");
        return false;
    }

    // Buffer: [registro iniziale, secondi, minuti, ore, giorno, data, mese, anno]
    uint8_t buffer[8];

    buffer[0] = DS3231_REG_SECONDS;                // Registro iniziale
    buffer[1] = dec_to_bcd(datetime->seconds);     // Secondi in BCD
    buffer[2] = dec_to_bcd(datetime->minutes);     // Minuti in BCD
    buffer[3] = dec_to_bcd(datetime->hours);       // Ore in BCD (formato 24 ore)
    buffer[4] = dec_to_bcd(datetime->day);         // Giorno della settimana in BCD
    buffer[5] = dec_to_bcd(datetime->date);        // Data in BCD
    buffer[6] = dec_to_bcd(datetime->month);       // Mese in BCD
    buffer[7] = dec_to_bcd(datetime->year);        // Anno in BCD

    // Scrivi tutti i registri in una singola transazione
    int result = i2c_write_blocking(DS3231_I2C_PORT, DS3231_I2C_ADDR, buffer, 8, false);

    if (result < 0) {
        printf("Errore nella scrittura dei dati al DS3231\n");
        return false;
    }

    return true;
}
