//
// Pico 2 (W) serial driver
//
// This driver implements a simple serial interface for the Clockwork PicoCalc using the
// UART peripheral. It handles character reception and transmission,
// user interrupts (Ctrl+C), and provides functions to check for available keys
// and emit characters. The driver uses a circular buffer to store received characters
// and an interrupt handler to process incoming data.
//

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// #define ENABLE_USER_INTERRUPT

#include "serial.h"

extern volatile bool user_interrupt;

static volatile uint8_t rx_buffer[UART_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

// Interrupt handler for UART RX
static void on_uart_rx()
{
    while (uart_is_readable(UART_PORT))
    {
        uint8_t ch = uart_getc(UART_PORT);
#ifdef ENABLE_USER_INTERRUPT
        // Check for user interrupt (Ctrl+C)
        if (ch == 0x03)                 // Ctrl+C
        {
            user_interrupt = true;      // Set the user interrupt flag
            continue;                   // Skip adding this character to the buffer
        }
#endif
        uint16_t next_head = (rx_head + 1) & (UART_BUFFER_SIZE - 1);
        rx_buffer[rx_head] = ch;
        rx_head = next_head;
    }
}

bool serial_input_available()
{
    return rx_head != rx_tail;
}

char serial_get_char()
{
    while (!serial_input_available()) {
        tight_loop_contents();
    }
        
    uint8_t ch = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & (UART_BUFFER_SIZE - 1);
    return ch;
}

bool serial_output_available()
{
    return uart_is_writable(UART_PORT);
}

void serial_put_char(char ch)
{
    uart_putc(UART_PORT, ch);             // Send the character
}

void serial_init(uint baudrate, uint databits, uint stopbits, uart_parity_t parity)
{
    // Set up our UART
    uart_init(UART_PORT, baudrate);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_RX, GPIO_FUNC_UART);
    
    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_PORT, false, false);

    // Set our data format
    uart_set_format(UART_PORT, databits, stopbits, parity);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_PORT, false);

    // Set up a RX interrupt
    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_PORT, true, false);
}

