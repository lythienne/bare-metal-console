/* File: shell.c
 * -------------
 * Methods to implement a basic shell on the Mango Pi 
 *
 * Author: Harrison Chen
 * Version: 2/17/26
 */
#include "shell.h"
#include "gpio.h"
#include "keyboard.h"
#include "mango.h"
#include "ps2_keys.h"
#include "shell_commands.h"
#include "uart.h"
#include "malloc.h"
#include "strings.h"
#include <stdarg.h>
#include <stddef.h>
#include "ringbuffer.h"
#include "printf.h"
//#include "console.h" 

#define LINE_LEN 80
#define HISTORY_CAPACITY 10

enum Language {English, TokiPona};

// Module-level global variables for shell
static struct {
    enum Language language;
    input_fn_t shell_read;
    formatted_fn_t shell_printf;
} module;

//enum Language console_get_language(void);

// NOTE TO STUDENTS:
// Your shell commands output various information and respond to user
// error with helpful messages. The specific wording and format of
// these messages would not generally be of great importance, but
// in order to streamline grading, we ask that you aim to match the
// output of the reference version.
//
// The behavior of the shell commands is documented in "shell_commands.h"
// https://cs107e.github.io/header#shell_commands
// The header file gives example output and error messages for all
// commands of the reference shell. Please match this wording and format.
//
// Your graders thank you in advance for taking this care!

static rb_t *history = NULL;
static int history_size = 0;

int rb_size(void) {
    rb_t *new_rb = rb_new();
    int i = 0;
    while (!rb_empty(history)) {
        rb_enqueue(new_rb, rb_dequeue(history));
        i++;
    }
    history = new_rb;
    return i;
}

char *rb_get(int i) {
    rb_t *new_rb = rb_new();
    char *line = NULL;
    while (i > 0) {
        if (rb_empty(history)) {
            return NULL;
        }
        line = (char *) (long) rb_dequeue(history);
        rb_enqueue(new_rb, (int) (long) line);
        i--;
    }
    while (!rb_empty(history)) {
        rb_enqueue(new_rb, rb_dequeue(history));
    }
    history = new_rb;
    return line;
}

int cmd_history(int argc, const char* argv[]) {
    rb_t *new_rb = rb_new();
    int i = 1;
    while (!rb_empty(history)) {
        char *line = (char *) (long) rb_dequeue(history);
        module.shell_printf("%d %s\n", i, line);
        rb_enqueue(new_rb, (int) (long) line);
        i++;
    }
    history = new_rb;
    return 0;
}

int cmd_heapreport(int argc, const char* argv[]) {
    malloc_report();
    return 0;
}

int cmd_repeat_last(int argc, const char* argv[]) {
    if (history_size == 0) {
        module.shell_printf((module.language) ? 
                            "pakala: tenpo pini la mu li lon ala\n" :
                            "error: no previous command\n");
        return -1;
    }
    return shell_evaluate((char *) (long) rb_get(history_size));
}

int cmd_sitelen(int argc, const char *argv[]) {
    char buf[LINE_LEN] = {'\0'};
    for (int i = 1; i < argc; ++i) {
        strlcat(buf, argv[i], LINE_LEN);
        strlcat(buf, " ", LINE_LEN);
    }
    module.shell_printf("@sp%s\n", buf);
    return 0;
}

int cmd_language(int argc, const char *argv[]) {
    if (argc == 2) {
        if (!strcmp(argv[1], "EN")) {
            module.language = English;
            return 0;
        }
        else if (!strcmp(argv[1], "TP")) {
            module.language = TokiPona;
            return 0;
        }
    }
    else {
        module.shell_printf((module.language) ? 
                            "pakala: toki pi nimi '%s' li lon ilo ni ala\n" : 
                            "error: no language named '%s' on this system\n", argv[1]);
        return -1;
    }
    module.shell_printf((module.language) ? 
                        "pakala: toki pi nimi suli li lon ilo ni ala\n" : 
                        "error: no language with a 2+ word name on this system\n");
    return -1;
}

