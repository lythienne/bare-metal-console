/* File: test_strings_printf.c
 * ---------------------------
 * Tests for the strings.c library and the printf.c library
 * 
 * Author: Harrison
 * Version: 2/1/26
 */
#include "assert.h"
#include "printf.h"
#include <stddef.h>
#include "strings.h"
#include "uart.h"

// Prototypes copied from printf.c to allow unit testing of helper functions
void num_to_string(unsigned long num, int base, char *outstr);
const char *hex_string(unsigned long val);
const char *decimal_string(long val);


static void test_memset(void) {
    char buf[25];
    size_t bufsize = sizeof(buf);

    memset(buf, 0x7e, bufsize);
    for (int i = 0; i < bufsize; i++)
        assert(buf[i] == 0x7e);

    memset(buf, 0x1ab, 7);          //set only part of buffer                                         
    for (int i = 0; i < bufsize; i++)
        assert(buf[i] == ((i < 7)? 0xab : 0x7e));

    memset(buf, 0xc, 1);            //set one character
    for (int i = 0; i < bufsize; i++)
        assert(buf[i] == ((i < 1)? 0x0c : ((i < 7)? 0xab : 0x7e)));

    memset(buf, 0x8e, 0);         //set no characters
    for (int i = 0; i < bufsize; i++)
        assert(buf[i] == ((i < 1)? 0xc : ((i < 7)? 0xab : 0x7e)));

    memset(buf, 0, bufsize);        //check memset doesn't write outside
    memset(buf+8, 0xfa, bufsize - 16);
    for (int i = 0; i < bufsize; i++) {
        assert(buf[i] == ((i < 8 || i >= 17)? 0 : 0xfa));
    }
}

static void test_strcmp(void) {
    assert(strcmp("me", "me") == 0);
    assert(strcmp("me", "you") != 0);

    assert(strcmp("", "") == 0);            //empty strings are equal
    assert(strcmp("a","b") < 0);            //test comparisons
    assert(strcmp("b","a") > 0);
    assert(strcmp("a","") > 0);             //compare to empty strings
    assert(strcmp("","a") < 0); 
    assert(strcmp("aa","a") > 0);           //compare different length equal strings
    assert(strcmp("a","aa") < 0);
    //compare very big strings
    assert(strcmp("abcdefghijklmnopqrstuvwxyz","abcdefghijklmnopqrstuvwxyz") == 0);
    //test other characters
    assert(strcmp("*      "," ~~~~~~~~") > 0);
}

static void test_strlcat(void) {
    char buf[20];
    size_t bufsize = sizeof(buf);
    // as aid for debugging, fill contents of buffer with repeat value 0x7e
    // rather than leave contents uninitialized, this makes it easier to
    // spot later if contents have not changed in the way you intend
    memset(buf, 0x7e, bufsize); // for debug, init array contents with fixed repeat value

    buf[0] = '\0'; // null at first index makes empty string
    assert(strcmp(buf, "") == 0);
    strlcat(buf, "CS", bufsize); // append CS
    assert(strcmp(buf, "CS") == 0);
    strlcat(buf, "107e", bufsize); // append 107e
    assert(strcmp(buf, "CS107e") == 0);

    strlcat(buf, "", bufsize);              //append empty string
    assert(strcmp(buf, "CS107e") == 0);
    //bigger than bufsize
    assert(strlcat(buf, "abcdefghijklmnopqrstuvwxyz", bufsize) == 6 + 26);
    assert(strcmp(buf, "CS107eabcdefghijklm") == 0);
    assert(strlen(buf) == bufsize - 1);                 //null terminated
    assert(strlcat(buf, "a", 10) == 10 + 1);
    assert(strcmp(buf, "CS107eabcdefghijklm") == 0);    //no change
}

static void test_strtonum(void) {
    assert(strtonum("67", NULL) == 67);

    const char *input = "107rocks";
    const char *rest = NULL;
    long val = strtonum(input, &rest);
    assert(val == 107);
    assert(rest == &input[3]);  // rest should now point to first non-digit character

    const char *hex = "0x02a";
    assert(strtonum(hex, NULL) == 0x02a);       //test hex
    const char *hex2 = "0x02A";
    assert(strtonum(hex2, NULL) == 0x02a);      //test capital hex
    const char *input2 = "0001";
    assert(strtonum(input2, NULL) == 1);        //removes leading 0s
    const char *input3 = "x01";
    assert(strtonum(input3, &rest) == 0);       //starts with illegal char
    assert(rest == input3);
    const char *input4 = "";
    assert(strtonum(input4, NULL) == 0);        //test empty string
    const char *input5 = "0x23ADE 67";
    assert(strtonum(input5, NULL) == 0x23ade);  //test space is illegal
    const char *input6 = "01";
    assert(strtonum(input6, &rest) == 1);       //test endptr
    assert(rest == &input6[2]);
    
    printf("%10p", (void *) strtonum("0x90abcdef", NULL));
    assert(strtonum("0x90abcdef", NULL) == 0x90abcdef);
}

