/* File: test_gpio_timer.c
 * -----------------------
 * A series of tests for the gpio and timer libraries on the MangoPi using an assert
 * macro and showing aborts with a flashing blue LED on the MangoPi.
 *
 * Author: Harrison Chen
 * Version: 1/23/26
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "timer.h"

// You call assert on an expression that you expect to be true. If expr
// instead evaluates to false, then assert calls abort, which stops
// your program and flashes onboard led.
#define assert(expr) if(!(expr)) abort()

// infinite loop that flashes onboard blue LED (GPIO PD18)
void abort(void) {
    volatile unsigned int *GPIO_CFG2 = (unsigned int *)0x02000098;
    volatile unsigned int *GPIO_DATA = (unsigned int *)0x020000a0;

    // Configure GPIO PD18 function to be output.
    *GPIO_CFG2 = (*GPIO_CFG2 & ~(0xf00)) | 0x100;
    while (1) { // infinite loop
        *GPIO_DATA ^= (1 << 18); // invert value
        for (volatile int delay = 0x100000; delay > 0; delay--) ; // wait
    }
}

void test_gpio_set_get_function(void) {
    // Check constants and math (for debugging)
    assert(~0xf == 0xfffffff0);
    assert(GPIO_FN_OUTPUT == 0x0001);

    // Test get pin function (pin defaults to disabled)
    assert( gpio_get_function(GPIO_PC0) == GPIO_FN_DISABLED);

    // Set pin to output, confirm get returns what was set, confirm other registers unchanged
    gpio_set_output(GPIO_PC0);
    assert( gpio_get_function(GPIO_PC0) == GPIO_FN_OUTPUT );
    for (int i = 1; i < 7; i++) {
        //assert( gpio_get_function(0x100 + i) == GPIO_FN_DISABLED);
    }

    // Set pin to input, confirm get returns what was set
    gpio_set_input(GPIO_PC0);
    assert( gpio_get_function(GPIO_PC0) == GPIO_FN_INPUT );

    // Set PC3, confirm get returns what was set (check bitmasking)
    gpio_set_function(GPIO_PC3, 0xe);
    assert(gpio_get_function(GPIO_PC3) == 0xe);

    // Set PD17, confirm get returns what was set (check cfg register change)
    gpio_set_function(GPIO_PD17, 0xb);
    assert(gpio_get_function(GPIO_PD17) == 0xb);

    // Check get on invalid register (PC17) returns invalid request
    assert(gpio_get_function(0x111) == GPIO_INVALID_REQUEST);

    // Set PC1 (again), confirm get returns what was set
    gpio_set_function(GPIO_PC0, GPIO_FN_DISABLED);
    assert(gpio_get_function(GPIO_PC0) == GPIO_FN_DISABLED);

    // Check all ids and all functions
    for (int id = 0; id <= GPIO_ID_LAST; id++) {
        if (!gpio_id_is_valid(id)) {
            assert(gpio_get_function(id) == GPIO_INVALID_REQUEST);
        }
        else {
            for (int func = 0; func <= 15; func++) {
                gpio_set_function(id, func);
                assert(gpio_get_function(id) == func);
            }
        }
    }
}
void test_gpio_read_write(void) {
    // set pin to output before gpio_write
    gpio_set_output(GPIO_PB4);

    // gpio_write low, confirm gpio_read reads what was written
    gpio_write(GPIO_PB4, 0);
    assert( gpio_read(GPIO_PB4) ==  0 );

    //gpio_write high, confirm gpio_read reads what was written
    gpio_write(GPIO_PB4, 1);
    assert( gpio_read(GPIO_PB4) ==  1 );

    // gpio_write low, confirm gpio_read reads what was written
    gpio_write(GPIO_PB4, 0);
    assert( gpio_read(GPIO_PB4) ==  0 );

    // write all PC0-7 to 1 and change PC5, check that doesn't affect rest
    for (int id = 0x100; id <= 0x107; id++) {
        gpio_set_output(id);
        gpio_write(id, 1);
    }

    gpio_write(GPIO_PC5, 0);

    for (int id = 0x100; id <= 0x107; id++) {
        if (id == 0x105) {
            assert( gpio_read(id) == 0 );
        }
        else {
            assert( gpio_read(id) == 1);
        }
    }

    // gpio_read invalid returns invalid
    assert( gpio_read(0x111) == GPIO_INVALID_REQUEST);

    // Check all ids and all functions
    for (int id = 0; id <= GPIO_ID_LAST; id++) {
        if (!gpio_id_is_valid(id)) {
            assert(gpio_read(id) == GPIO_INVALID_REQUEST);
        }
        else {
            gpio_set_output(id);
            for (int val = 0; val <= 1; val++) {
                gpio_write(id, val);
                assert(gpio_read(id) == val);
            }
        }
    }

    // Test with LED (write 1 to PB0 and PB1)
    gpio_write(GPIO_PB0, 1);
    gpio_write(GPIO_PB1, 1);
}

void test_timer(void) {
    // Test timer tick count incrementing
    unsigned long start = timer_get_ticks();
    for( int i=0; i<10; i++ ) { /* Spin */ }
    unsigned long finish = timer_get_ticks();
    assert( finish > start );

    // Test timer delay
    int usecs = 100;
    start = timer_get_ticks();
    timer_delay_us(usecs);
    finish = timer_get_ticks();
    assert( finish >= start + usecs*TICKS_PER_USEC );

    // Test timer delay (ms)
    int msecs = 10;
    start = timer_get_ticks();
    timer_delay_ms(msecs);
    finish = timer_get_ticks();
    assert( finish >= start + msecs*TICKS_PER_USEC );

    // Test timer delay
    int secs = 1;
    start = timer_get_ticks();
    timer_delay(secs);
    finish = timer_get_ticks();
    assert( finish >= start + secs*TICKS_PER_USEC );

    // Test timer blinking LED
    gpio_set_output(GPIO_PB11);
    gpio_set_output(GPIO_PB4);
    gpio_write(GPIO_PB4, 1);

    for (int i = 0; i < 10; i++) {
        gpio_write(GPIO_PB11, 0);
        timer_delay(1);
        gpio_write(GPIO_PB11, 1);
        timer_delay(1);
    }
}

