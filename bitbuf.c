#include <stdio.h>
#include "bitbuf.h"

/** Flush a bit buffer out to a file
 *
 * Note: This function does not flush the file.
 *
 * @param bits pointer to a bit buffer
 * @param out file to which to flush bits
 */
int flush_bitbuf(struct bitbuf *bits, FILE *out)
{
    size_t retv, n;

    n = bits->pos / bits->unit;
    retv = fwrite(&(bits->bs), bits->unit / 8, n, out);
    bits->pos = 0;
    if (retv < n) {
        warn(sprintf("WARNING: Only %i of %i bytes written out\n", retv, n));
        return -1;
    } else {
        return 0;
    }
}

/** Fill a bit buffer
 *
 * This function calls flush_bitbuf() if needed.
 *
 * @param x int whose low-order bits are to be added to the buffer
 * @param entropy number of low-order bits of x to be added to the buffer
 * @param bits pointer to bit buffer
 * @param out file to use when flushing the buffer
 * @return 0 on success, -1 on failure
 */
int fill_bitbuf(unsigned int x, unsigned int entropy,
                struct bitbuf *bits, FILE *out)
{
    unsigned int i, j;

    /* Check input */
    if (bits->size % bits->unit != 0 ||
        bits->pos >= bits->size)
        return -1;
    if (entropy > sizeof(int) * 8)
        return -1;
    /* Flush buffer if there isn't space to add entropy bits */
    if (bits->pos >= bits->size - entropy)
        if (flush_bitbuf(bits, out) != 0)
            return -1;
    /* Add bits to buffer */
    i = bits->pos / bits->unit;
    j = bits->pos % bits->unit;
    if (j == 0)
        bits->bs[i] = 0;    /* Zero out storage */
    bits->bs[i] |= x << j;
    if (entropy > bits->unit - j) {
        bits->bs[i+1] = 0;  /* Zero out storage */
        bits->bs[i+1] |= x >> (bits->unit - j);
    }
    bits->pos += entropy;
    return 0;
}
