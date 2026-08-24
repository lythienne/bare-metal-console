/* File: malloc.c
 * --------------
 * ***** TODO: add your file header comment here *****
 */


 /*
 * The code given below is simple "bump" allocator from lecture.
 * An allocation request is serviced by using sbrk to extend
 * the heap segment.
 * It does not recycle memory (free is a no-op) so when all the
 * space set aside for the heap is consumed, it will not be able
 * to service any further requests.
 *
 * This code is given here just to show the very simplest of
 * approaches to dynamic allocation. You will replace this code
 * with your own heap allocator implementation.
 */

#include "malloc.h"
#include "memmap.h"
#include "printf.h"
#include <stddef.h> // for NULL
#include "strings.h"
#include "backtrace.h"

// Macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two -- why?
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
#define FREE 0
#define USED 1
#define RZ_SIZE 4
#define HDR_SIZE 32
#define REDZONE 0xBB

void print_backtrace(frame_t *ptr) {
    frame_t frames[3];
    for (int i = 0; i < 3; i++) {
        frames[i] = (ptr)[i];
    }
    backtrace_print_frames(frames, 3);
}

int mashCharsTogether(char *ptr) {
    int output = 0;
    for (int i = 0; i < 4; i++) {
        output |= ptr[i] << i*8;
    }
    return output;
}

/*
 * Data variables private to this module used to track
 * statistics for debugging/validate heap:
 *    count_allocs, count_frees, total_bytes_requested
 */
static int count_allocs, count_frees, total_bytes_requested;

void report_damaged_redzone (void *ptr) {
    int *int_ptr = ptr;
    printf("\n=============================================\n");
    printf(  " **********  Mini-Valgrind Alert  ********** \n");
    printf(  "=============================================\n");
    printf("Attempt to free address %p that has damaged red zone(s): [%8x] [%8x]\n", 
           ptr, *(int_ptr + HDR_SIZE / 4), mashCharsTogether(((char *) int_ptr) + HDR_SIZE + RZ_SIZE + *int_ptr));
    printf("Block of size %d bytes (+ %d), allocated by\n", *int_ptr, HDR_SIZE + RZ_SIZE * 2);
    print_backtrace((frame_t *) ptr + 1);
}

/*
 * The segment of memory available for the heap runs from HEAP_START
 * to HEAP_MAX (markers placed in memmap.ld establish these boundaries,
 * constants declared in memmap.h)
 *
 * The pointer variable cur_head_end is initialized to HEAP_START and
 * is adjusted upward as in-use portion of heap segment enlarges.
 * Because cur_head_end is qualified as static, this variable
 * is not stored in stack frame, instead variable is located in data segment.
 * The one variable is shared by all and retains its value between calls.
 */

// Call sbrk to enlarge in-use heap area
void *sbrk(size_t nbytes) {
    static void *cur_heap_end = HEAP_START;     // IMPORTANT: declared static

    if (cur_heap_end == HEAP_START) {
        cur_heap_end = (char *) cur_heap_end + 4;
    }

    void *new_heap_end = (char *)cur_heap_end + nbytes; 
    if (new_heap_end > HEAP_MAX)    // if request would extend beyond heap max
        return NULL;                // reject
    void *prev_heap_end = cur_heap_end;
    cur_heap_end = new_heap_end;
    return prev_heap_end;
}

// offset by 2 bc of this function and malloc
void putBacktrace(frame_t* ptr) {
    frame_t frames[5];
    backtrace_gather_frames(frames, 5);
    for (int i = 2; i < 5; i++) {
        ((frame_t *) ptr)[i-2] = frames[i];
    }
}

