/* File: console.c
 * ---------------
 * ***** TODO: add your file header comment here *****
 */
#include "console.h"
#include "gl.h"
#include "gl_mine.h"
#include "malloc.h"
#include "printf.h"
#include <stdarg.h>
#include "strings.h"
#include "keyboard.h"

enum Language {English, TokiPona};

// module-level variables, you may add/change this struct as you see fit!
static struct {
    enum Language language;
    color_t bg_color, fg_color;
    unsigned int line_height;

    unsigned int width;         //in chars
    unsigned int height;
    unsigned int x;
    unsigned int y;

    char *text;                //please remember not to go out of bounds ...
} module;

void console_init(int nrows, int ncols, color_t foreground, color_t background) {
    // Please add this amount of vertical space between console rows
    const static int LINE_SPACING = 5;

    module.line_height = gl_get_char_height() + LINE_SPACING;
    module.fg_color = foreground;
    module.bg_color = background;
    module.width = ncols;
    module.height = nrows;
    module.text = malloc(nrows * ncols);

    gl_init(ncols * gl_get_char_width(), nrows * module.line_height, GL_DOUBLEBUFFER);
    console_clear();
}

void console_clear(void) {
    gl_clear(module.bg_color);
    memset(module.text, 0, module.width * module.height);
    module.x = 0;
    module.y = 0;
    gl_swap_buffer();
}

void scroll_up(char (* text_2d)[module.width]) {
    for (int i = 0; i < module.height - 1; i++) {
        memcpy(text_2d[i], text_2d[i + 1], module.width);
    }
    memset(text_2d[module.height - 1], 0, module.width);
    module.y = module.height - 1;
}

void new_line(char (* text_2d)[module.width]) {
    if (++module.y == module.height) {
        scroll_up(text_2d);
    }
    module.x = 0;
}

void draw_text(char (* text_2d)[module.width]) {
    gl_clear(module.bg_color);
    for (int i = 0; i < module.height; i++) {
        /*
        if (module.language == TokiPona)  {
            gl_draw_sp_string(0, i * module.line_height, text_2d[i], module.fg_color);
        }
        else if (text_2d[i][0] == '@' && text_2d[i][1] == 's' && text_2d[i][2] == 'p') {
            gl_draw_sp_string(0, i * module.line_height, text_2d[i] + 3, module.fg_color);
        }
        else {*/
            gl_draw_string(0, i * module.line_height, text_2d[i], module.fg_color);
        //}
    }
    gl_swap_buffer();
}

enum Language console_get_language(void) {
    return module.language;
}

#define MAX_BUF_LINES 5
#define TAB_SIZE 8
int console_printf(const char *format, ...) {
    char (* text_2d)[module.width] = (char (*)[module.width]) module.text;
    char buf[MAX_BUF_LINES * module.width];
    va_list args;
    va_start(args, format);

    int len = vsnprintf(buf, MAX_BUF_LINES * module.width, format, args);
    va_end(args);

    int index = 0;
    static int wrapped = 0;
    while (buf[index]) {
        switch (buf[index]) {
            case '\n': 
                new_line(text_2d);
                wrapped = 0;
                break;
            case '\b':
                if (module.x > 0) {
                    module.x--;
                }
                else if (wrapped) {
                    module.x = module.width - 1;
                    module.y--;
                }
                break;
            case '\f':
                wrapped = 0;
                console_clear();
                break;
            case '\t':
                int numSpaces = 8 - module.x % TAB_SIZE;
                if (module.x + numSpaces < module.width) {
                    for (int i = 0; i < numSpaces; i++) {
                        text_2d[module.y][module.x] = ' ';
                        module.x++;
                    }
                }
                break;
            default:
                text_2d[module.y][module.x] = buf[index];
                if (++module.x == module.width){
                    new_line(text_2d);
                    wrapped = 1;
                }
        }
        index++;
    }
    draw_text(text_2d);
	return len;
}

#define CONSOLEWIDTH (80*14)
#define CONSOLEHEIGHT (30*21)
#define OFFSET 35
void console_startup_screen(void) {
    gl_init(CONSOLEWIDTH, CONSOLEHEIGHT, GL_SINGLEBUFFER);
    gl_clear(GL_BLACK);
    unsigned int height = gl_get_height();
    unsigned int width = gl_get_width();

    gl_draw_rect(width/4, height/4, width/2, height/2, GL_WHITE);

    gl_draw_string(width/2 - 9 * gl_get_char_width(), height/3+50, "Choose a Language!", GL_BLACK);
    gl_draw_rect(width*2/5 - 87, height/2+OFFSET - 2, 174, 54, GL_BLACK);
    gl_draw_rect(width*2/5 - 85, height/2+OFFSET, 170, 50, GL_WHITE);
    gl_draw_string(width*2/5 - 77, height/2+OFFSET + 17, "English (1)", GL_BLACK);

    gl_draw_rect(width*3/5 - 87, height/2+OFFSET - 2, 174, 54, GL_BLACK);
    gl_draw_rect(width*3/5 - 85, height/2+OFFSET, 170, 50, GL_WHITE);
    gl_draw_sp_string(width*3/5 - 42, height/2+OFFSET + 17, "toki pona  (2)", GL_BLACK);

    gl_draw_sp_string(60, 40, "mi olin e toki pona <3", GL_CYAN);
    gl_draw_sp_string(700, 100, "kulupu sona CS107E li pona tawa mi!", GL_GREEN);
    gl_draw_sp_string(100, 500, "mi unpa e mama sina!", GL_BLUE);
    gl_draw_sp_string(130, 200, "tenpo ale la mi lape ala :P", GL_RED);
    gl_draw_sp_string(1000, 300, "o moku e kala pona", GL_PURPLE);
    gl_draw_sp_string(900, 550, "o ilo sona pona!", GL_AMBER);
    gl_draw_sp_string(600, 520, "kiwen li moku e mi", GL_MAGENTA);

    gl_draw_sp_string(0, 590, "Sitelen Pona Glyph Set: a akesi ala alasa ale anpa ante anu awen e en ijo ike ilo insa jaki jan jelo jo kala kalama kama kasi ken kepeken kijetesantakalu kili kin kipisi kiwen ko kon kule kulupu kute la lape laso lawa leko len lete li lili linja lipu loje lon luka lukin lupa ma mama mani meli mi", gl_color(0xBB, 0xBB, 0xBB));
    gl_draw_sp_string(0, 611, "mije misikeke moku moli monsi monsuta mu mun musi mute n namako nanpa nasa nasin nena ni nimi noka o olin ona open pakala pali palisa pan pana pi pilin pimeja pini pipi poka poki pona sama seli selo seme sewi sijelo sike sin sina sinpin sitelen soko sona soweli suli suno supa suwi tan taso tawa telo tenpo toki tomo tu unpa uta utala walo wan waso wawa weka wile toki-pona epiku linluwi meso", gl_color(0xBB, 0xBB, 0xBB));

    char opt = keyboard_read_next();
    while (opt != '1' && opt != '2') {
        opt = keyboard_read_next();
    }
    module.language = (opt == '1') ? English : TokiPona;
    module.language = English;
}

