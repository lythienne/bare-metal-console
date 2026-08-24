/* File: test_backtrace_malloc.c
 * -----------------------------
 * TODO: add your file header comment here
 */
#include "assert.h"
#include "backtrace.h"
#include "malloc.h"
#include "printf.h"
#include <stdint.h>
#include "strings.h"
#include "timer.h"
#include "uart.h"

void heap_dump(const char *label); // available in malloc.c but not public interface

static int recursion(int n) {
    printf("\nEnter call recursion(%d):\n", n);
    backtrace_print();
    if (n == 0) {
        return 0;
    } else if (n % 2 == 0) {  // even
        return 2 * recursion(n-1);
    } else {                   // odd
        return 1 + recursion(n-1);
    }
}

static void show_frames(int nframes) {
    frame_t f[nframes];
    printf("\nEnter call show_frames(%d):\n", nframes);
    int frames_filled = backtrace_gather_frames(f, nframes);

    assert(frames_filled <= nframes);
    printf("Gathered backtrace contains %d frames:\n", frames_filled);
    backtrace_print_frames(f, frames_filled);
    printf("\n");
}

static void silly_goose(int nframes) {
    show_frames(nframes);
}

static void happy_day(void) {
    silly_goose(2);
    silly_goose(6);
    show_frames(3);
}

static void test_backtrace(void) {
    happy_day();
    recursion(4);
}

static void mars(void) {
    // no code calls this function. It should never execute
    uart_putstring("saying hello from mars.  How did execution get here?\n");
}

static void good_egg(void *val) {
    // well-behaved function, stack canary will be intact at function end
    void *array[3];

    for (int i = 0; i < 3; i++) {
        array[i] = val;
    }
    backtrace_print();
    printf("good_egg correctly used stack array at addr %p\n", array);
}

static void bad_guy(int num_beyond, void *val) {
    // WARNING: buggy function writes beyond its stack buffer
    // stack canary will be overwritten, stack smash will be detected at function end
    int size = 3;
    void *array[size];

    for (int i = 0; i < size + num_beyond; i++) {
        array[i] = val;
    }
    printf("bad_guy wrote %d value(s) past end of stack array at addr %p\n", num_beyond, array);
}

void test_stack_protector(void) {
    // call to bad_guy function will write past end of stack buffer
    // If StackGuard is enabled, should halt and report stack smashing
    // If StackGuard not enabled, bad_guy will corrupt stack, consequence differs
    // based on which part of housekeeping data on stack was overwritten and
    // what value was written there

    printf("\nTesting stack protector, be sure that stack-protector is enabled in Makefile.\n");
    void *val = 0x0;
    // val = (void *)mars;      // try different value to see change in consequence
    // val = (void *)0x10000;

    good_egg(val);
    for (int count = 1; count < 5; count++) {
        printf("\ncall bad_guy(%d) ...\n", count);
        bad_guy(count, val);
        printf("... survived bad_guy(%d).\n", count);
    }
}

static void test_heap_dump(void) {
    heap_dump("Empty heap");

    int *p = malloc(sizeof(int));
    *p = 0;
    heap_dump("After p = malloc(4)");

    char *q = malloc(16);
    memcpy(q, "aaaaaaaaaaaaaaa", 16);
    heap_dump("After q = malloc(16)");

    free(p);
    heap_dump("After free(p)");

    free(q);
    heap_dump("After free(q)");
}

static void test_recycle(void) {
    printf("Test Recycle");
    char *arr = malloc(32);
    for (int i = 0; i < 5; i++) {
        heap_dump("After malloc(32)");
        free(arr);
        heap_dump("After free");
        arr = malloc(32);
    }
    free(arr);

    char *tiny = malloc(1);
    heap_dump("After malloc(1)");
    char *small[4];
    for (int i = 0; i < 4; i++) {       //should be enough to have to expand heap once
        small[i] = malloc(7);           //some of these should make 0 byte blocks (which will get coalesced)
        heap_dump("After malloc(7)");   //and the first split (of the 16) should coalesce to the 32 byte block
    }
    for (int i = 0; i < 4; i++) {
        free(small[i]);
    }
    free(tiny);
    heap_dump("After final free");
}

