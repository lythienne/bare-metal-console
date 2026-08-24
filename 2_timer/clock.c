/* File: clock.c
 * -------------
 * Functions to run a simple countdown clock on the MangoPi, has general function to
 * display a digit given a bit pattern and runs a loop counting down for a duration
 * and flashes 6-7 at the end.
 *
 * Author: Harrison Chen
 * Version: 1/23/26
 */
#include "gpio.h"
#include "gpio_extra.h"
#include "timer.h"
#include "stdint.h"

/* __Note__: Do not edit/remove the three lines of code below.
 * These preprocessor directives cooperate with Makefile to allow
 * setting DURATION as part of the build process. You can test your
 * clock application on the duration of your choice by specifying
 * value when invoking make like so
 *       make run DURATION=30
 * The value for DURATION is expressed in seconds and defaults to
 * 67 seconds if not explicitly set. (i.e. 1 min 7 seconds)
 */
#ifndef DURATION
#define DURATION 67
#endif

/* Making these constants global so I don't have to put them in every function that uses
 * them or pass them around
 */
const int SEC_PER_MIN = 60;

// vowels in the international phonetic alphabet
typedef enum {i = 0, I, e, epsilon, ae, back_a, o, u, schwa} Vowel;
// equal temperament
typedef enum {A = 440, As = 466, B = 494, C = 523, Cs = 554, D = 587, 
        Ds = 622, E = 659, F = 698, Fs = 740, G = 784, Gs = 831} Note;

const int N_SEGMENT = 7;
const int N_DIGIT = 4;
const int N_BUTTON = 5;
const int N_SPEAKER = 3;

const gpio_id_t SEGMENT[] = {GPIO_PD17, GPIO_PB6, GPIO_PB12, GPIO_PB11, GPIO_PB10, 
    GPIO_PD11, GPIO_PD13};
const gpio_id_t DIGITS[] = {GPIO_PB4, GPIO_PB3, GPIO_PB2, GPIO_PC0};
const gpio_id_t BUTTON = GPIO_PD12;
//F, B, L, R, M
const gpio_id_t FIVE_BUTTON[] = {GPIO_PD22, GPIO_PD21, GPIO_PB7, GPIO_PG12, GPIO_PG13};
const gpio_id_t SPEAKERS[] = {GPIO_PB0, GPIO_PB1, GPIO_PB9}; 

const uint8_t NUMBERS[] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 
                           0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};

const long PWM = 0x02000c00;
const int OFFSET = 0x20;

const unsigned int CLK_SPD = 24000000;

// bitmask function takes a pointer to a register, a value to bitmask in, the length
// of that value, and the index at which to place the value
void bitmask(volatile unsigned int* reg, int val, int length, int index) {
    int mask = ~0 >> (32 - length);
    *reg = (*reg & ~(mask << index)) | (val << index); 
}

// sets all segment and digit pins to output mode, sets button to input mode
void initializePins(void) {
    const int PWM34_FUNCTION = 0b0010; 

    for (int i = 0; i < N_SEGMENT; i++) {
        gpio_set_output(SEGMENT[i]);
    }
    for (int i = 0; i < N_DIGIT; i++) {
        gpio_set_output(DIGITS[i]);
    }
    gpio_set_input(BUTTON);
    for (int i = 0; i < N_BUTTON; i++) {
        gpio_set_input(FIVE_BUTTON[i]);
        gpio_set_pullup(FIVE_BUTTON[i]);
    }
    
    const int PWMS[] = {3};                 //using PWM 3,4,6

    const long CCU_PWM_GATE = 0x020017ac;   //clock control unit PWM settings
    const int DEASSERT = 1 << 16;           //de-assert pwm reset
    const int PASS = 1;                     //pass pwm gate

    const long PCC = PWM + 0x020;           //PWM clock control (which clock to use)
    const int PCC_OFFSET = 0x004;

    const long PCGR = PWM + 0x040;          //PWM clock gate register
    const int GATE346 = 0b01011000;         //pass PWM 3, 4, 6 (01011000)

    const long PCR = PWM + 0x100;           //PWM clock register
    const int CONFIG = 0b01;                //bit 9 = cycle mode, bit 8 = active low
    
    //universal set up config
    *((volatile unsigned int *) CCU_PWM_GATE) |= DEASSERT | PASS;   //turn clock on for pwm
    bitmask((volatile unsigned int *) PCGR, GATE346, 8, 0);

    for (int i = 0; i < N_SPEAKER; i++) {
        gpio_set_function(SPEAKERS[i], PWM34_FUNCTION);

        *((volatile unsigned int *) (PCC + PCC_OFFSET * (PWMS[i]/2))) = 0x00000000; //PCC at default

        volatile unsigned int *pcr_reg = (unsigned int *) (PCR + PWMS[i] * OFFSET);
        bitmask(pcr_reg, CONFIG, 2, 8);     //config length = 2, index = 8
    }
}

