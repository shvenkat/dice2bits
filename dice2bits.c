# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <assert.h>
# include <termios.h>
# include <errno.h>

#ifndef TCSASOFT
#define TCSASOFT 0
#endif

#define BUFSIZE 32    /* Should be a small positive integer */

// TODO: Lock memory
// TODO: Prevent other process from attaching to or reading memory
// TODO: Prevent hardware from reading process memory
// TODO: Lock files, see getpass

void warn(char const *msg)
{
    fputs(msg, stderr);
}

void error(char const *msg)
{
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}

/** Prepare the terminal by turning off echoing
 *
 * @param in tty file descriptor
 * @param prev_tattr previous terminal attributes used to restore the terminal
 */
int config_tty(FILE *in, struct termios *prev_tattr)
{
    struct termios t;

    if (tcgetattr(fileno(in), prev_tattr) == 0) {
        t = *prev_tcattr;
        t.c_lflag &= ~(ECHO|ISIG);
        if (tcsetattr(fileno(in), TCSAFLUSH|TCSASOFT, &t) == 0)
            return 0;
        else
            return -1;
    } else {
        return -1;
    }
}

/** Restore the terminal
 *
 * @param prev_tattr terminal attributes to be restored
 */
int restore_tty(FILE *in, struct termios const *prev_tattr)
{
    return tcsetattr(fileno(in), TCSAFLUSH|TCSASOFT, prev_tattr);
}

/** Convert one ASCII digit representing a dice roll to an integer
 *
 * To generate random bits, 2 of the 6 possible outcomes are discarded as
 * invalid, with the remaining 4 providing 2 bits of entropy.
 *
 * @param top one of the characters '1'..'6'
 * @param *x random bits
 * @param *entropy number of random low-order bits in x
 * @return 0 indicating a valid roll, -1 indicating an invalid roll
 */
int roll_int_discard(char top, unsigned int *x, unsigned int *entropy)
{
    int t;

    t = (int) (top - '0');
    if (t <= 1 || t >= 6) {
        *x = 0;
        *entropy = 0;
        return -1;                /* reduce sample space to a power of 2 */
    }
    t -= 2;
    assert(t >= 0 && t < 4);
    *x = (unsigned int)t;
    *entropy = 2;
    return 0;
}

/** Convert one ASCII digit representing a dice roll to an integer
 *
 * To generate random bits, the following map is used:
 *     Roll  Bits  Entropy
 *        1     0        1
 *        2    00        2
 *        3    10        2
 *        4    11        2
 *        5    01        2
 *        6     1        1
 * @param top one of the characters '1'..'6'
 * @param *x random bits
 * @param *entropy number of random low-order bits in x
 * @return 0 on success, -1 on failure
 */
int roll_int_greedy(char top, unsigned int *x, unsigned int *entropy)
{
    int t;

    t = (int) (top - '0');
    switch(t) {
        case 1:
            *x = 0; *entropy = 1; return 0;
        case 2:
            *x = 0; *entropy = 2; return 0;
        case 3:
            *x = 2; *entropy = 2; return 0;
        case 4:
            *x = 3; *entropy = 2; return 0;
        case 5:
            *x = 1; *entropy = 2; return 0;
        case 6:
            *x = 1; *entropy = 1; return 0;
        default:
            *x = 0; *entropy = 0; return -1;
    }
}

/** Convert two ASCII digits representing a dice roll to a single integer
 *
 * When a six-sided die is shaken or cast inside a box and allowed to settle
 * along one edge, there are 24 possible orientations of the die. Each outcome
 * is specified by the numbers on the top and side faces, those opposite the
 * faces in contact with the box. To generate random bits from a roll, 8
 * outcomes are discarded as invalid, and the remaining 16 provide 4 bits of
 * entropy.
 *
 * @param top one of the characters '1'..'6'
 * @param side one of four characters among '1'..'6' (the top and bottom
 *          numbers being invalid on a side face)
 * @param *x random bits
 * @param *entropy number of random low-order bits in x
 * @return 0 on success, -1 on failure
 */
int eroll_int_discard(char top, char side, unsigned int *x,
                      unsigned int *entropy)
{
    int t, s, r;

    t = (int) (top  - '0');
    if (t <= 1 || t >= 6) {       /* reduce sample space to a power of 2 */
        *x = 0;
        *entropy = 0;
        return -1;
    }
    s = (int) (side - '0');
    if (s < 1 || s > 6 ||
        s == t || s == (7 - t)) { /* side cannot match top or bottom face */
        *x = 0;
        *entropy = 0;
        return -1;
    }
    s -= ((s > t) ? 1 : 0) + ((s > (7 - t)) ? 1 : 0) + 1; /* map s to 0..3 */
    t -= 2;                       /* map t to 0..3 */
    r = 4 * t + s;
    assert(r >= 0 && r < 16);
    *x = (unsigned int)r;
    *entropy = 4;
    return 0;
}

