/* File: keyboard.c
 * -----------------
 * Mid to high level methods to read and interpret ps2 keyboard sequences.
 *
 * Author: Harrison Chen
 * Version 2/17/26
 */
#include "keyboard.h"
#include "ps2.h"
#include "ps2_keys.h"
#include <stdint.h>
#include "malloc.h"

static ps2_device_t *dev;

void keyboard_init(gpio_id_t clock_gpio, gpio_id_t data_gpio) {
    dev = ps2_new(clock_gpio, data_gpio);
}

// I'm calling free here because I can't add ps2_free to the header, but
// otherwise I would call that
void keyboard_destroy() {
    free(dev);
}

uint8_t keyboard_read_scancode(void) {
    return ps2_read(dev);
}

key_action_t keyboard_read_sequence(void) {
    int press = 1;
    uint8_t code = keyboard_read_scancode();
    if (code == 0xE0) {
        code = keyboard_read_scancode();
    }
    if (code == 0xF0) {
        press = 0;
        code = keyboard_read_scancode();
    }
    key_action_t action = {(press)? KEY_PRESS : KEY_RELEASE, code};

    return action;
}

#define CAPS_LOCK (0x58)
#define SHIFT (0x12)
#define RSHIFT (0x59)
#define ALT (0x11)
#define CTRL (0x14)

key_event_t keyboard_read_event(void) {
    static keyboard_modifiers_t modifiers;
    static uint8_t ctrlReleased = 1;
    key_action_t action = keyboard_read_sequence();
    while (action.keycode == CAPS_LOCK || action.keycode == SHIFT ||
            action.keycode == RSHIFT || action.keycode == ALT || action.keycode == CTRL) {
        keyboard_modifiers_t mask = 0;
        switch (action.keycode) {
            case CAPS_LOCK:
                if (ctrlReleased && action.what == KEY_PRESS) {           //caps lock only 
                    modifiers ^= KEYBOARD_MOD_CAPS_LOCK;
                    ctrlReleased = 0;
                }
                else if (action.what == KEY_RELEASE) {
                    ctrlReleased = 1;
                }
                action = keyboard_read_sequence();
                continue;
            case SHIFT:
            case RSHIFT:
                mask = KEYBOARD_MOD_SHIFT;
                break;
            case ALT:
                mask = KEYBOARD_MOD_ALT;
                break;
            case CTRL:
                mask = KEYBOARD_MOD_CTRL;
                break;
        }
        if (action.what == KEY_PRESS) {
            modifiers |= mask;
        }
        else {
            modifiers &= ~mask;
        }
        action = keyboard_read_sequence();
    }
    key_event_t event = {action, ps2_keys[action.keycode], modifiers};
    return event;
}

int isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

#define ASCII_END 0x90
#define ALT_MARKER 0x80
#define CTRL_MARKER 0x81
char keyboard_read_next(void) {
    key_event_t event;
    for (event = keyboard_read_event(); event.action.what != KEY_PRESS; event = keyboard_read_event()) {}
    uint8_t other_key = (event.modifiers & KEYBOARD_MOD_SHIFT) ||
            (event.modifiers & KEYBOARD_MOD_CAPS_LOCK && isLetter(event.key.ch));
    return ((event.action.keycode < ASCII_END) ? other_key ? event.key.other_ch : event.key.ch : event.action.keycode)
            + ((event.modifiers & KEYBOARD_MOD_ALT) ? ALT_MARKER : 0)
            + ((event.modifiers & KEYBOARD_MOD_CTRL) ? CTRL_MARKER : 0);
}
