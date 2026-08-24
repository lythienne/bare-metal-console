/* File: extension.c
 * -----------------
 * ***** TODO: add your file header comment here *****
 */
#include "console.h"
#include "gl.h"
#include "gl_mine.h"
#include "printf.h"
#include "uart.h"


void main(void) {
    uart_init();
    printf("Executing main() in %s\n", __FILE__);

    /* TODO:
        Add test code to confirm your extended graphics features
        work correctly in a wide variety of scenarios, including clipping.
    */
    console_startup_screen();
    printf("Completed main() in %s\n", __FILE__);
}