#define RPT_INDEX 10
#define NUM_CMDS 11
static const command_t en_commands[] = {
    {"help",  "help [cmd]\t\t",     "print command usage and description", cmd_help},
    {"echo",  "echo [args]\t\t",    "print arguments", cmd_echo},
    {"clear", "clear\t\t\t",        "clear screen (if your terminal supports it)", cmd_clear},
    {"reboot", "reboot\t\t\t",      "reboot the Mango Pi", cmd_reboot},
    {"peek",  "peek [addr]\t\t",    "print contents of memory at address", cmd_peek},
    {"poke",  "poke [addr] [val]\t","store value into memory at address", cmd_poke},
    {"history", "history\t\t\t",    "print last used commands (up to 10)", cmd_history},
    {"heapreport", "heapreport\t\t","print a report of current heap usage", cmd_heapreport},
    {"sitelen", "sitelen [args]\t\t","prints arguments as sitelen pona", cmd_sitelen},
    {"language", "language [EN/TP]\t", "changes system language to selected language", cmd_language},
    {"!!", "!!\t\t\t",              "repeats last command", cmd_repeat_last},
};
static const command_t tp_commands[] = {
    {"sona",    "sona [cmd]             ",   "pana toki e nasin pi mu ni", cmd_help},
    {"kalama",  "kalama [args]            ","toki e ijo 'args'", cmd_echo},
    {"weka",    "weka                   ",         "weka e sitelen tan sinpin (ilo ken e ni la)", cmd_clear},
    {"lape",    "lape                   ",         "ilo ni o lape", cmd_reboot},
    {"lukin",   "lukin [addr]            ", "lukin e nanpa lon nanpa ma 'addr'", cmd_peek},
    {"ante",    "ante [addr] [val]      ", "ante e nanpa lon nanpa ma 'addr'tawa nanpa 'val'", cmd_poke},
    {"pini",    "pini                   ",         "pana e mu pini  10", cmd_history},
    {"poki",    "poki                   ",         "pana e jaki pi poki ilo", cmd_heapreport},
    {"sitelen", "sitelen [args]            ", "toki e ijo kepeken sitelen pona", cmd_sitelen},
    {"toki",    "toki [EN/TP]           ", "ante e toki ilo tawa toki pi wile jan", cmd_language},
    {"!!", "!!                 ", "awen mu e pini", cmd_repeat_last},
};

#define commands ((module.language) ? tp_commands : en_commands)

int find_cmd(const char* cmd_str) {
    bool found = 0;
    int i = 0;
    while (!found && i < NUM_CMDS) {
        if (strcmp(commands[i].name, cmd_str) == 0) {
            found = 1;
        }
        else {
            i++;
        }
    }
    if (!found) {
        module.shell_printf((module.language) ? 
                            "pakala: mu pi nimi '%s' li lon ala\n" : 
                            "error: no such command '%s'\n", cmd_str);
        return -1;
    }
    return i;
}

int cmd_echo(int argc, const char *argv[]) {
    char buf[LINE_LEN] = {'\0'};
    for (int i = 1; i < argc; ++i) {
        strlcat(buf, argv[i], LINE_LEN);
        strlcat(buf, " ", LINE_LEN);
    }
    module.shell_printf("%s\n", buf);
    return 0;
}

int cmd_reboot(int argc, const char *argv[]) {
    while (!rb_empty(history)) {
        free((char *) (long) rb_dequeue(history));
    }
    module.shell_printf((module.language) ? 
                        "ilo ni o lape...\n" :
                        "Rebooting...\n");
    mango_reboot();
}

int cmd_peek(int argc, const char *argv[]) {
    if (argc == 1) {
        module.shell_printf((module.language) ?
                            "pakala: mu lukin li wile e ijo  1[addr]\n" :
                            "error: peek expects 1 argument [addr]\n");
        return -1;
    }
    const char **endptr = &argv[1];
    unsigned long addr = strtonum(argv[1], endptr);
    if (**endptr) {
        module.shell_printf((module.language) ? 
                            "pakala: mu lukin li sona ala e nanpa ma '%s'\n" :
                            "error: peek cannot convert '%s'\n", argv[1]);
        return -1;
    }
    if (addr % 4 != 0) {
        module.shell_printf((module.language) ? 
                            "pakala: nanpa ma pi mu lukin o open lon kulupu  4\n" :
                            "error: peek address must be 4-byte aligned\n");
        return -1;
    }
    module.shell_printf("%p: %08x\n", (int *) addr, *((int *) addr));
    return 0;
}

int cmd_poke(int argc, const char *argv[]) {
    if (argc < 3) {
        module.shell_printf((module.language) ?
                            "pakala: mu ante li wile e ijo  2[addr] en [val]\n" :
                            "error: poke expects 2 arguments [addr] and [val]\n");
        return -1;
    }
    const char **endptr = &(argv[1]);
    unsigned long addr = strtonum(argv[1], endptr);
    if (**endptr) {
        module.shell_printf((module.language) ? 
                            "pakala: mu ante li sona ala e nanpa ma '%s'\n" :
                            "error: poke cannot convert '%s'\n", argv[1]);
        return -1;
    }
    const char **endptr2 = &(argv[2]);
    unsigned long val = strtonum(argv[2], endptr2);
    if (**endptr2) {
        module.shell_printf((module.language) ? 
                            "pakala: mu ante li sona ala e nanpa ma '%s'\n" :
                            "error: poke cannot convert '%s'\n", argv[2]);
        return -1;
    }
    if (addr % 4 != 0) {
        module.shell_printf((module.language) ? 
                            "pakala: nanpa ma pi mu ante o open lon kulupu  4\n" :
                            "error: poke address must be 4-byte aligned\n");
        return -1;
    }
    *((int *) addr) = val;
    return 0;
}

