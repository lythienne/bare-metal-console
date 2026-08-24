/* File: ps2_assign5.c
 * -------------------
 * Low level methods for interacting with the ps2 control on the MangoPi
 *
 * Author: Harrison Chen
 * Version: 2/17/26
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "malloc.h"
#include "timer.h"
#include "ps2.h"
#include "printf.h"

// A ps2_device is a structure that stores all of the state and information
// needed for a PS2 device. The clock field stores the gpio id for the
// clock pin, and the data field stores the gpio id for the data pin.
// Read ps2_new for example code that sets and uses these fields.
//
// You may extend the ps2_device structure with additional fields as needed.
// A pointer to the current ps2_device is passed into all ps2_ calls.
// Storing state in this structure is preferable to using global variables:
// it allows your driver to support multiple PS2 devices accessed concurrently
// (e.g., a keyboard and a mouse).
//
// This definition fills out the structure declared in ps2.h.
struct ps2_device {
    gpio_id_t clock;
    gpio_id_t data;
};

// Creates a new PS2 device connected to given clock and data pins,
// The gpios are configured as input and set to use internal pull-up
// (PS/2 protocol requires clock/data to be high default)
ps2_device_t *ps2_new(gpio_id_t clock_gpio, gpio_id_t data_gpio) {
    // consider why must malloc be used to allocate device
    ps2_device_t *dev = malloc(sizeof(*dev));

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);
    return dev;
}

void ps2_free(ps2_device_t *dev) {
    free(dev);
}

#define DELAY -1
#define TOO_LONG (500 * 24)

int readBit(ps2_device_t *dev, unsigned int *prev) {
    while (!gpio_read(dev->clock)) {}   //wait until high
    while (gpio_read(dev->clock)) {}    //wait until falling edge
    unsigned int ticks = timer_get_ticks();
    
    int bit = (!*prev || (ticks - *prev) <= TOO_LONG)? gpio_read(dev->data) & 1 : DELAY;
    *prev = ticks;
    return bit;
}

// Read a single PS2 scancode. Always returns a correctly received scancode:
// if an error occurs (e.g., start bit not detected, parity is wrong), the
// function should read another scancode.
uint8_t ps2_read(ps2_device_t *dev) {
    unsigned int prev = 0;
    int bit = readBit(dev, &prev);

read:
    prev = 0;
    if (bit == 1 || bit == DELAY) { //start bit must be low
        bit = readBit(dev, &prev);
        goto read;
    }; 

    bit = readBit(dev, &prev);
    if (bit == DELAY) {
        goto read;
    }
    int odd = 0;
    int readCode = 0;
    for (int i = 0; i < 8; i++) {   //code bits
        readCode += bit << i;
        odd ^= bit;
        bit = readBit(dev, &prev);
        if (bit == DELAY) {
            goto read; 
        }
    }

    if (!(odd ^ bit)) {             //parity bit
        goto read;
    }

    bit = readBit(dev, &prev);
    if (bit == 0 || bit == DELAY) { //stop bit
        bit = readBit(dev, &prev);
        goto read;
    }
    return readCode;
}