static void test_helpers(void) {
    char buf[32];
    size_t bufsize = sizeof(buf);
    memset(buf, 0x7e, bufsize); // for debug, init array contents with fixed repeat value

    num_to_string(45, 10, buf);
    assert(strcmp(buf, "45") == 0);
    num_to_string(45, 16, buf);
    assert(strcmp(buf, "2d") == 0);

    assert(strcmp(decimal_string(-88), "-88") == 0);
    assert(strcmp(hex_string(0x107e), "107e") == 0);
    
    num_to_string(0, 10, buf);          //test 0
    assert(strcmp(buf, "0") == 0);

    num_to_string(0, 16, buf);
    assert(strcmp(buf, "0") == 0);

    assert(strcmp(decimal_string(-0), "0") == 0);       //-0 is 0
    assert(strcmp(hex_string(-0), "0") == 0);

    assert(strcmp(decimal_string(-1234567890), "-1234567890") == 0);        //can do long negative

    assert(strcmp(hex_string(0xffffffffffffffff), "ffffffffffffffff") == 0);    //big numbers
    assert(strcmp(hex_string(0x0123456789abcdef0), "123456789abcdef0") == 0);
}

static void test_snprintf(void) {
    char buf[100];
    size_t bufsize = sizeof(buf);
    memset(buf, 0x7e, bufsize); // for debug, init array contents with fixed repeat value
    
    // bufSize 0
    assert(snprintf(buf, 0, "asdflkdshklflkdshflkshlk") == 0);

    // No formatting codes
    assert(snprintf(buf, bufsize, "Hello, world!") == 13);
    assert(strcmp(buf, "Hello, world!") == 0);

    // One string formatting code
    assert(snprintf(buf, bufsize, "%s", "binky") == 5);
    assert(strcmp(buf, "binky") == 0);

    // One char formatting code
    assert(snprintf(buf, bufsize, "binky%c", 'c') == 6);
    assert(strcmp(buf, "binkyc") == 0);

    // Mixed formatting code
    assert(snprintf(buf, bufsize, "LS%cU = 100%% %s!", 'J', "fresh") == 18);
    assert(strcmp(buf, "LSJU = 100% fresh!") == 0);

    // Mixed formatting code with small buffer
    assert(snprintf(buf, 10, "LS%cU = 100%% %s!", 'J', "fresh") == 18);
    assert(strcmp(buf, "LSJU = 10") == 0);

    // One decimal, hex, long decimal, long hex
    assert(snprintf(buf, bufsize, "ten: %d", 10) == 7);
    assert(strcmp(buf, "ten: 10") == 0);

    assert(snprintf(buf, bufsize, "ten (hex): %x", 0x10) == 13);
    assert(strcmp(buf, "ten (hex): 10") == 0);

    assert(snprintf(buf, bufsize, "4 bil: %ld", 4000000000) == 17);
    assert(strcmp(buf, "4 bil: 4000000000") == 0);

    assert(snprintf(buf, bufsize, "big hex: %lx", 0xabcdefabc) == 18);
    assert(strcmp(buf, "big hex: abcdefabc") == 0);

    // Mixed formatting chars, strings, and numbers
    snprintf(buf, bufsize, "%ld%%%lx %c %d%s%x", 
            5234567890, 0x100000000, '=', 939600594, " or ", 0x38012AD2);
    assert(strcmp(buf, "1234567890%100000000 = 939600594 or 38012ad2"));

    // fieldWidth
    assert(snprintf(buf, bufsize, "%10d%10x", 67, 0x67) == 20);
    assert(strcmp(buf, "        670000000067") == 0);

    // One pointer
    assert(snprintf(buf, bufsize, "%p", (unsigned long *) 0x02000040) == 10);
    assert(strcmp(buf, "0x02000040") == 0);

    // Mixed formatting everything
    snprintf(buf, bufsize, "%%%c%s%d%x%ld%lx%10x%p", ' ', "hi", 9, 15, 
            4000000000, 0x123456789, 1, (unsigned long *) 0x30);
    assert(strcmp(buf, "% hi9f400000000012345678900000000010x00000030") == 0);

    // Overload buffer with everything after
    snprintf(buf, bufsize, "%100x%%%c%s%d%x%ld%lx%10x%p", 1, ' ', "hi", 9, 15, 
            4000000000, 0x123456789, 1, (unsigned long *) 0x30);
    char zeroes[100];
    memset(zeroes, '0', 99);
    zeroes[99] = '\0';
    assert(strcmp(buf, zeroes) == 0);

    // Padding width starts with a 0
    assert(snprintf(buf, bufsize, "hex number: %08x yay!", 0xabcdef) == 25);
    assert(strcmp(buf, "hex number: 00abcdef yay!") == 0);

    // Padding width is 0
    assert(snprintf(buf, bufsize, "hex number: %0x yay!", 0xabcdef) == 23);
    assert(strcmp(buf, "hex number: abcdef yay!") == 0);

    // Padding on %, chars, strings
    assert(snprintf(buf, bufsize, "%8c%8s", '@', "banana") == 16);
    assert(strcmp(buf, "       @  banana") == 0);

    // Alt padding on ptr
    assert(snprintf(buf, bufsize, "%16p", (void *) 0xabcdef) == 18);
    assert(strcmp(buf, "0x0000000000abcdef") == 0);
}


