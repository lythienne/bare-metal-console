/* File: print_pinout.c
 * --------------------
 * Sample test program that uses printf to output an ASCII version of
 * the Mango Pi pinout. (C translation of python $CS107E/bin/pinout.py)
 *
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 */

#include "printf.h"
#include "uart.h"

static const char *board[] = {
    "    |OTG|  |USB|                | HDMI |     ",
    " O--|   |--|   |----------------| mini |---O ",
    " |                  +-------+      +-----+ | ",
    " |     Mango Pi     |  D1   |      |micro| | ",
    " |      MQ-Pro      |  SoC  |      | sd  | | ",
    " |                  +-------+      +-----+ | ",
    " |                                         | ",
    " | @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ 1 | ",
    " | - - - - - - - - - - - - - - - - - - - - | ",
    " | @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ @ | ",
    " O-----------------------------------------O ",
};

const char *pin_labels[20][2] = {
    { "3V3",    "5V"},
    { "PG13",   "5V"},
    { "PG12",   "GND"},
    { "PB7",    "PB8 (TX)"},
    { "GND",    "PB9 (RX)"},
    { "PD21",   "PB5"},
    { "PD22",   "GND"},
    { "PB0",    "PB1"},
    { "3V3",    "PD14"},
    { "PD12",   "GND"},
    { "PD13",   "PC1"},
    { "PD11",   "PD10"},
    { "GND",    "PD15"},
    { "PE17",   "PE16"},
    { "PB10",   "GND"},
    { "PB11",   "PC0"},
    { "PB12",   "GND"},
    { "PB6",    "PB2"},
    { "PD17",   "PB3"},
    { "GND",    "PB4"},
};

static void print_board(void) {
    for (int i = 0; i < sizeof(board)/sizeof(*board); i++) {
        const char *rowstr = board[i];
        printf("%s\n", rowstr);
    }
}

static void print_pin_labels(void) {
    int pin_number = 1;
    for (int i = 0; i < sizeof(pin_labels)/sizeof(*pin_labels); i++) {
        printf("%8s  %2d|%2d  %s\n",
            pin_labels[i][0],
            pin_number,
            pin_number+1,
            pin_labels[i][1]);
        pin_number += 2;
    }
}

void main(void) {
    uart_init();
    printf("\n");
    print_board();
    printf("\n");
    print_pin_labels();
}