/* turns off all other digits and sets the display digit given by digitNum (0-3)
 * to the pattern in digit
 */
void display(int digitNum, uint8_t digit) {
    for (int i = 0; i < N_SEGMENT; i++) {
        gpio_write(SEGMENT[i], 0);
    }

    for (int i = 0; i < N_DIGIT; i++) {
        if (i == digitNum) {
            gpio_write(DIGITS[i], 1);
        }
        else {
            gpio_write(DIGITS[i], 0);
        }
    }

    for (int i = 0; i < N_SEGMENT; i++) {
        gpio_write(SEGMENT[i], digit & 1);
        digit = digit >> 1;
    }
}

// configs the nth PWM to have frequency act/entire, and to enable/disable
void configPWM(int n, unsigned int entire, unsigned int active, int enable) {
    const long PPR = PWM + 0x104;
    const long PER = PWM + 0x080;

    if (enable) {
        volatile unsigned int *ppr_reg = (unsigned int *) (unsigned long) (PPR + OFFSET * n);
        *ppr_reg = ((entire << 16) | active);  //set entire into upper 16, active into lower
    }

    volatile unsigned int *per_reg = (unsigned int *) (unsigned long) PER;
    bitmask(per_reg, enable, 1, n);     //enable unit size = 1 bit
}

// play 3 pwm pins at 3 different frequencies for some duration
// notes are given in frequencies, using PWM 3, 4, 6
void playChord(int note1, int note2, int note3, int ms) {
    note1 = CLK_SPD/note1;
    note2 = CLK_SPD/note2;
    note3 = CLK_SPD/note3;

    configPWM(3, note1, note1/2, 1);
    configPWM(4, note2, note2/2, 1);
    configPWM(6, note3, note3/2, 1);
    timer_delay_ms(ms);
    configPWM(3, note1, note1/2, 0);
    configPWM(4, note2, note2/2, 0);
    configPWM(6, note3, note3/2, 0);
}

// play a vowel on a note for a duration (a vowel has the same 2
// main formant frequencies independent of the underlying note)
// (these are hard coded)
void playVowel(Vowel vowel, Note note, int ms) {
    switch (vowel) {
        case i:
            playChord(note, 380, 2480, ms);
            break;
        case I:
            playChord(note, 590, 2090, ms);
            break;
        case e:
            playChord(note, 620, 2020, ms);
            break;
        case epsilon:
            playChord(note, 680, 1690, ms);
            break;
        case ae:
            playChord(note, 790, 1690, ms);
            break;
        case back_a:
            playChord(note, 880, 1430, ms);
            break;
        case o:
            playChord(note, 640, 1100, ms);
            break;
        case u:
            playChord(note, 440, 1000, ms);
            break;
        case schwa:
            playChord(note, 600, 1200, ms);
            break;
    }
}

