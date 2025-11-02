#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// DS3231 I2C Address
#define DS3231_I2C_ADDR 0x68

// DS3231 Register Addresses
#define DS3231_REG_SECONDS      0x00
#define DS3231_REG_MINUTES      0x01
#define DS3231_REG_HOURS        0x02
#define DS3231_REG_DAY          0x03
#define DS3231_REG_DATE         0x04
#define DS3231_REG_MONTH        0x05
#define DS3231_REG_YEAR         0x06
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_STATUS       0x0F
#define DS3231_REG_TEMP_MSB     0x11
#define DS3231_REG_TEMP_LSB     0x12

// I2C Configuration
#define DS3231_I2C_PORT         i2c1
#define DS3231_SDA_PIN          6
#define DS3231_SCL_PIN          7
#define DS3231_I2C_BAUDRATE     100000  // 100 kHz

// Struttura per data e ora
typedef struct {
    uint8_t seconds;    // 0-59
    uint8_t minutes;    // 0-59
    uint8_t hours;      // 0-23 (formato 24 ore)
    uint8_t day;        // 1-7 (giorno della settimana)
    uint8_t date;       // 1-31 (giorno del mese)
    uint8_t month;      // 1-12
    uint8_t year;       // 0-99 (anno dal 2000)
} ds3231_datetime_t;

/**
 * @brief Inizializza il dispositivo DS3231 sulla porta I2C1
 *
 * Configura i pin GP6 (SDA) e GP7 (SCL) per I2C1 e inizializza
 * la comunicazione con il DS3231 a 100 kHz.
 *
 * @return true se l'inizializzazione è riuscita, false altrimenti
 */
bool ds3231_init(void);

/**
 * @brief Legge data e ora dal DS3231
 *
 * Legge tutti i registri del tempo dal DS3231 e li converte
 * dal formato BCD al formato decimale.
 *
 * @param datetime Puntatore alla struttura dove salvare data e ora
 * @return true se la lettura è riuscita, false altrimenti
 */
bool ds3231_read_time(ds3231_datetime_t *datetime);

/**
 * @brief Scrive data e ora nel DS3231
 *
 * Converte i valori dalla struttura dal formato decimale al formato BCD
 * e li scrive nei registri del DS3231.
 *
 * @param datetime Puntatore alla struttura contenente data e ora da impostare
 * @return true se la scrittura è riuscita, false altrimenti
 */
bool ds3231_write_time(const ds3231_datetime_t *datetime);

/**
 * @brief Converte un valore BCD in decimale
 *
 * @param bcd Valore in formato BCD
 * @return Valore in formato decimale
 */
uint8_t bcd_to_dec(uint8_t bcd);

/**
 * @brief Converte un valore decimale in BCD
 *
 * @param dec Valore in formato decimale
 * @return Valore in formato BCD
 */
uint8_t dec_to_bcd(uint8_t dec);

#endif // DS3231_H
