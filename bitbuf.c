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

    n = bits->size / bits-> unit;
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
 * This function calls flush_bitbuf() if needed. Nevertheless, the buffer may
 * be full when fill_bitbuf() returns.
 *
 * @param x int whose low-order bits are to be added to the buffer
 * @param entropy number of low-order bits of x to be added to the buffer
 * @param bits pointer to bit buffer
 * @param out file to use when flushing the buffer
 */
int fill_bitbuf(unsigned int x, unsigned int entropy,
                struct bitbuf *bits, FILE *out)
{
    unsigned int i, j, k;

    if (bits->pos >= bits->size)
        return -1;
    if (bits->pos > bits->size - entropy)
        flush_bitbuf(bits, out);
    i = bits->pos / bits->unit;
    j = bits->pos % bits->unit;
    k = j + entropy - bits->unit;
    if (j == 0)
        bits->bs[i] = 0;
    if (k <= 0) {
        /* cast k to unsigned */
        bits->bs[i] |= x << j;
        bits->pos += entropy;
    } else {
        bits->bs[i] |= (x >> k) << j;
        bits->pos += entropy - k;
        fill_bifbuf(x % (2^k), k, bits, out);
    }
    return 0;
}