// loop to input countdown clock duration using five button
int inputTime(int duration) {
    const int DIGIT_0 = 600;
    const int DIGIT_1 = 60;
    const int DIGIT_2 = 10;
    const int DIGIT_3 = 1;

    int currDigit = 3;      //start at 3

    while (gpio_read(FIVE_BUTTON[4])) {
        if (!gpio_read(FIVE_BUTTON[0])) {
            switch (currDigit) {
                case 0:
                    if ((duration + DIGIT_0) % (10 * DIGIT_0) > (duration % (10 * DIGIT_0))) {
                        duration += DIGIT_0;
                    }
                    else {
                        duration -= 9 * DIGIT_0;
                    }
                    break;
                case 1:
                    if ((duration + DIGIT_1) % (10 * DIGIT_1) > (duration % (10 * DIGIT_1))) {
                        duration += DIGIT_1;
                    }
                    else {
                        duration -= 9 * DIGIT_1;
                    }
                    break;
                case 2:
                    if ((duration + DIGIT_2) % (6 * DIGIT_2) > (duration % (6 * DIGIT_2))) {
                        duration += DIGIT_2;
                    }
                    else {
                        duration -= 5 * DIGIT_2;
                    }
                    break;
                case 3:
                    if ((duration + DIGIT_3) % (10 * DIGIT_3) > (duration % (10 * DIGIT_3))) {
                        duration += DIGIT_3;
                    }
                    else {
                        duration -= 9 * DIGIT_3;
                    }
                    break;
            }
        }
        else if (!gpio_read(FIVE_BUTTON[1])) {
            switch (currDigit) {
                case 0:
                    int after = duration - DIGIT_0;
                    if (after >= 0 && after % (10 * DIGIT_0) < (duration % (10 * DIGIT_0))) {
                        duration -= DIGIT_0;
                    }
                    else {
                        duration += 9 * DIGIT_0;
                    }
                    break;
                case 1:
                    after = duration - DIGIT_1;
                    if (after >= 0 && after % (10 * DIGIT_1) < (duration % (10 * DIGIT_1))) {
                        duration -= DIGIT_1;
                    }
                    else {
                        duration += 9 * DIGIT_1;
                    }
                    break;
                case 2:
                    after = duration - DIGIT_2; 
                    if (after >= 0 && after % (6 * DIGIT_2) < (duration % (6 * DIGIT_2))) {
                        duration -= DIGIT_2;
                    }
                    else {
                        duration += 5 * DIGIT_2;
                    }
                    break;
                case 3:
                    after = duration - DIGIT_3; 
                    if (after >= 0 && after % (10 * DIGIT_3) < (duration % (10 * DIGIT_3))) {
                        duration -= DIGIT_3;
                    }
                    else {
                        duration += 9 * DIGIT_3;
                    }
                    break;
            }
        }
        else if (!gpio_read(FIVE_BUTTON[2])) {
            currDigit = (currDigit + 3) % 4;
        }
        else if (!gpio_read(FIVE_BUTTON[3])) {
            currDigit = (currDigit + 1) % 4;
        }

        const int RATE = 100;
        int min = duration / SEC_PER_MIN;
        int sec = duration % SEC_PER_MIN;

        for (int i = 0; i < RATE; i++) {
            if (i < RATE * 7 / 10 || currDigit != 0) {
                display(0, NUMBERS[min / 10]);
                timer_delay_us(500);
            }
            if (i < RATE * 7 / 10 || currDigit != 1) {
                display(1, NUMBERS[min % 10]);
                timer_delay_us(500);
            }
            if (i < RATE * 7 / 10 || currDigit != 2) {
                display(2, NUMBERS[sec / 10]); 
                timer_delay_us(500);
            }
            if (i < RATE * 7 / 10 || currDigit != 3) {
                display(3, NUMBERS[sec % 10]);
                timer_delay_us(500);
            }
        }
    }
    return duration;
}

// cycles the word "too LonG" on the display
void tooLongLoop(void) {
    const uint8_t TOO_LONG[] = {0b01111000, 0b01011100, 0b01011100, 0b00000000, 0b00111000,
                                0b01011100, 0b01010100, 0b01111101, 0b00000000, 0b00000000,
                                0b00000000};

    const int MSG_LENGTH = 11;
    const int RATE = 50;
    int letter = 0;
    while (1) {
        for (int i = 0; i < RATE; i++) {
            display(0, TOO_LONG[letter % MSG_LENGTH]);
            timer_delay_ms(1);
            display(1, TOO_LONG[(letter + 1) % MSG_LENGTH]);
            timer_delay_ms(1);
            display(2, TOO_LONG[(letter + 2) % MSG_LENGTH]);
            timer_delay_ms(1);
            display(3, TOO_LONG[(letter + 3) % MSG_LENGTH]);
            timer_delay_ms(1);
        }
        letter++;
    }
}

// displays countdown as digits and starts counting down when button pressed
void countdownLoop(int countdown) {
    int buttonPressed = 0;
    while (countdown > 0) {
        const int RATE = 250;
        int min = countdown / SEC_PER_MIN;
        int sec = countdown % SEC_PER_MIN;

        for (int i = 0; i < RATE; i++) {
            display(0, NUMBERS[min / 10]);
            timer_delay_ms(1);
            display(1, NUMBERS[min % 10]);
            timer_delay_ms(1);
            display(2, NUMBERS[sec / 10]); 
            timer_delay_ms(1);
            display(3, NUMBERS[sec % 10]);
            timer_delay_ms(1);
            if (!buttonPressed && !gpio_read(BUTTON)) {
                buttonPressed = 1;
            }
        }
        if (buttonPressed) {
            countdown--;
        }
    }
}

// called after countdown ends, flashes alternating 67
void endLoop(void) {
    const int VISIBLE_FLASH_DELAY = 30;
    while (1) {
        for (int i = 0; i < 8; i++) {
            display(1, NUMBERS[6]);
            timer_delay_ms(VISIBLE_FLASH_DELAY);
            display(2, NUMBERS[7]);
            timer_delay_ms(VISIBLE_FLASH_DELAY);
        }
        display(2, 0);

        playVowel(I, 1000, 200);
        timer_delay_ms(100);
        playVowel(epsilon, 490, 500);
        timer_delay_ms(20);
        playVowel(epsilon, 480, 500);
    }
}

