/* File: ps2_assign7.c
 * -------------------
 * Low level methods for interacting with the ps2 control on the MangoPi
 *
 * Author: Harrison Chen
 * Version: 2/26/26
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "malloc.h"
#include "timer.h"
#include "ps2.h"
#include "gpio_interrupt.h"
#include "uart.h"
#include "ringbuffer.h"

#define TOO_LONG (500 * 24)

enum State {START = 0, DATA_START, DATA_ODD, DATA_EVEN, PARITY_ODD, PARITY_EVEN, STOP};

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
    enum State state;
    uint8_t scancode;
    uint8_t data_index;
    unsigned int prev_time;
    rb_t *code_rb;
};

void readBitHandler(void *aux_data) {
    ps2_device_t *dev = (ps2_device_t *) aux_data;
    gpio_interrupt_clear(dev->clock);
    int val = gpio_read(dev->data)& 1; 
    unsigned int ticks = timer_get_ticks();
    unsigned int prev_ticks = dev->prev_time;
    if (prev_ticks == 0 || ticks - prev_ticks >= TOO_LONG) {
        dev->state = START;
        dev->scancode = 0;
        dev->data_index = 0;
    }
    dev->prev_time = ticks;

    switch (dev->state) {
        case START:
            if (!val) {
                dev->state = DATA_START;
            }
            break;
        case DATA_START:
            dev->scancode = val;
            dev->data_index = 1;
            dev->state = val ? DATA_ODD : DATA_EVEN;
            break;
        case DATA_ODD:
            dev->scancode |= val << dev->data_index;
            dev->data_index++;
            if (dev->data_index >= 8) {
                dev->data_index = 0;
                dev->state = val ? PARITY_EVEN : PARITY_ODD;
            }
            else {
                dev->state = val ? DATA_EVEN : DATA_ODD;
            }
            break;
        case DATA_EVEN:
            dev->scancode |= val << dev->data_index;
            dev->data_index++;
            if (dev->data_index >= 8) {
                dev->data_index = 0;
                dev->state = val ? PARITY_ODD : PARITY_EVEN;
            }
            else {
                dev->state = val ? DATA_ODD : DATA_EVEN;
            }
            break;
        case PARITY_ODD:
            dev->state = val ? START : STOP;
            break;
        case PARITY_EVEN:
            dev->state = val ? STOP : DATA_START;
            break;
        case STOP:
            if (val && !rb_full(dev->code_rb)) {
                rb_enqueue(dev->code_rb, dev->scancode);
            }
            dev->scancode = 0;
            dev->state = val ? START : DATA_START;
            break;
    }
}

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

    dev->state = START;
    dev->prev_time = 0;
    dev->scancode = 0;
    dev->data_index = 0;
    dev->code_rb = rb_new();

    gpio_interrupt_init();
    gpio_interrupt_config(clock_gpio, GPIO_INTERRUPT_NEGATIVE_EDGE, false);
    gpio_interrupt_register_handler(clock_gpio, readBitHandler, dev);
    gpio_interrupt_enable(clock_gpio);
    return dev;
}

void ps2_free(ps2_device_t *dev) {
    free(dev);
}

// Read a single PS2 scancode. Always returns a correctly received scancode:
// if an error occurs (e.g., start bit not detected, parity is wrong), the
// function should read another scancode.
uint8_t ps2_read(ps2_device_t *dev) {
    while (rb_empty(dev->code_rb)) {};
    return rb_dequeue(dev->code_rb);
}

