/* File: printf.c
 * --------------
 * printf library for printing formatted strings, contains helper functions
 * for converting numbers to strings and other printfs that output to buffers
 *
 * Author: Harrison Chen
 * Version 2/1/26
 */
#include "printf.h"
#include <stdarg.h>
#include <stdint.h>
#include "strings.h"
#include "uart.h"

/* Prototypes for internal helpers.
 * Typically we would qualify these functions as static (private to module)
 * but in order to call them from the test program, must declare externally
 */
void num_to_string(unsigned long num, int base, char *outstr);
const char *hex_string(unsigned long val);
const char *decimal_string(long val);

// max number of digits in long + space for negative sign and null-terminator
#define MAX_DIGITS 25

//assembly instruction should not go above 30 chars
#define MAX_INSN_LEN 30
#define MAX_IMM_LEN 12

//concats to instruction
#define cat_to_insn(x) (strlcat(instruction, (x), MAX_INSN_LEN))

static const char *reg_names[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
                                    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
                                    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
                                    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6" };
static const uint32_t OP = 0b0110011;
static const char *OP_insn_names[14] = {"add", "sll", "slt", "sltu", "xor", "srl", "or", "and", 
                                        "sub", "",    "",    "",     "",    "sra"};
static const uint32_t OP32 = 0b0111011;
static const char *OP32_insn_names[14] = {"addw", "sllw", "", "", "", "srlw", "", "",
                                          "subw", "",     "", "", "", "sraw"};

static const uint32_t OP_IMM = 0b0010011;
static const char *OP_IMM_insn_names[9] = {"addi", "slli", "slti", "sltiu", "xori", "srli", 
                                           "ori", "andi", "srai"};
static const uint32_t OP_IMM_32 = 0b0011011;
static const char *OP_IMM_32_insn_names[6] = {"addiw", "slliw", "", "", "sraiw", "srliw"};

static const uint32_t LOAD = 0b0000011;
static const char *LOAD_insn_names[6] = {"lb", "lh", "lw", "ld", "lbu", "lhu"};

static const uint32_t STORE = 0b0100011;
static const char *STORE_insn_names[4] = {"sb", "sh", "sw", "sd"};

static const uint32_t BRANCH = 0b1100011;
static const char *BRANCH_insn_names[8] = {"beq", "bne", "", "", "blt", "bge", "bltu", "bgeu"};

static const uint32_t LUI = 0b0110111;
static const uint32_t AUIPC = 0b0010111;
static const uint32_t JAL = 0b1101111;
static const uint32_t JALR = 0b1100111;

struct insn {
    uint32_t opcode: 7;
    uint32_t rd:     5;
    uint32_t func3:  3;
    uint32_t rs1:    5;
    uint32_t rs2:    5;
    uint32_t func7:  7;
};

/* Convenience functions `hex_string` and `decimal_string` are provided
 * to you.  You should use the functions as-is, do not change the code!
 *
 * A key implementation detail to note is these functions declare
 * a buffer to hold the output string and return the address of buffer
 * to the caller. If that buffer memory were located on stack, it would be
 * incorrect to use pointer after function exit because local variables
 * are deallocated. To ensure the buffer memory is accessible after
 * the function exists, the declaration is qualified `static`. Memory
 * for a static variable is not stored on stack, but instead in the global data
 * section, which exists outside of any function call. Additionally static
 * makes it so there is a single copy of the variable, which is shared by all
 * calls to the function. Each time you call the function, it overwrites/reuses
 * the same variable/memory.
 *
 * Adding static qualifier to a variable declared inside a function is a
 * highly atypical practice and appropriate only in very specific situations.
 * You will likely never need to do this yourself.
 * Come talk to us if you want to know more!
 */

// converts an unsigned value to its hexadecimal representation in a string
const char *hex_string(unsigned long val) {
    // static buffer to allow use after function returns (see note above)
    static char buf[MAX_DIGITS];
    num_to_string(val, 16, buf); // num_to_string does the hard work
    return buf;
}

// converts a signed value to its decimal representation in a string (with sign)
const char *decimal_string(long val) {
    // static buffer to allow use after function returns (see note above)
    static char buf[MAX_DIGITS];
    if (val < 0) {
        buf[0] = '-';   // add negative sign in front first
        num_to_string(-val, 10, buf + 1); // pass positive val as arg, start writing at buf + 1
    } else {
        num_to_string(val, 10, buf);
    }
    return buf;
}

//
int jalBitArrange(unsigned int func3, unsigned int rs1, unsigned int rs2, unsigned int func7) {
    int res = 0;
    res = (func7 & 0x40) << 14;
    res |= rs1 << 15;
    res |= func3 << 12;
    res |= (rs2 & 1) << 11;
    res |= (func7 & 0x3f) << 5;
    res |= (rs2 & 0x1e);
    return res;
}

// rearranged branch bits from rd section and func7 section into a signed int
int branchBitArrange(unsigned int rd, unsigned int func7) {
    int res = 0;
    res = (func7 & 0x3f) << 5;                           //left shift 5  -> 10:5, 00000
    res |= rd;                                          //add rd        -> 10:1, 11
    res = (res & ~((0x1 << 11) | 0x1)) | ((res & 0x1) << 11); 
                                                        //move 11 over  -> 11:1, 0
    return res;
}

/* takes a pointer to the binary of an assembled instruction and returns the deassembled
 * instruction as a string
 */
const char *assembly_string(unsigned int *addr) {
    static char instruction[MAX_INSN_LEN];
    *instruction = '\0';

    static char immediate[MAX_IMM_LEN];
    *immediate = '\0';

    struct insn in = *(struct insn *)addr;
    int signBits = (in.func7 & 0x40)? ~0xfff : 0;
    switch (in.opcode) {
        case OP:                                //R
            cat_to_insn(OP_insn_names[in.func3 + (in.func7 >> 2)]);    //i put sub at 1000 and sra at 1101
        case OP32:
            if (in.opcode == OP32) {
                cat_to_insn(OP32_insn_names[in.func3 + (in.func7 >> 2)]);
            }
            cat_to_insn(" ");
            cat_to_insn(reg_names[in.rd]);
            cat_to_insn(",");
            cat_to_insn(reg_names[in.rs1]);
            cat_to_insn(",");
            cat_to_insn(reg_names[in.rs2]);
            break;
        case OP_IMM:
            if (in.func3 == 5 && in.func7 == 0b10000) {
                cat_to_insn(OP_IMM_insn_names[8]);                      //srai
            }
            else {
                cat_to_insn(OP_IMM_insn_names[in.func3]);
            }
        case OP_IMM_32:
            if (in.opcode == OP_IMM_32) {
                if (in.func3 == 5 && in.func7 == 0b10000) {
                    cat_to_insn(OP_IMM_32_insn_names[4]);                      //srai
                }
                else {
                    cat_to_insn(OP_IMM_32_insn_names[in.func3]);
                }
            }
            cat_to_insn(" ");
            cat_to_insn(reg_names[in.rd]);
            cat_to_insn(",");
            cat_to_insn(reg_names[in.rs1]);
            cat_to_insn(",");
            snprintf(immediate, MAX_IMM_LEN, "%d", signBits | (in.func7 & 0x7f) << 5 | in.rs2);
            cat_to_insn(immediate);
            break;
        case LOAD:
            if (in.opcode == LOAD) {
                cat_to_insn(LOAD_insn_names[in.func3]);
            }
            cat_to_insn(" ");
            cat_to_insn(reg_names[in.rd]);
            cat_to_insn(",");
            snprintf(immediate, MAX_IMM_LEN, "%d", signBits | (in.func7 & 0x7f) << 5 | in.rs2);
            cat_to_insn(immediate);
            cat_to_insn("(");
            cat_to_insn(reg_names[in.rs1]);
            cat_to_insn(")");
            break;
        case STORE:
            cat_to_insn(STORE_insn_names[in.func3]);
            cat_to_insn(" ");
            cat_to_insn(reg_names[in.rs2]);
            cat_to_insn(",");
            snprintf(immediate, MAX_IMM_LEN, "%d", signBits | (in.func7 & 0x7f) << 5 | in.rd);
            cat_to_insn(immediate);
            cat_to_insn("(");
            cat_to_insn(reg_names[in.rs1]);
            cat_to_insn(")");
            break;
        case BRANCH:
            cat_to_insn(BRANCH_insn_names[in.func3]);
            cat_to_insn(" ");
            cat_to_insn(reg_names[in.rs1]);
            cat_to_insn(",");
            cat_to_insn(reg_names[in.rs2]);
            cat_to_insn(",");
            snprintf(immediate, MAX_IMM_LEN, "%d", signBits | branchBitArrange(in.rd, in.func7));
            cat_to_insn(immediate);
            break;
        case AUIPC:
        case LUI:
            cat_to_insn((in.opcode == AUIPC)? "auipc " : "lui ");
            cat_to_insn(reg_names[in.rd]);
            cat_to_insn(",");
            signBits = (in.func7 & 0x40)? ~0xfffff : 0;
            snprintf(immediate, MAX_IMM_LEN, "%d", 
                     signBits | in.func7 << 13 | in.rs2 << 8 | in.rs1 << 3 | in.func3);
            cat_to_insn(immediate);
            break;
        case JALR:
            cat_to_insn("jalr ");
            cat_to_insn(reg_names[in.rd]);
            cat_to_insn(",");
            snprintf(immediate, MAX_IMM_LEN, "%d", signBits | (in.func7 & 0x3f) << 5 | in.rs2);
            cat_to_insn(immediate);
            cat_to_insn("(");
            cat_to_insn(reg_names[in.rs1]);
            cat_to_insn(")");
            break;
        case JAL:
            cat_to_insn("jal ");
            cat_to_insn(reg_names[in.rd]);
            cat_to_insn(",");
            signBits = (in.func7 & 0x40)? ~0xfffff : 0;
            snprintf(immediate, MAX_IMM_LEN, "%d", 
                     signBits | jalBitArrange(in.func3, in.rs1, in.rs2, in.func7));
            cat_to_insn(immediate);
            break;
        default:
            snprintf(instruction, MAX_INSN_LEN, "%08x", *addr);
    }
    return instruction;
}

/* helper function, turns an unsigned number value to an arbitrary base and
 * puts it in outstr
 */
void num_to_string(unsigned long num, int base, char *outstr) {
    if (!num) {
        *outstr = '0';
        outstr++;
    }
    else {
        char reverse[MAX_DIGITS];
        int index = 0;
        while (num) {
            char digit = num % base + '0';
            if (digit >= 10 + '0') {
                digit += 'a' - '0' - 10;
            }
            num /= base;
            reverse[index] = digit;
            index++;
        }
        while (index) {
            index--;
            *outstr = reverse[index];
            outstr++;    
        }
    }
    *outstr = '\0';
}

// constructs a formatted string with a va_list of arguments and puts it in a given buffer
int vsnprintf(char *buf, size_t bufsize, const char *format, va_list args) {
    if (!bufsize) {
        return bufsize;
    }
    *buf = '\0';
    int size = 0;
    int padSize = 0;
    int fieldWidth = 0;
    char padChar = ' ';        

    while (*format) {
        const char* toAdd;
        if (*format == '%') {
            format++;
            const char** endptr = &format;
            fieldWidth = strtonum(format, endptr);
            format = *endptr;
            switch (*format) {
                case '%':
                    toAdd = "%";
                    break;
                case 'c':
                    char arg[] = {va_arg(args, int), '\0'};
                    toAdd = arg;
                    break;
                case 's':
                    toAdd = va_arg(args, char*);
                    break;
                case 'd':
                    toAdd = decimal_string((long) va_arg(args, int));
                    break;
                case 'x':
                    toAdd = hex_string((long) va_arg(args, unsigned int));
                    padChar = '0';
                    break;
                case 'l':
                    format++;
                    if (*format == 'd') {
                        toAdd = decimal_string(va_arg(args, long));
                    }
                    else if (*format == 'x') {
                        toAdd = hex_string(va_arg(args, long));
                        padChar = '0';
                    }
                    else {
                        toAdd = "";
                    }
                    break;
                case 'p':
                    if (format[1] == 'I') {
                        format++;
                        unsigned int *addr = (unsigned int *) va_arg(args, void*);
                        toAdd = assembly_string(addr);
                    }
                    else {
                        size -= strlen(buf) - strlcat(buf, "0x", bufsize);
                        toAdd = hex_string((long) va_arg(args, void*));
                        fieldWidth = fieldWidth? fieldWidth : 8;
                        padChar = '0';
                    }
                    break;
                default:
                    toAdd = "";
            }
            padSize = fieldWidth - strlen(toAdd);
        }
        else {
            char temp[] = {*format, '\0'};
            toAdd = temp;
        }
        if (padSize > 0) {
            char* pad[padSize + 1];
            memset(pad, '\0', padSize + 1);
            size -= strlen(buf) - 
                strlcat(buf, memset(pad, padChar, padSize), bufsize);
        }
        size -= strlen(buf) - strlcat(buf, toAdd, bufsize);
        format++;
        padSize = 0;
        fieldWidth = 0;
        padChar = ' ';
    }
    return size;
}

/* constructs a formatted string with a variable number of arguments and 
 * puts it in a given buffer
 */
int snprintf(char *buf, size_t bufsize, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buf, bufsize, format, args);
    va_end(args);
    return length;
}

// ok to assume printf output is never longer that MAX_OUTPUT_LEN
#define MAX_OUTPUT_LEN 1024

// prints a formatted string from a variable number of arguments
int printf(const char *format, ...) {
    char strToPrint[MAX_OUTPUT_LEN];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(strToPrint, MAX_OUTPUT_LEN, format, args);
    va_end(args);
    uart_putstring(strToPrint);
    return length;
}