// sings (with vowels) the star spangled banner (hard coded)
// PWM3 has base note, PWM4 and 6 have the vowel formants (overtones)
void ssb(void) {
    const int BPM = 70;
    const int QUARTER = 60000/70;       //70 bpm
    const int EIGHTH = QUARTER/2;
    const int SIXTEENTH = EIGHTH/2;

    const int HALF = QUARTER * 2;

    //o-ou seI caen iu si
    playVowel(o, Ds*2, EIGHTH+SIXTEENTH);
    playVowel(o, C*2, SIXTEENTH-BPM);
    playVowel(u, C*2, BPM/2);
    timer_delay_ms(BPM/2);
    playVowel(e, Gs, QUARTER-BPM*2.5);
    playVowel(I, Gs, BPM/2);
    timer_delay_ms(BPM*2);
    playVowel(ae, C*2, QUARTER);
    playVowel(i, Ds*2, BPM/2);
    playVowel(u, Ds*2, QUARTER-BPM);
    timer_delay_ms(BPM/2);
    playVowel(i, Gs*2, HALF-BPM*2);
    timer_delay_ms(BPM*2);

    //baI thschwa dans schwa li lait
    playVowel(back_a, C*4, EIGHTH+SIXTEENTH-BPM/2);
    playVowel(I, C*4, BPM/2);
    playVowel(schwa, As*4, SIXTEENTH);
    playVowel(back_a, Gs*2, QUARTER-BPM/2);
    timer_delay_ms(BPM/2);
    playVowel(schwa, C*2, QUARTER);
    playVowel(i, D*2, QUARTER);
    playVowel(back_a, Ds*2, QUARTER-BPM/2);
    playVowel(I, Ds*2, BPM/2);
    timer_delay_ms(QUARTER);

    //wschwat so prau dli ui heild
    playVowel(schwa, Ds*2, EIGHTH+SIXTEENTH-BPM*2);
    timer_delay_ms(BPM*2);
    playVowel(o, Ds*2, SIXTEENTH-BPM);
    timer_delay_ms(BPM);
    playVowel(back_a, C*4, QUARTER+EIGHTH-BPM*1.5);
    playVowel(u, As*4, BPM/2);
    timer_delay_ms(BPM);
    playVowel(i, As*4, EIGHTH);
    playVowel(u, Gs*2, BPM/2);
    playVowel(i, Gs*2, QUARTER-BPM);
    timer_delay_ms(BPM/2);
    playVowel(e, G*2, HALF-BPM*2.5);
    playVowel(I, G*2, BPM/2);
    timer_delay_ms(BPM*2);

    //aet thschwa tuai laits laest gli mIng
    playVowel(ae, F*2, EIGHTH+SIXTEENTH-BPM);
    timer_delay_ms(BPM);
    playVowel(schwa, G*2, SIXTEENTH-BPM);
    timer_delay_ms(BPM);
    playVowel(u, Gs*2, BPM/2); 
    playVowel(back_a, Gs*2, QUARTER-BPM); 
    playVowel(i, Gs*2, BPM/2); 
    playVowel(back_a, Gs*2, QUARTER-BPM*1.5); 
    playVowel(i, Gs*2, BPM/2); 
    timer_delay_ms(BPM);
    playVowel(ae, Ds*2, QUARTER-BPM*2);
    timer_delay_ms(BPM*2);
    playVowel(i, C*2, QUARTER);
    playVowel(I, Gs, QUARTER);

    //hus brad straIps aend brait stars
    playVowel(o, Ds*2, EIGHTH+SIXTEENTH);
    playVowel(back_a, C*2, SIXTEENTH-BPM*1.5);
    timer_delay_ms(BPM*1.5);
    playVowel(back_a, Gs, QUARTER-BPM*2.5);
    playVowel(I, Gs, BPM/2);
    timer_delay_ms(BPM*2);
    playVowel(ae, C*2, QUARTER-BPM);
    timer_delay_ms(BPM);
    playVowel(back_a, Ds*2, QUARTER-100);
    playVowel(i, Ds*2, 50);
    timer_delay_ms(50);
    playVowel(back_a, Gs*2, HALF-100);
    timer_delay_ms(100);


}

// starts a countdown based on given duration
int main(void) {
    int countdown = DURATION;
    const int MAX_DURATION = 99 * 60 + 59;

    initializePins();
    ssb();

    countdown = inputTime(countdown);

    // If duration too long, loop "too LonG" text
    if (countdown > MAX_DURATION) {
        tooLongLoop();    
    }

    // Display number and count down after button pressed 
    countdownLoop(countdown);
    
    // on countdown end display flashing 6-7
    endLoop();
    
    return countdown;
}
