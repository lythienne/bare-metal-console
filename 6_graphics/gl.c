/* File: gl.c
 * ----------
 * ***** TODO: add your file header comment here *****
 */
#include "gl.h"
//#include "font_sp.h"
#include "fb.h"
#include "gl_mine.h"
#include "font.h"
#include "strings.h"
#include "printf.h"

void gl_init(int width, int height, gl_mode_t mode) {
    fb_init(width, height, (fb_mode_t) mode);
}

int gl_get_width(void) {
    return fb_get_width();
}

int gl_get_height(void) {
    return fb_get_height();
}

color_t gl_color(uint8_t r, uint8_t g, uint8_t b) {
    return 0xff << 24 | r << 16 | g << 8 | b;
}

void gl_swap_buffer(void) {
    fb_swap_buffer();
}

void gl_clear(color_t c) {
    color_t *ptr = fb_get_draw_buffer();
    int width = gl_get_width();
    int height = gl_get_height();
    for (int i = 0; i < width * height; i++) {
        *ptr++ = c;
    }
}

void gl_draw_pixel(int x, int y, color_t c) {
    unsigned int width = fb_get_width();
    if (x < 0 || x >= width || y < 0 || y >= fb_get_height()) {
        return;
    }
    unsigned int (*im)[width] = fb_get_draw_buffer();
    im[y][x] = c;
}

color_t gl_read_pixel(int x, int y) {
    unsigned int width = fb_get_width();
    if (x < 0 || x >= width || y < 0 || y >= fb_get_height()) {
        return 0;
    }
    unsigned int (*im)[width] = fb_get_draw_buffer();
    return im[y][x];
}

static bool in_bounds (int x, int y, int w, int h, int buf_width, int buf_height) {
    if (x >= buf_width || y >= buf_height || w < 0 || h < 0) {
        return 0;
    }
    return 1;
}

static void set_bounds (int *x, int *y, int w, int h, int buf_width, int buf_height, int *x1, int *y1) {
    *x1 = *x + w;
    *y1 = *y + h;

    *x = (*x < 0) ? 0 : *x;
    *y = (*y < 0) ? 0 : *y;

    *x1 = (*x1 > buf_width) ? buf_width : *x1;
    *y1 = (*y1 > buf_height) ? buf_height : *y1;

}

void gl_draw_rect(int x, int y, int w, int h, color_t c) {
    unsigned int buf_width = fb_get_width();
    unsigned int buf_height = fb_get_height();

    if (!in_bounds(x, y, w, h, buf_width, buf_height)) {
        return;
    }

    int x1;
    int y1;
    set_bounds(&x, &y, w, h, buf_width, buf_height, &x1, &y1);

    unsigned int (*im)[buf_width] = fb_get_draw_buffer();
    for (int j = y; j < y1; j++) {
        for (int i = x; i < x1; i++) {
            im[j][i] = c;
        }     
    }
}

static void gl_draw_glyph(int x, int y, int width, int height, uint8_t *glyph_buf, int buflen, color_t c) {
    unsigned int buf_width = fb_get_width();
    unsigned int buf_height = fb_get_height();
    
    if (!in_bounds(x, y, width, height, buf_width, buf_height)) {
        return;
    }

    uint8_t (*glyph_im)[width] = (uint8_t (*)[width]) glyph_buf;

    int glyph_x = x;
    int glyph_y = y;

    int x1;
    int y1;
    set_bounds(&x, &y, width, height, buf_width, buf_height, &x1, &y1);

    unsigned int (*im)[buf_width] = fb_get_draw_buffer();
    for (int j = y; j < y1; j++) {
        for (int i = x; i < x1; i++) {
            if (glyph_im[j - glyph_y][i - glyph_x]) {
                im[j][i] = c;
            }
        }
    }
}

void gl_draw_char(int x, int y, char ch, color_t c) {
    int char_width = gl_get_char_width();
    int char_height = gl_get_char_height();

    int buflen = font_get_glyph_size();
    uint8_t char_buf[buflen];
    if (font_get_glyph(ch, char_buf, buflen)) {
        gl_draw_glyph(x, y, char_width, char_height, char_buf, buflen, c);
    }
}

/*
void gl_draw_sitelen_pona(int x, int y, char *word, color_t c) {
    int sp_width = font_sp_get_glyph_width();
    int sp_height = font_sp_get_glyph_height();

    int buflen = font_sp_get_glyph_size();
    uint8_t sp_buf[buflen];
    if (font_sp_get_glyph(word, sp_buf, buflen)) {
        gl_draw_glyph(x, y, sp_width, sp_height, sp_buf, buflen, c);
    }
}
*/

void gl_draw_string(int x, int y, const char* str, color_t c) {
    int char_width = gl_get_char_width();
    while (*str) {
        gl_draw_char(x, y, *str, c);
        x += char_width;
        str++;
    }
}

/*
static bool isLetter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

#define TP_WORD_MAX_LEN 15
void gl_draw_sp_string(int x, int y, const char* str, color_t c) {
    int sp_width = font_sp_get_glyph_width();
    char possible_word[TP_WORD_MAX_LEN + 2];
    memset(possible_word, 0, TP_WORD_MAX_LEN + 2);
    int word_index = 0;

    while (*str) {
        if (isLetter(*str)) {
            possible_word[word_index] = *str;
            word_index++;
            if (word_index > TP_WORD_MAX_LEN) {
                gl_draw_string(x, y, possible_word, c);
                x += sp_width * strlen(possible_word);
                word_index = 0;
                memset(possible_word, 0, TP_WORD_MAX_LEN + 2);
                str++;
                while (isLetter(*str)) {
                    gl_draw_char(x, y, *str, c);
                    x += sp_width;
                    str++;
                }
                str--;
            }
        }
        else if (possible_word[0]){                         //there are letters in possible_word
            if (get_toki_pona(possible_word)) {
                gl_draw_sitelen_pona(x, y, possible_word, c);
                x += sp_width;
                if (*str == ' ') {
                    str++;
                }
            }
            else {
                gl_draw_string(x, y, possible_word, c);
                x += sp_width * strlen(possible_word);
            }
            word_index = 0;
            memset(possible_word, 0, TP_WORD_MAX_LEN + 2);
            str--;
        }
        else {
            gl_draw_char(x, y, *str, c);
            x += sp_width;
        }
        str++;
    }
    if (possible_word[0]) {
        if (get_toki_pona(possible_word)) {
            gl_draw_sitelen_pona(x, y, possible_word, c);
        }
        else {
            gl_draw_string(x, y, possible_word, c);
        }
    }
}
*/

int gl_get_char_height(void) {
    return font_get_glyph_height();
}

int gl_get_char_width(void) {
    return font_get_glyph_width();
}