void *malloc (size_t nbytes) {
    if (nbytes == 0) {
        return NULL;
    }
    int nbytesExpand = roundup(nbytes, 8) + HDR_SIZE + RZ_SIZE * 2;
    total_bytes_requested += nbytes;

    char* cur_heap_end = sbrk(0);
    char* ptr = HEAP_START;
    ptr += 4;
    int found = 0;
    while (!found && ptr < cur_heap_end) {
        int blkSize = roundup(*((int *) ptr), 8);
        if (blkSize >= ((int) nbytes) && ((int *) ptr)[1] == FREE) {
            int spaceLeft = blkSize - nbytesExpand;
            char *splitPtr = ptr + nbytesExpand;
            memset(ptr + HDR_SIZE + RZ_SIZE + nbytes, REDZONE, RZ_SIZE);
            if (spaceLeft >= 0 && splitPtr < cur_heap_end) {
                *((int *) splitPtr) = spaceLeft;
                putBacktrace(((frame_t *) splitPtr) + 1);
                memset(splitPtr + HDR_SIZE, REDZONE, RZ_SIZE);
                memset(splitPtr + HDR_SIZE + RZ_SIZE + spaceLeft, REDZONE, RZ_SIZE);
                free(splitPtr + HDR_SIZE + RZ_SIZE);
                count_frees--;
            }
            else if (spaceLeft > 0 - HDR_SIZE - RZ_SIZE * 2 && splitPtr < cur_heap_end){
                *((int *) splitPtr) = spaceLeft; 
                free(splitPtr + HDR_SIZE + RZ_SIZE);
                count_frees--;
            }
            found = 1;
        }
        else {
            ptr += blkSize + HDR_SIZE + RZ_SIZE * 2; 
        }
    }
    char *data = ptr;
    if (!found) {
        data = sbrk(nbytesExpand);
        if (data == NULL) {
            return data;
        }
        memset(data + HDR_SIZE, REDZONE, RZ_SIZE);
        memset(data + HDR_SIZE + RZ_SIZE + nbytes, REDZONE, RZ_SIZE);
    }
    ((int *) data)[0] = nbytes;
    ((int *) data)[1] = USED;
    putBacktrace(((frame_t *) data) + 1);

    count_allocs++;
    return data + HDR_SIZE + RZ_SIZE;
}

void free (void *ptr) {
    if (ptr == NULL) {
        return;
    }
    count_frees++;

    int* cur_heap_end = sbrk(0);
    int* free_ptr = (int *) (((char *) ptr) - HDR_SIZE - RZ_SIZE);
    free_ptr[1] = FREE;
    if (*free_ptr >= 0 && (free_ptr[HDR_SIZE / 4] != 0xBBBBBBBB ||
        mashCharsTogether(((char *) free_ptr) + HDR_SIZE + RZ_SIZE + *free_ptr) != 0xBBBBBBBB)) {
        report_damaged_redzone(free_ptr);
    }
    unsigned int nbytesFree = roundup(*free_ptr, 8);
    free_ptr += (roundup(*free_ptr, 8) + HDR_SIZE + RZ_SIZE * 2) / 4;
    while (free_ptr < cur_heap_end && free_ptr[1] == FREE) {
        nbytesFree += roundup(*free_ptr, 8) + HDR_SIZE + RZ_SIZE * 2;
        free_ptr += (roundup(*free_ptr, 8) + HDR_SIZE + RZ_SIZE * 2) / 4;
    }

    *((int *) ptr) = nbytesFree;
}

void heap_dump (const char *label) {
    void *cur_heap_end = sbrk(0);
    printf("\n---------- HEAP DUMP (%s) ----------\n", label);
    
    int *ptr = HEAP_START;
    ptr += 1;
    printf("Heap segment at %p - %p\n", ptr, cur_heap_end);
    int index = 1;
    while ((unsigned long) ptr < (unsigned long) cur_heap_end) {
        if (ptr[1] && ptr[1] != USED) {
            printf("Not a header at %p, code is %d\n", ptr, ptr[1]);
        }
        printf(" - Block %d (@ %p): %4d bytes (+%d), %s\n", 
                index, (void *) (ptr + (HDR_SIZE + RZ_SIZE) / 4), 
                ptr[0], HDR_SIZE + RZ_SIZE * 2, ptr[1]? "in use" : "free");
        ptr += (roundup(*ptr, 8) + HDR_SIZE + RZ_SIZE * 2) / 4;
        index++;
    }

    printf("----------  END DUMP (%s) ----------\n", label);
    printf("Stats: %d in-use (%d allocs, %d frees), %d total payload bytes requested\n\n",
        count_allocs - count_frees, count_allocs, count_frees, total_bytes_requested);
}

void malloc_report (void) {
    printf("\n=============================================\n");
    printf(  "         Mini-Valgrind Malloc Report         \n");
    printf(  "=============================================\n");
    printf("final stats: %d allocs, %d frees, %d total payload bytes requested\n", 
            count_allocs, count_frees, total_bytes_requested);
    int *ptr = HEAP_START;
    ptr += 1;
    int *cur_heap_end = sbrk(0);
    int n = 0;
    int sum = 0;
    while (ptr < cur_heap_end) {
        if (ptr[1] == USED) {
            printf("\n%d bytes are lost, allocated by\n", *ptr);
            print_backtrace((frame_t *) ptr + 1);
            n++;
            sum += *ptr;
        }
        ptr += (roundup(*ptr, 8) + HDR_SIZE + RZ_SIZE * 2) / 4;
    }
    printf("\nLost %d total bytes in %d blocks\n", sum, n);
}

