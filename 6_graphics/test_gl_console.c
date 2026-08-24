/* File: test_gl_console.c
 * -----------------------
 * ***** TODO: add your file header comment here *****
 */
#include "assert.h"
#include "console.h"
#include "fb.h"
#include "gl.h"
#include "printf.h"
#include "strings.h"
#include "timer.h"
#include "uart.h"

static void pause(const char *message) {
    if (message) printf("\n%s\n", message);
    printf("[PAUSED] type any key in tio to continue: ");
    int ch = uart_getchar();
    uart_putchar(ch);
    uart_putchar('\n');
}

static void test_fb(void) {
    const int SIZE = 500;
    fb_init(SIZE, SIZE, FB_SINGLEBUFFER); // init single buffer

    assert(fb_get_width() == SIZE);
    assert(fb_get_height() == SIZE);
    assert(fb_get_depth() == 4);

    fb_swap_buffer();
    fb_swap_buffer();

    void *cptr = fb_get_draw_buffer();
    assert(cptr != NULL);
    int nbytes = fb_get_width() * fb_get_height() * fb_get_depth();
    memset(cptr, 0x99, nbytes); // fill entire framebuffer with light gray pixels
    pause("Now displaying 500 x 500 screen of light gray pixels");

    fb_init(1280, 720, FB_DOUBLEBUFFER); // init double buffer
    cptr = fb_get_draw_buffer();
    nbytes =  fb_get_width() * fb_get_height() * fb_get_depth();
    memset(cptr, 0xff, nbytes); // fill one buffer with white pixels
    fb_swap_buffer();
    pause("Now displaying 1280 x 720 white pixels");

    cptr = fb_get_draw_buffer();
    memset(cptr, 0x33, nbytes); // fill other buffer with dark gray pixels
    fb_swap_buffer();
    pause("Now displaying 1280 x 720 dark gray pixels");

    for (int i = 0; i < 5; i++) {
        fb_swap_buffer();
        timer_delay_ms(250);
    }
}

static void test_gl(void) {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    // Double buffer mode, make sure you test single buffer too!
    gl_init(WIDTH, HEIGHT, GL_DOUBLEBUFFER);
    assert(gl_get_height() == HEIGHT);
    assert(gl_get_width() == WIDTH);

    // Background is purple
    gl_clear(gl_color(0x55, 0, 0x55)); // create purple color

    // Draw green pixel in lower right
    gl_draw_pixel(WIDTH-10, HEIGHT-10, GL_GREEN);
    assert(gl_read_pixel(WIDTH-10, HEIGHT-10) == GL_GREEN);

    // Blue rectangle in center of screen
    gl_draw_rect(WIDTH/2 - 100, HEIGHT/2 - 50, 200, 100, GL_BLUE);

    // Single amber character
    gl_draw_char(60, 10, 'A', GL_AMBER);
    // Single cyan character
    gl_draw_char(0, 0, 'W', GL_CYAN);

    const char *str = "amogus is cool and I want amogus.\nh\bi\f";
    gl_draw_string(100, 100, str, GL_RED);

    // Show buffer with drawn contents
    gl_swap_buffer();
    pause("Now displaying 1280 x 720, purple bg, single green pixel, blue center rect, amber letter A");

    // Single buffer mode
    gl_init(WIDTH, HEIGHT, GL_SINGLEBUFFER);
    assert(gl_get_height() == HEIGHT);
    assert(gl_get_width() == WIDTH);

    gl_clear(GL_BLACK);
    gl_draw_rect(750, 750, 100, 100, GL_GREEN);
    gl_draw_rect(750, 400, 100, 100, GL_YELLOW);

    gl_draw_string(400, 100, "this string is so long it will go off the screen", GL_MAGENTA);
    gl_draw_string(0, 300, "hi im emily", GL_GREEN);
    gl_draw_string(0, 0, "is this clipped", GL_CYAN);
    gl_swap_buffer();

    pause("Now displaying 1280 x 720, black bg, fully clipped green square, half clipped yellow square, clipped string");
}

static void test_console(void) {
    console_init(25, 50, GL_CYAN, GL_INDIGO);
    pause("Now displaying console: 25 rows x 50 columns, bg indigo, fg cyan");

    // Line 1: Hello, world!
    const char *str = "Hello, world\n";
    assert(console_printf("%s", str) == strlen(str));
    pause("Display Hello World");


    // Add line 2: Happiness == CODING
    console_printf("Happiness");
    console_printf(" == ");
    console_printf("CODING\n");

    // Add 2 blank lines and line 5: I am Pi, hear me roar!
    console_printf("\n\nI am Pi, hear me v\b \broar!\n"); // typo, backspace, correction
    pause("Console printfs");

    // Wrapped text
    console_printf("\n\nThis line is so long that it will wrap around hopefully");
    pause("Wrapped text");

    console_printf("\b\b\b\b\b\b\b\b\b\b! Backspace through wrapped works");
    pause("Backspace through wrapped text");

    // Clear all lines
    console_printf("\f");
    pause("Console clear");

    console_printf("\b67\n");
    pause("Backspace on blank line should do nothing");

    console_printf("\f");
    for (int i = 0; i < 50; i++){
        console_printf("Line %d \n", i);
    }
    pause("Text scrolls properly");

    // Line 2: "Goodbye"
    console_printf("Goodbye!\n");
    pause("Bye!");
}

void gl_draw_sp_string(int x, int y, const char *str, color_t c);

static void test_sp_font(void) {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    // Double buffer mode, make sure you test single buffer too!
    gl_init(WIDTH, HEIGHT, GL_DOUBLEBUFFER);
    gl_clear(GL_BLUE);
    
    gl_draw_sp_string(0, 0, "toki pona li pona tawa mi", GL_WHITE);
    gl_draw_sp_string(0, 100, "mi wile e ni: sitelen ni li pona tawa jan Julie.", GL_WHITE);
    gl_draw_sp_string(0, 200, "ma tomo New York li pona tawa mi", GL_WHITE);
    gl_draw_sp_string(0, 300, "kijetesantakalu kijetesantakalu veryverylongwordblahblahblahblahblah li pona tawa mi", GL_WHITE);
    gl_swap_buffer();
    pause("Toki pona string in sitelen pona\n");
}

/* TODO: Add tests to test your graphics library and console.
   For the graphics library, test both single & double
   buffering and confirm all drawing is clipped to bounds
   of framebuffer
   For the console, make sure to test wrap of long lines and scrolling.
   Be sure to test each module separately as well as in combination
   with others.
*/

void main(void) {
    timer_init();
    uart_init();
    printf("Executing main() in test_gl_console.c\n");

    test_fb();
    test_gl();
    //test_console();
    //test_sp_font();

    printf("Completed main() in test_gl_console.c\n");
}
