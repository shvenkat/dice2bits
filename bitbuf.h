#include <stdio.h>

#define BUFSIZE 32    /* Should be a small positive integer */

struct bitbuf {
    unsigned int[BUFSIZE] bs;
    size_t  pos;
    size_t  size;
    size_t  unit;
};

int flush_bitbuf(struct bitbuf *bits, FILE *out)

int fill_bitbuf(unsigned int x, unsigned int n
                struct bitbuf *bits, FILE *out)
