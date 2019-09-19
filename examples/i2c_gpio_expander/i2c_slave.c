#include <ch554.h>
#include <debug.h>
#include <stdint.h>
#include <stdbool.h>

#include "i2c_slave.h"
#include "gpio.h"

#define SCL_PIN         0
#define SCL_PORT        1
SBIT(SCL, PORT_A_REG, SCL_PIN);

#define SDA_PIN         1
#define SDA_PORT        1
SBIT(SDA, PORT_A_REG, SDA_PIN);

#define REG_COUNT   8
static uint8_t regs[REG_COUNT];

static uint8_t reg;     // Register that the state machine is currently pointing to

inline void wait_for_start() {
    __bit SCL_last;
    __bit SDA_last;

begin_wait:
    // wait for SCL and SDA both high
    while((SCL == 0) || (SDA == 0)) {}

    // wait for transition to SCL high and SDA low
    do {
        SCL_last = SCL;
        SDA_last = SDA;
    } while((SCL_last == 1) && (SDA_last == 1));
    if(SCL_last == 0)
        goto begin_wait;

    // wait for transition to SCL low and SDA low
    do {
        SCL_last = SCL;
        SDA_last = SDA;
    } while((SCL_last == 1) && (SDA_last == 0));
    if(SDA_last == 1)
        goto begin_wait;

}

// Read a bit, but check for a read_address
#define read_bit_2(dest)        \
        while (SCL == 0) {}     \
        if(SDA == 1) {          \
            dest |= SDA;        \
            while(SCL == 1) {   \
                if(SDA == 0) {    \
                    while(SCL == 1) {} \
                    goto read_address;   \
                }               \
            }                   \
        } else {                \
            dest |= SDA;        \
            while(SCL == 1) {}  \
        }                       \

// Read a bit on the next rising edge
#define read_bit(dest)          \
        dest = (dest << 1);     \
        while (SCL == 0) {}     \
        dest |= SDA;            \
        while(SCL == 1) {}

#define write_bit(src)          \
        SDA = (src & 0x80);     \
        while(SCL == 0) {}      \
        src = src << 1;         \
        while(SCL == 1) {}

// Pull data line low, wait for a clock rise and fall, then release
#define send_ack()              \
        SDA = 0;                \
        while(SCL == 0) {}      \
        while (SCL == 1) {}     \
        SDA = 1;

#define send_ack_and_load()     \
        SDA = 0;                \
        while(SCL == 0) {}      \
        val = regs[reg];        \
        while (SCL == 1) {}     \
        SDA = 1;

#define send_ack_and_store()    \
        SDA = 0;                \
        while(SCL == 0) {}      \
        regs[reg] = val;        \
        while (SCL == 1) {}     \
        SDA = 1;

// release data line, wait for a clock rise and fall, then release
#define send_nack()             \
        SDA = 1;                \
        while(SCL == 0) {}      \
        while (SCL == 1) {}     \
        SDA = 1;

void i2c_slave_init() {
    uint8_t i;

    reg = 0;
    for(i = 0; i < REG_COUNT; i++)
        regs[i] = i;

    gpio_pin_mode(SCL_PIN, SCL_PORT, GPIO_MODE_INPUT);
    gpio_pin_mode(SDA_PIN, SDA_PORT, GPIO_MODE_OPEN_DRAIN);
}

void i2c_slave_poll() {
    uint8_t address = 0;    // Address that 
    uint8_t new_reg = 0;    // 
    uint8_t val = 0;

    // TODO: Timeout during transaction

    wait_for_start();
read_address:
    read_bit(address);  // 7 address bits
    read_bit(address);
    read_bit(address);
    read_bit(address);
    read_bit(address);
    read_bit(address);
    read_bit(address);
    read_bit(address);  // Read/Write bit

    if(address == I2C_ADDRESS_WRITE) {
        send_ack();

        read_bit(new_reg);
        read_bit(new_reg);
        read_bit(new_reg);
        read_bit(new_reg);
        read_bit(new_reg);
        read_bit(new_reg);
        read_bit(new_reg);
        read_bit(new_reg);

        if(new_reg >= REG_COUNT) {
            send_nack();
            return;
        }

        reg = new_reg;  // TODO: Is this too slow?
        send_ack();

        // Look for a repeated start here
        read_bit_2(val);
        read_bit(val);
        read_bit(val);
        read_bit(val);
        read_bit(val);
        read_bit(val);
        read_bit(val);
        read_bit(val);

        send_ack_and_store();
    }
    else if(address == I2C_ADDRESS_READ) {
        send_ack_and_load();

        write_bit(val);
        write_bit(val);
        write_bit(val);
        write_bit(val);
        write_bit(val);
        write_bit(val);
        write_bit(val);
        write_bit(val);

        send_ack();
    }
    else {
        return;
    }
}