void test_breadboard_connections(void) {
    const int N_SEG = 7, N_DIG = 4;
    gpio_id_t segment[] = {GPIO_PD17, GPIO_PB6, GPIO_PB12, GPIO_PB11, GPIO_PB10, GPIO_PD11, GPIO_PD13};
    gpio_id_t digit[] = {GPIO_PB4, GPIO_PB3, GPIO_PB2, GPIO_PC0};
    gpio_id_t button = GPIO_PD12;

    for (int i = 0; i < N_SEG; i++) {   // configure segments & digits as output
        gpio_set_output(segment[i]);
    }
    for (int i = 0; i < N_DIG; i++) {
        gpio_set_output(digit[i]);
    }
    gpio_set_input(button);         // configure button

    while (1) { // loop forever
        for (int i = 0; i < N_DIG; i++) {   // iterate over digits
            gpio_write(digit[i], 1);        // turn on digit
            for (int j = 0; j < N_SEG; j++) {   // iterate over segments
                gpio_write(segment[j], 1);      // turn on segment
                timer_delay_ms(200);
                while (gpio_read(button) == 0)
                    ;                       // pause while button pressed
                gpio_write(segment[j], 0);  // turn off segment
            }
            gpio_write(digit[i], 0);    // turn off digit
        }
    }
}

void test_gpio_pullup(void) {
    gpio_set_input(GPIO_PC1);
    gpio_set_pullup(GPIO_PC1);
    assert(((*((int *) (0x02000054 + 0x30 + 0)) >> 2) & 0x11) == 0x01);
    assert(gpio_read(GPIO_PC1) == 1);

    gpio_set_input(GPIO_PD22);
    gpio_set_pullup(GPIO_PD22);
    assert(((*((int *) (0x02000054 + 0x60 + 4)) >> 2 * (22%16)) & 0x11) == 0x01);
    assert(gpio_read(GPIO_PD22) == 1);

    gpio_set_input(GPIO_PE13);
    gpio_set_pullup(GPIO_PE13);
    assert(((*((int *) (0x02000054 + 0x90 + 0)) >> 2 * (13%16)) & 0x11) == 0x01);
    assert(gpio_read(GPIO_PE13) == 1);

    gpio_set_input(GPIO_PG16);
    gpio_set_pullup(GPIO_PG16);
    assert(((*((int *) (0x02000054 + 0xF0 + 4)) >> 2 * (16%16)) & 0x11) == 0x01);
    assert(gpio_read(GPIO_PG16) == 1);

    const int N_DIG = 4;
    const int N_SEG = 7;
    const int N_BUTTON = 5;
    enum {F = 0, B, L, R, M}; 
    gpio_id_t five_button[] = {GPIO_PD22, GPIO_PD21, GPIO_PB7, GPIO_PG12, GPIO_PG13};
    gpio_id_t segment[] = {GPIO_PD17, GPIO_PB6, GPIO_PB12, GPIO_PB11, GPIO_PB10, GPIO_PD11, GPIO_PD13};
    gpio_id_t digit[] = {GPIO_PB4, GPIO_PB3, GPIO_PB2, GPIO_PC0};
    char letters[] = {0b01110001, 0b01111100, 0b00111000, 0b01010000};

    for (int i = 0; i < N_BUTTON; i++) {
        gpio_set_input(five_button[i]);
        gpio_set_pullup(five_button[i]);
        assert(gpio_read(five_button[i]) == 1);
    }
    for (int i = 0; i < N_SEG; i++) {   // configure segments & digits as output
        gpio_set_output(segment[i]);
    }
    for (int i = 0; i < N_DIG; i++) {
        gpio_set_output(digit[i]);
    }

    /*
    while (gpio_read(five_button[M])) {
        for (int j = F; j <= R; j++) {
            if (!gpio_read(five_button[j])) {
                int digitNum = j;
                char letter = letters[j];
                for (int i = 0; i < N_DIG; i++) {
                    if (i == digitNum) {
                        gpio_write(digit[i], 1);
                    }
                    else {
                        gpio_write(digit[i], 0);
                    }
                }
                for (int i = 0; i < N_SEG; i++) {
                    gpio_write(segment[i], letter & 1);
                    letter = letter >> 1;
                }
                timer_delay_ms(1);
            }
        }
    }
    */
}

void main(void) {
    gpio_init();
    timer_init();

    // Uncomment the call to each test function below when you have implemented
    // the functions and are ready to test them

    //test_gpio_set_get_function();
    //test_gpio_read_write();
    //test_timer();     //commenting these out because they take too long
    //test_breadboard_connections();
    test_gpio_pullup();
}
