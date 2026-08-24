/* File: gpio.c
 * ------------
 *  Library functions for changing configuration functions and reading and writing
 *  data to the GPIO pins on the MangoPi
 *
 *  Author: Harrison Chen
 *  Version: 1/22/26
 */
#include "gpio.h"
#include <stddef.h>

enum { GROUP_B = 0, GROUP_C, GROUP_D, GROUP_E, GROUP_F, GROUP_G };

typedef struct  {
    unsigned int group;
    unsigned int pin_index;
} gpio_pin_t;

// The gpio_id_t enumeration assigns a symbolic constant for each
// in such a way to use a single hex constant. The more significant
// hex digit identifies the group and lower 2 hex digits are pin index:
//       constant 0xNnn  N = which group,  nn = pin index within group
//
// This helper function extracts the group and pin index from a gpio_id_t
// e.g. GPIO_PB4 belongs to GROUP_B and has pin_index 4
static gpio_pin_t get_group_and_index(gpio_id_t gpio) {
    gpio_pin_t gp;
    gp.group = gpio >> 8;
    gp.pin_index = gpio & 0xff; // lower 2 hex digits
    return gp;
}

// The gpio groups are differently sized, e.g. B has 13 pins, C only 8.
// This helper function confirms that a gpio_id_t is valid (group
// and pin index are valid)
bool gpio_id_is_valid(gpio_id_t pin) {
    gpio_pin_t gp = get_group_and_index(pin);
    switch (gp.group) {
        case GROUP_B: return (gp.pin_index <= GPIO_PB_LAST_INDEX);
        case GROUP_C: return (gp.pin_index <= GPIO_PC_LAST_INDEX);
        case GROUP_D: return (gp.pin_index <= GPIO_PD_LAST_INDEX);
        case GROUP_E: return (gp.pin_index <= GPIO_PE_LAST_INDEX);
        case GROUP_F: return (gp.pin_index <= GPIO_PF_LAST_INDEX);
        case GROUP_G: return (gp.pin_index <= GPIO_PG_LAST_INDEX);
        default:      return false;
    }
}

// This helper function is suggested to return the address of
// the config0 register for a gpio group, i.e. get_cfg0_reg(GROUP_B)
// Refer to the D1-H user manual to learn the address the config0 register
// for each group. Be sure to note how the address of the config1 and
// config2 register can be computed as relative offset from config0.
// (okay to discard this function if it doesn't fit with your design)
static volatile unsigned int *get_cfg0_reg(unsigned int group) {
    const unsigned int PB_CFG0 = 0x02000030;
    const unsigned int OFFSET = 0x0030;
    return (unsigned int *) ((unsigned long) (PB_CFG0 + group * OFFSET));
}

// This helper function is suggested to return the address of
// the data register for a gpio group. Refer to the D1-H user manual
// to learn the address of the data register for each group.
// (okay to discard this function if it doesn't fit with your design)
static volatile unsigned int *get_data_reg(unsigned int group) {
    const unsigned int PB_DATA = 0x02000040;
    const unsigned int OFFSET = 0x0030;
    return (unsigned int *) ((unsigned long) (PB_DATA + group * OFFSET));
}

void gpio_init(void) {
    // no initialization required for this peripheral
}

// sets the config function of the given gpio pin (by id) to input (0x0000)
// does nothing if given pin is invalid
void gpio_set_input(gpio_id_t pin) {
    gpio_set_function(pin, GPIO_FN_INPUT);
}

// sets the config function of the given gpio pin (by id) to output (0x0001)
// does nothing if given pin is invalid
void gpio_set_output(gpio_id_t pin) {
    gpio_set_function(pin, GPIO_FN_OUTPUT);
}

// sets the config function of the given gpio pin (by id) to the given function 
// does nothing if given pin or function is invalid
void gpio_set_function(gpio_id_t pin, unsigned int function) {
    const int LAST_FN = 0xf;
    const int FN_SIZE = 4;
    const int PINS_PER_CONFIG = 8;
    const int BITMASK = 0xf;

    if (gpio_id_is_valid(pin) && function <= LAST_FN) {
        gpio_pin_t pin_struct = get_group_and_index(pin);
        volatile unsigned int *cfg0_reg = get_cfg0_reg(pin_struct.group) + pin_struct.pin_index / PINS_PER_CONFIG;
        function = function << FN_SIZE * (pin_struct.pin_index % PINS_PER_CONFIG);
        *cfg0_reg = (*cfg0_reg & ~(BITMASK << FN_SIZE * (pin_struct.pin_index % PINS_PER_CONFIG))) | function;
    }
}

// reads and returns the config function of the given gpio pin (by id)
// returns GPIO_INVALID_REQUEST if given pin is invalid
unsigned int gpio_get_function(gpio_id_t pin) {
    const int FN_SIZE = 4;
    const int PINS_PER_CONFIG = 8;
    const int BITMASK = 0xf;

    if (!gpio_id_is_valid(pin)) {
        return GPIO_INVALID_REQUEST;
    }
    gpio_pin_t pin_struct = get_group_and_index(pin);
    volatile unsigned int *cfg0_reg = get_cfg0_reg(pin_struct.group) + pin_struct.pin_index / PINS_PER_CONFIG;
    return (*cfg0_reg >> FN_SIZE * (pin_struct.pin_index % PINS_PER_CONFIG)) & BITMASK;
}

// writes a value to the data of the given gpio pin (by id) 
// does nothing if value or pin is invalid
void gpio_write(gpio_id_t pin, int value) {
    if (gpio_id_is_valid(pin) && (value == 0 || value == 1)) {
        const int BITMASK = 0x1;

        gpio_pin_t pin_struct = get_group_and_index(pin);        
        volatile unsigned int *data_reg = get_data_reg(pin_struct.group);
        *data_reg = (*data_reg & ~(BITMASK << pin_struct.pin_index)) | (value << pin_struct.pin_index);
    }
}

// reads and returns the value at the data of the given gpio pin (by id)
// returns GPIO_INVALID_REQUEST if given pin is invalid
int gpio_read(gpio_id_t pin) {
    if (!gpio_id_is_valid(pin)) {
        return GPIO_INVALID_REQUEST;
    }
    const int BITMASK = 0x1;

    gpio_pin_t pin_struct = get_group_and_index(pin);        
    volatile unsigned int *data_reg = get_data_reg(pin_struct.group);
    return (*data_reg >> pin_struct.pin_index) & BITMASK;
}
// sets input pin to use an internal pull up resistor
// does nothing if pin not valid or not configured to input
void gpio_set_pullup(gpio_id_t pin) {
    if (gpio_id_is_valid(pin)) {
        const int PB_PULL0 = 0x02000054;
        const int OFFSET = 0x0030;
        const int CFG_SIZE = 2;
        const int CFG_PER_REG = 16;
        int pullup = 0b01;
        const int BITMASK = 0b11;

        gpio_pin_t pin_struct = get_group_and_index(pin);
        volatile unsigned int *pull_reg = ((unsigned int *) (unsigned long) (PB_PULL0
                + pin_struct.group * OFFSET)) + pin_struct.pin_index / CFG_PER_REG;
        pullup = pullup << CFG_SIZE * (pin_struct.pin_index % CFG_PER_REG);
        *pull_reg = (*pull_reg & ~(BITMASK << CFG_SIZE * (pin_struct.pin_index % CFG_PER_REG))) | pullup;
    }
}