// This function just here as code to disassemble for extension
int sum(int n) {
    int result = 6;
    for (int i = 0; i < n; i++) {
        result += i * 3;
    }
    return result + 729;
}

void test_disassemble(void) {
    const unsigned int add =  0x00f706b3;
    const unsigned int xori = 0x0015c593;
    const unsigned int bne =  0xfe061ce3;
    const unsigned int sd =   0x02113423;
    const unsigned int j =    0x0000006f;

    // formatting code %pI accesses the disassemble extension.
    // If extension not implemented, regular version of printf
    // will simply output pointer address followed by I
    // e.g.  "... disassembles to 0x07ffffd4I"
    printf("Encoded instruction %08x disassembles to %pI\n", add, &add);
    printf("Encoded instruction %08x disassembles to %pI\n", xori, &xori);
    printf("Encoded instruction %08x disassembles to %pI\n", bne, &bne);
    printf("Encoded instruction %08x disassembles to %pI\n", sd, &sd);
    printf("Encoded instruction %08x disassembles to %pI\n", j, &j);

    unsigned int *fn = (unsigned int *)sum; // disassemble instructions from sum function
    for (int i = 0; i < 10; i++) {
        printf("%p:  %08x  %pI\n", &fn[i], fn[i], &fn[i]);
    }
}

void main(void) {
    uart_init();
    uart_putstring("Start execute main() in test_strings_printf.c\n");

    test_memset();
    test_strcmp();
    test_strlcat();
    test_strtonum();
    test_helpers();
    test_snprintf();
    
    //testing large negative immediates
    const unsigned int i = 0xaaafbf13;      //sltiu t5,t6,-1366
    const unsigned int s = 0xab49a4a3;      //sw s4,-1367(s3)
    const unsigned int b = 0xab6ae563;      //bltu s5,s6,-3414
    const unsigned int u = 0xaaaaa117;      //auipc sp,-349526
    const unsigned int j = 0xaabaa0ef;      //jal ra,-349526
    printf("---Negative large immediates---\nI: %pI\nS: %pI\nB: %pI\nU: %pI\nJ: %pI\n", 
           &i, &s, &b, &u, &j);

    //testing large positive immediates
    const unsigned int ip = 0x5db23083;     //ld ra,1499(tp)
    const unsigned int sp = 0x4fb182a3;     //sb s11,1253(gp) 
    const unsigned int bp = 0x67adf963;     //bgeu s11,s10,1650
    const unsigned int up = 0x45654c37;     //lui s8,284244
    const unsigned int jp = 0x566b8b67;     //jalr s6,1382(s7)
    printf("---Positive large immediates---\nI: %pI\nS: %pI\nB: %pI\nU: %pI\nJ: %pI\n", 
           &ip, &sp, &bp, &up, &jp);

    // check printf works
    /*printf("percent:%%\nchar:%c\nstring:%s\ndec:%d\nhex:%x\n", '*', "[]", -67, 0x67);
    printf("long dec:%ld\nlong hex:%lx\npointer:%p\npadding:%20d\n", 
            -3234567890, 0x543212345, (unsigned long *) 0x67, -67);
    */

    test_disassemble(); // uncomment if you implement extension

    uart_putstring("Successfully finished executing main() in test_strings_printf.c\n");
}
