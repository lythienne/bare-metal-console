/* File: test_keyboard_shell.c
 * ---------------------------
 * ***** TODO: add your file header comment here *****
 */
#include "assert.h"
#include "keyboard.h"
#include "printf.h"
#include "shell.h"
#include "strings.h"
#include "uart.h"
#include "timer.h"

#define ESC_SCANCODE 0x76

static void test_keyboard_scancodes(void) {
    printf("\nNow reading single scancodes. Type ESC to finish this test.\n");
    while (1) {
        uint8_t scancode = keyboard_read_scancode();
        printf("[%02x]\n", scancode);
        if (scancode == ESC_SCANCODE) break;
    }
    printf("\nDone with scancode test.\n");
}

static void test_keyboard_sequences(void) {
    printf("\nNow reading scancode sequences (key actions). Type ESC to finish this test.\n");
    while (1) {
        key_action_t action = keyboard_read_sequence();
        printf("%s [%02x]\n", action.what == KEY_PRESS ? "  Press" :"Release", action.keycode);
        if (action.keycode == ESC_SCANCODE) break;
    }
    printf("Done with scancode sequences test.\n");
}

static void test_keyboard_events(void) {
    printf("\nNow reading key events. Type ESC to finish this test.\n");
    while (1) {
        key_event_t evt = keyboard_read_event();
        printf("%s PS2_key: {%c,%c} Modifiers: 0x%x\n", evt.action.what == KEY_PRESS? "  Press" : "Release", evt.key.ch, evt.key.other_ch, evt.modifiers);
        if (evt.action.keycode == ESC_SCANCODE) break;
    }
    printf("Done with key events test.\n");
}

static void test_keyboard_chars(void) {
    printf("\nNow reading chars. Type ESC to finish this test.\n");
    while (1) {
        char c = keyboard_read_next();
        if (c >= '\t' && c <= 0x80)
            printf("%c", c);
        else
            printf("[%02x]", c);
        if (c == ps2_keys[ESC_SCANCODE].ch) break;
    }
    printf("\nDone with key chars test.\n");
}

static void test_keyboard_assert(void) {
    char ch;
    printf("\nHold down Shift and type 'g'\n");
    ch = keyboard_read_next();
    assert(ch == 'G');  // confirm user can follow directions and correct key char generated
}

static void test_shell_evaluate(void) {
    shell_init(keyboard_read_next, printf);

    printf("\nTest shell_evaluate on fixed commands.\n");
    int ret = shell_evaluate("echo hello, world!");
    printf("Command result is zero if successful, is it? %d\n", ret);
    assert(ret == 0);
    ret = shell_evaluate("help");
    assert(ret == 0);
    ret = shell_evaluate("help echo");
    assert(ret == 0);
    ret = shell_evaluate("poke 0x40000000 0x107e");
    assert(ret == 0);
    printf("\nThis should print out 0x107e:\n");
    ret = shell_evaluate("peek 0x40000000");
    assert(ret == 0);
}

// This is an example of a "mock" input used for testing. When asked to read
// next character, read_fixed returns char from a fixed string and advances
// its internal index
static char read_fixed(void) {
    const char *input = "echo hello, world\nhelp\n";
    static int index = 0; // note declared static

    char next = input[index];
    index = (index + 1) % strlen(input);
    return next;
}

static void test_shell_readline_fixed_input(void) {
    char buf[80];
    size_t bufsize = sizeof(buf);

    shell_init(read_fixed, printf); // input is fixed sequence of characters

    printf("\nTest shell_readline, feed chars from fixed string as mock input.\n");
    printf("readline> ");
    shell_readline(buf, bufsize);
    printf("readline> ");
    shell_readline(buf, bufsize);
}

static void test_shell_readline_keyboard(void) {
    char buf[80];
    size_t bufsize = sizeof(buf);

    shell_init(keyboard_read_next, printf); // input from keybaord

    for (int i = 1; i <= 20; i++) {
        printf("\nTest shell_readline, type a line of input on ps2 keyboard.\n");
        printf("Test %d> ", i);
        shell_readline(buf, bufsize);
        shell_evaluate(buf);
    }
}

void main(void) {
    uart_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);

    printf("Testing keyboard and shell.\n");

    test_keyboard_scancodes();
    timer_delay_ms(500);

    test_keyboard_sequences();
    timer_delay_ms(500);

    test_keyboard_events();
    timer_delay_ms(500);

    test_keyboard_chars();

    test_keyboard_assert();

    test_shell_evaluate();

    test_shell_readline_fixed_input();

    test_shell_readline_keyboard();

    printf("Finished executing main() in test_keyboard_shell.c\n");
}