static void test_coalesce(void) {
    printf("Test coalesce\n");
    char *small[12];
    for (int i = 0; i < 12; i++) {
        small[i] = malloc(5);
    }
    heap_dump("After malloc-ing 12 small blocks");
    for (int i = 11; i >= 0; i--) {
        free(small[i]);
        heap_dump("After freeing a small block");
    }
    char *big = malloc(180);
    heap_dump("After malloc-ing large block");
    free (big);
}

static void test_heap_simple(void) {
    printf("Test Heap simple\n");
    // allocate a string and array of ints
    // assign to values, check, then free
    const char *alphabet = "abcdefghijklmnopqrstuvwxyz";
    int len = strlen(alphabet);

    char *str = malloc(len + 1);
    memcpy(str, alphabet, len + 1);

    int n = 10;
    int *arr = malloc(n*sizeof(int));
    for (int i = 0; i < n; i++) {
        arr[i] = i;
    }

    assert(strcmp(str, alphabet) == 0);
    free(str);
    assert(arr[0] == 0 && arr[n-1] == n-1);
    free(arr);
}

static void test_heap_oddballs(void) {
    printf("Test Heap Oddballs\n");
    // test oddball cases
    char *ptr;

    ptr = malloc(900000000); // request too large to fit
    assert(ptr == NULL); // should return NULL if cannot service request
    heap_dump("After reject too-large request");

    ptr = malloc(0); // legal request, though weird
    heap_dump("After malloc(0)");
    free(ptr);

    free(NULL); // legal request, should do nothing
    heap_dump("After free(NULL)");
}

static void test_heap_multiple(void) {
    // array of dynamically-allocated strings, each
    // string filled with repeated char, e.g. "A" , "BB" , "CCC"
    // Examine each string, verify expected contents intact.
    printf("Test Heap Multiple\n");

    int n = 8;
    char *arr[n];

    for (int i = 0; i < n; i++) {
        int num_repeats = i + 1;
        char *ptr = malloc(num_repeats + 1);
        memset(ptr, 'A' - 1 + num_repeats, num_repeats);
        ptr[num_repeats] = '\0';
        arr[i] = ptr;
    }
    heap_dump("After all allocations");
    for (int i = n-1; i >= 0; i--) {
        int len = strlen(arr[i]);
        char first = arr[i][0], last = arr[i][len -1];
        assert(first == 'A' - 1 + len);  // verify payload contents
        assert(first == last);
        free(arr[i]);
    }
    heap_dump("After all frees");
}

static void test_heap_leaks(void) {
    // This function allocates blocks which are never freed.
    // Leaks will be reported if doing the Valgrind extension, but otherwise
    // they are harmless/silent
    printf("Test Heap Leaks\n");
    char *ptr;

    ptr = malloc(9); // leaked
    ptr = malloc(5);
    free(ptr);
    ptr = malloc(107); // leaked
    malloc_report();
}

void test_heap_redzones(void) {
    // DO NOT ATTEMPT THIS TEST unless your heap has red zone protection!
    printf("Test Heap Redzones\n");
    char *ptr;

    ptr = malloc(9);
    memset(ptr, 'a', 9); // write into payload
    free(ptr); // ptr is OK
    //heap_dump("after 9");

    ptr = malloc(5);
    ptr[-1] = 0x45; // write before payload
    free(ptr);      // ptr is NOT ok
    //heap_dump("after 5");

    ptr = malloc(12);
    ptr[13] = 0x45; // write after payload
    free(ptr);      // ptr is NOT ok
    //heap_dump("after 12");
}

void main(void) {
    uart_init();
    uart_putstring("Start execute main() in test_backtrace_malloc.c\n");

    test_backtrace();
    //test_stack_protector(); // Selectively uncomment when ready to test this

    test_heap_dump();
    test_recycle();
    /*test_coalesce();
    test_heap_simple();
    test_heap_oddballs();
    test_heap_multiple();*/
    test_heap_leaks();

    test_heap_redzones(); // DO NOT USE unless you have implemented red zone protection!
    uart_putstring("\nSuccessfully finished executing main() in test_backtrace_malloc.c\n");
}
