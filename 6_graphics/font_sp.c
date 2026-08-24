/*
 * Support for sitelen pona pictographic font
 * 
 * Author: Harrison Chen
 * Version: 2/25/26
 */

#include "font_sp.h"
#include "sitelenpona_font.c"
#include "strings.h"

#define NUM_TP_WORDS 133
const char *toki_pona[] = {
    "", "a", "akesi", "ala", "alasa", "ale", "anpa", "ante", "anu", "awen", "e", "en", "esun", "ijo", "ike", "ilo", "insa",
    "jaki", "jan", "jelo", "jo", "kala", "kalama", "kama", "kasi", "ken", "kepeken", "kijetesantakalu", "kili", "kin", 
    "kipisi", "kiwen", "ko", "kon", "kule", "kulupu", "kute", "la", "lape", "laso", "lawa", "leko", "len", "lete", "li", 
    "lili", "linja", "lipu", "loje", "lon", "luka", "lukin", "lupa", "ma", "mama", "mani", "meli", "mi", "mije", "misikeke",
    "moku", "moli", "monsi", "monsuta", "mu", "mun", "musi", "mute", "n", "namako", "nanpa", "nasa", "nasin", "nena", "ni", 
    "nimi", "noka", "o", "olin", "ona", "open", "pakala", "pali", "palisa", "pan", "pana", "pi", "pilin", "pimeja", "pini", 
    "pipi", "poka", "poki", "pona", "sama", "seli", "selo", "seme", "sewi", "sijelo", "sike", "sin", "sina", "sinpin", 
    "sitelen", "soko", "sona", "soweli", "suli", "suno", "supa", "suwi", "tan", "taso", "tawa", "telo", "tenpo", "toki", 
    "tomo", "tu", "unpa", "uta", "utala", "walo", "wan", "waso", "wawa", "weka", "wile", "toki-pona", "ni2", "epiku", 
    "linluwi", "meso"
};

static struct {
    const struct font *font;
} const module = {
    .font = &sitelen_pona,
};

int get_toki_pona(const char *word) {
    int found = 0;
    for (int i = 1; i <= NUM_TP_WORDS; i++) {
        if (!strcmp(word, toki_pona[i])) {
            found = i;
            break;
        }
    }
    return found;
}

int font_sp_get_glyph_height(void) {
    return module.font->glyph_height;
}

int font_sp_get_glyph_width(void) {
    return module.font->glyph_width;
}

int font_sp_get_glyph_size(void) {
    return font_sp_get_glyph_width() * font_sp_get_glyph_height();
}

/*
 * Extract glyph pixels for requested character from font bitmap.
 * Read bits from font bitmap and store into array of bytes, one byte per pixel.
 * Use 0xff byte for 'on' pixel, 0x0 for 'off' pixel.
 */
bool font_sp_get_glyph(char *word, uint8_t buf[], size_t buflen) {
    if (buflen != font_sp_get_glyph_size()) {
        return false;
    }
    int tp_index = get_toki_pona(word);
    if (!tp_index) {
        return false;
    }

    int index = 0;
    int nbits_in_row = (module.font->last_char - module.font->first_char + 1) * font_sp_get_glyph_width();
    int x_offset = (tp_index - module.font->first_char);
    for (int y = 0; y < font_sp_get_glyph_height(); y++) {
        for (int x = 0; x < font_sp_get_glyph_width(); x++) {
            int bit_index = y * nbits_in_row + x_offset * font_sp_get_glyph_width() + x;
            // extract single bit for this pixel from bitmap
            int val = module.font->pixel_data[bit_index];
            // use 0xff for on pixel, 0x0 for off pixel
            buf[index++] = val != 0 ? 0xFF : 0x00;
        }
    }
    return true;
}
