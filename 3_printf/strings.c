/* File: strings.c
 * ---------------
 * Simple C library with the following string operations:
 * memcpy, memset, strlen, strcmp, strlcat, and strtonum
 *
 * Author: Harrison Chen
 * Version 1/29/26
 */
#include "strings.h"

// memcpy takes the first n bytes from src and copies them to dst
void *memcpy(void *dst, const void *src, size_t n) {
    /* Copy contents from src to dst one byte at a time */
    char *d = dst;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

// memset takes the lowest byte at val and copies it n times into dst
void *memset(void *dst, int val, size_t n) {
    char *d = dst;
    for (int i = 0; i < n; i++) {
        *d = val;
        d++;
    }
    return dst;
}

/* strlen returns the count of bytes from str until the null terminator
 * @precondition str should be null terminated
 */
size_t strlen(const char *str) {
    /* Implementation a gift to you from lab3 */
    size_t n = 0;
    while (str[n] != '\0') {
        n++;
    }
    return n;
}

/* strcmp lexically compares two strings
 * @precondition s1 and s2 should be null terminated
 * @return       positive if s1 > s2, 0 if equal, negative if s1 < s2
 */
int strcmp(const char *s1, const char *s2) {
    int n1 = 0;
    int n2 = 0;
    while (s1[n1] != '\0' || s2[n2] != '\0') {
        if (s1[n1] < s2[n2]) {
            return -1;
        }
        else if (s1[n1] > s2[n2]) {
            return 1;
        }
        n1++;
        n2++;
    }
    return 0;
}

/* strlcat copies the string at src after the string at dst. Cuts off 
 * string at dstsize and inserts the null terminator. Does nothing if
 * dst has no null terminator within dstsize.
 *
 * @return  length of src string + length of dst string or 
 *          length of src string + dstsize if dst string does not terminate
 */
size_t strlcat(char *dst, const char *src, size_t dstsize) {
    size_t len = strlen(dst);
    if (len < dstsize) {
        size_t remaining = dstsize - len - 1;
        memcpy(dst + len, src, remaining);
        dst[dstsize - 1] = '\0';
        return len + strlen(src);
    }
    return dstsize + strlen(src);
}

/* strtonum returns the number at the beginning of a string at str.
 * Supports both hex "0x..." and decimal "...", stops reading number
 * at null terminator or non-number character.
 * If endptr is not NULL, points *endptr to the character after the number.
 *
 * @precondition string at str is null terminated
 * @return       number at beginning of string or 0 if no number
 */
unsigned long strtonum(const char *str, const char **endptr) {
    unsigned int result = 0;
    int isLegalChar = 1;
    int isHex = (str[0] == '0' && str[1] == 'x');
    const char* curr = str + isHex * 2;
    while (isLegalChar && *curr != '\0') {
        result *= isHex * 6 + 10;                           //isHex is 1 or 0
        if (*curr >= '0' && *curr <= '9') {
            result += *curr - '0';
        }
        else if (isHex && (*curr >= 'a' && *curr <= 'f')) {
            result += *curr - 'a' + 10;                     //0xa = 10
        }
        else if (isHex && (*curr >= 'A' && *curr <= 'F')) {
            result += *curr - 'A' + 10;                     //0xA = 10
        }
        else {
            isLegalChar = 0;
            result /= isHex * 6 + 10;
        }
        curr += isLegalChar;                                //isLegalChar is 1 or 0
    }
    if (endptr != NULL) {
        *endptr = (*--curr == 'x')? curr : curr + 1;
    }
    return result;
}
