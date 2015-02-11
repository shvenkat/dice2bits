#ifndef _bitbuf_h
#define _bitbuf_h

#define BUFSIZE 32    /* a small positive integer */

struct bitbuf {
    unsigned int bs[BUFSIZE];    /* bit storage */
    size_t  pos;                 /* position index in bits */
    size_t  size;                /* size in bits */
    size_t  unit;                /* int size in bits */
};

int flush_bitbuf(struct bitbuf *bits, FILE *out);

int fill_bitbuf(unsigned int x, unsigned int n,
                struct bitbuf *bits, FILE *out);

#endif