int cmd_help(int argc, const char *argv[]) {
    if (argc == 1) {
        for (int i = 0; i < NUM_CMDS; i++) {
            module.shell_printf("%s%s\n", commands[i].usage, commands[i].description);
        }
    }
    else {
        int cmd_index = find_cmd(argv[1]);
        if (cmd_index == -1) {
            return -1;
        }
        module.shell_printf("%s%s\n", commands[cmd_index].usage, commands[cmd_index].description);
    }
    return 0;
}

int cmd_clear(int argc, const char* argv[]) {
    module.shell_printf("\f");   // formfeed character
    return 0;
}


void shell_init(input_fn_t read_fn, formatted_fn_t print_fn) {
    module.shell_read = read_fn;
    module.shell_printf = print_fn;
    module.language = English; //console_get_language(); 
}

void shell_bell(void) {
    uart_putchar('\a');
}

// returns true if pass, false if not
bool backward_check(int i) {
    if (i == 0) {
        shell_bell();
        return 0;
    }
    return 1;
}

static void buf_printf(char *print_buf, size_t bufsize, const char *format, ...) {
    char buf[bufsize];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, bufsize, format, args);
    va_end(args);

    strlcat(print_buf, buf, bufsize);
}

void shift_chars(char buf[], int index, int len, size_t bufsize) {
    if (len == bufsize - 1) {
        len--;
    }
    for (int i = len + index; i > index; i--) {
        buf[i] = buf[i - 1];
    }
}

void cursor_back(int num, char *print_buf, size_t bufsize) {
    for (int count = 0; count < num; count++) {
        buf_printf(print_buf, bufsize, "\b");
    }
}

void cursor_forward(int num, char buf[], int i, char *print_buf, size_t bufsize) {
    for (int count = 0; count < num; count++) {
        buf_printf(print_buf, bufsize, "%c", buf[count + i]);
    }
}

void wipe(int num, char *print_buf, size_t bufsize) {
    for (int count = 0; count < num; count++) {
        buf_printf(print_buf, bufsize, " ");
    }
}
/*
 * Returns a pointer to a new null-terminated string containing at most `n`
 * bytes copied from the string pointed to by `src`.
 *
 * Example: strndup("cs107e", 4) == "cs10"
 */
static char *strndup(const char *src, size_t n) {
    char* retStr = malloc(n + 1);
    memcpy(retStr, src, n);
    retStr[n] = '\0';
    return retStr;
}