/** Convert two ASCII digits representing a dice roll to a single integer
 *
 * When a six-sided die is shaken or cast inside a box and allowed to settle
 * along one edge, there are 24 possible orientations of the die. Each outcome
 * is specified by the numbers on the top and side faces, those opposite the
 * faces in contact with the box. To generate random bits from a roll, 4 bits
 * of entropy are returned if the top number is between 2 and 5, and 3 bits
 * otherwise.
 *
 * @param top one of the characters '1'..'6'
 * @param side one of four characters among '1'..'6' (the top and bottom
 *          numbers being invalid on a side face)
 * @param *x random bits
 * @param *entropy number of random low-order bits in x
 * @return 0 on success, -1 on failure
 */
int eroll_int_greedy(char top, char side, unsigned int *x,
                      unsigned int *entropy)
{
    int t, s, r;

    t = (int) (top - '0');
    s = (int) (side - '0');
    if (t < 1 || t > 6 || s < 1 || s > 6 || s == t || s == (7 - t)) {
        *x = 0;
        *entropy = 0;
        return -1;
    }
    s -= ((s > t) ? 1 : 0) + ((s > (7 - t)) ? 1 : 0) + 1;  /* s: [0, 3] */
    if (t > 1 && t < 6) {
        t -= 2;                       /* t: [0, 3] */
        r = 4 * t + s;
        assert(r >= 0 && r < 16);
        *x = (unsigned int)r;
        *entropy = 4;
        return 0;
    } else {
        if (t == 1)                   /* t: [0, 1] */
            t = 0;
        else
            t = 1;
        r = 4 * t + s;
        assert(r >= 0 && r < 8);
        *x = (unsigned int)r;
        *entropy = 3;
        return 0;
    }
}

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

/** Convert an ASCII string representing a sequence of dice rolls to bits
 */
int roll_bits(FILE *in, FILE *out, FILE *err, int is_interactive,
              int is_extended)
{
    struct bitbuf {
        unsigned int[BUFSIZE] bs;
        size_t  pos;
        size_t  size;
        size_t  unit;
    };
    struct bitbuf bits = {.pos  = 0,
                          .unit = sizeof(int) * 8,
                          .size = sizeof(int) * 8 * BUFSIZE};
    int entropy = 0;
    int num_rolls = 0;
    char const *prompt = "Enter dice rolls ";
    int top, side, delim;
    int x = -1;
    int input_err = 0;

    if (is_interactive)
        fputs(prompt, err);
    do {
        top = fgetc(in);
        if (top == EOF || (char)top == '\n')
            break;
        if (is_extended == 0) {
            num_rolls += 1;
            x = roll_int((char)top);
            if (x < 0)
                continue;
            entropy += 2;
            fill_bitbuf((unsigned int)x, 2, &bits, out);
        } else {
            side = fgetc(in);
            if (side == EOF || (char)side == '\n') {
                input_err = 1;
                break;
            }
            num_rolls += 1;
            x = eroll_int((char)top, (char)side);
            if (x < 0)
                continue;
            entropy += 4;
            fill_bitbuf((unsigned int)x, 4, &bits, out);
            fprintf(err, prompt);
            fprintf(err, "(bits extracted: %i) ", entropy);
            delim = fgetc(in);
            if (delim == EOF || (char)delim == '\n')  /* TODO: Test for '\r'? */
                break;
            if ((char)delim != ' ') {
                input_err = 1;
                break;
            }
        }
    } while (1)
    flush_bitbuf(&bits, out);
    if (input_err == 1)
        warn("WARNING: Input error, see usage");
    fprintf(err, "%i bits generated from %i rolls\n", entropy, num_rolls);
}

int main()
{
    FILE * const in = stdin;
    FILE * const out = stdout;
    FILE * const err = stderr;
    int is_interactive = 0;
    int is_extended = 0;
    int tty_changed = 0;
    struct termios prev_tattr;

    if (isatty(fileno(in))) {
        is_interactive = 1;
        tty_changed = (config_tty(in, &prev_tattr) == 0);
        if (tty_changed == 0)
            error("ERROR: Unable to configure terminal");
    }
    roll_bits(in, out, err, is_interactive);
    if (is_interactive != 0 && tty_changed != 0) {
        if (restore_tty(prev_tattr) != 0)
            warn("WARNING: Unable to restore terminal settings");
    }
    exit(EXIT_SUCCESS);
}