#define ALT_MARKER 0x80
#define CTRL_MARKER 0x81
void shell_readline(char buf[], size_t bufsize) {
    if (!history) {
        history = rb_new();
    }
    int history_index = history_size;
    int i = 0;
    char curr_buf[bufsize];
    memset(buf, '\0', bufsize);
    char cur = module.shell_read();
    char print_buf[bufsize];
    while (cur != '\n' && cur != '\n' + CTRL_MARKER) {
        while (cur > 0x7f && cur != PS2_KEY_ARROW_LEFT && cur != PS2_KEY_ARROW_RIGHT &&
               cur != 'a' + CTRL_MARKER && cur != 'e' + CTRL_MARKER &&
               cur != PS2_KEY_ARROW_UP && cur != PS2_KEY_ARROW_DOWN) {
            shell_bell();
            cur = module.shell_read();
        }
        int len = strlen(buf + i);
        switch (cur) {
            case '\b':
            case '\b' + CTRL_MARKER:
                if (backward_check(i)) {
                    buf_printf(print_buf, bufsize, "\b");
                    if (buf[i] != '\0'){
                        for (int count = 0; count < len; count++) {
                            buf[i + count - 1] = buf[i + count];
                        }
                        for (int count = 0; count < len; count++) {
                            buf_printf(print_buf, bufsize, "%c", buf[count + i - 1]);
                        }
                    }
                    buf[len + --i] = '\0';
                    buf_printf(print_buf, bufsize, " ");
                    cursor_back(len + 1, print_buf, bufsize);
                }
                break;
            case PS2_KEY_ARROW_LEFT:
                if (backward_check(i)) {
                    buf_printf(print_buf, bufsize, "\b");
                    i--;
                }
                break;
            case PS2_KEY_ARROW_RIGHT:
                if (buf[i] == '\0') {
                    shell_bell();
                }
                else {
                    buf_printf(print_buf, bufsize, "%c", buf[i]);
                    i++;
                }
                break;
            case PS2_KEY_ARROW_UP:
                if (history_index < 1) {
                    shell_bell();
                }
                else { 
                    cursor_back(i, print_buf, bufsize);         //wipe line
                    wipe(len + i, print_buf, bufsize);
                    cursor_back(len + i, print_buf, bufsize);
                    if (history_index == history_size) {
                        memcpy(curr_buf, buf, bufsize);
                    }

                    memcpy(buf, rb_get(history_index), bufsize);
                    i = strlen(buf);
                    cursor_forward(i, buf, 0, print_buf, bufsize);
                    history_index--;
                }
                break;
            case PS2_KEY_ARROW_DOWN:
                history_index++;
                if (history_index >= history_size + 1) {
                    history_index--;
                    shell_bell();
                }
                else {
                    cursor_back(i, print_buf, bufsize);         //wipe line
                    wipe(len + i, print_buf, bufsize);
                    cursor_back(len + i, print_buf, bufsize);
                    
                    if (history_index == history_size) {
                        memcpy(buf, curr_buf, bufsize);
                    }
                    else {
                        memcpy(buf, rb_get(history_index + 1), bufsize);
                    }
                    i = strlen(buf);
                    cursor_forward(i, buf, 0, print_buf, bufsize);
                }
                break;
            case 'a' + CTRL_MARKER:
                cursor_back(i, print_buf, bufsize);
                i = 0;
                break;
            case 'e' + CTRL_MARKER:
                cursor_forward(len, buf, i, print_buf, bufsize);
                i += len;
                break;
            default:
                if (i + len >= bufsize - 1) {
                    shell_bell();
                }
                else {
                    if (buf[i] != '\0') {
                        shift_chars(buf, i, len, bufsize);
                    }
                    buf[i] = cur;
                    cursor_forward(len + 1, buf, i, print_buf, bufsize);
                    cursor_back(len, print_buf, bufsize);
                    i++;
                }
            
        }
        module.shell_printf("%s", print_buf);
        *print_buf = '\0';
        cur = module.shell_read();
    }
    if (i >= bufsize) {
        buf[bufsize - 1] = 0;
    }
    module.shell_printf("\n");
}

static bool isspace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n';
}

static int tokenize(const char *line, const char *array[],  int max) {
    int ntokens = 0;
    const char *cur = line;

    while (ntokens < max) {
        while (isspace(*cur)) cur++;    // skip spaces (stop non-space/null)
        if (*cur == '\0') break;        // no more non-space chars
        const char *start = cur;
        while (*cur != '\0' && !isspace(*cur)) cur++; // advance to end (stop space/null)
        array[ntokens++] = strndup(start, cur - start);   // make heap-copy, add to array
    }
    return ntokens;
}

void free_tokens(const char *tokens[], int num_tokens) {
    for (int i = 0; i < num_tokens; i++) {
        free((void *) tokens[i]);
    }
}

#define TOKEN_MAX 100
int shell_evaluate(const char *line) {
    if (!history) {
        history = rb_new();
    }
    const char *tokens[TOKEN_MAX];
    int nTokens = tokenize(line, tokens, TOKEN_MAX);
    
    if (nTokens == 0) {
        return -1;
    }
    
    int cmd_index = find_cmd(tokens[0]);
    int success = -1;
    if (cmd_index != -1) {
        success = commands[cmd_index].fn(nTokens, tokens);
    }
    
    //free_tokens(tokens, nTokens);
    if (cmd_index != RPT_INDEX) {
        if (history_size == HISTORY_CAPACITY) {              //history full
            free((char *) (long) rb_dequeue(history));
            history_size--;
        }
        char* command = strndup(line, LINE_LEN);
        rb_enqueue(history, (int) (long) command);
        history_size++;
    }
    return success;
}

void shell_run(void) {
    history = rb_new();
    if (module.language == English) {
        module.shell_printf("Welcome to the CS107E shell.\nRemember to type on your PS/2 keyboard!\n");
    }
    else {
        module.shell_printf("o kama pona tawa ilo sona toki pi kulupu CS107E.\no toki kepeken ilo pi sitelen toki PS/2!\n");
    }
    int instr = 1;
    while (1) {
        char line[LINE_LEN];

        module.shell_printf((module.language)? "[%d] ilo> " : "[%d] Pi> ", instr);
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
        if (strcmp(line, "")){
            instr++;
        }
    }
}
