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

// TODO: Lock memory
// TODO: Prevent other process from attaching to or reading memory
// TODO: Prevent hardware from reading process memory
// TODO: Lock files, see getpass

int warn(char const *msg)
{
    fputs(msg, stderr);
    return 0;
}

int abort(char const *msg)
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
        if(tcsetattr(fileno(in), TCSAFLUSH|TCSASOFT, &t) == 0)
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
 * @return a value in 0..3 inclusive, or -1 indicating an invalid roll
 */
int roll_int(char top)
{
    int t;

    t = (int) (top - '0');
    if (t <= 1 || t >= 6)
        return -1;                /* reduce sample space to a power of 2 */
    t -= 2;
    assert(t >= 0 && t < 4);
    return t;
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
 * @return a value in 0..15 inclusive, or -1 indicating an invalid roll
 */
int eroll_int(char top, char side)
{
    int t, s, r;
    t = (int) (top  - '0');
    if (t <= 1 || t >= 6)
        return -1;                /* reduce sample space to a power of 2 */
    s = (int) (side - '0');
    if (s == t || s == (7 - t))
        return -1;                /* side cannot match top or bottom face */
    s -= ((s > t) ? 1 : 0) + ((s > (7 - t)) ? 1 : 0) + 1; /* map s to 0..3 */
    t -= 2;                       /* map t to 0..3 */
    r = 4 * t + s;
    assert(r >= 0 && r < 16);
    return r;
}

int flush_bitbuf(struct bitbuf *bits, FILE *out)
{
}

int fill_bitbuf(int x, int entropy, struct bitbuf *bits, FILE *out)
{
}

/** Convert an ASCII string representing a sequence of dice rolls to bits
 */
int roll_bits(FILE *in, FILE *out, FILE *err, int is_interactive,
              int is_extended)
{
    struct bitbuf {
        int[32] bs;
        int     pos;
        int     size;
        int     unit;
    };
    struct bitbuf bits = {.unit = sizeof(int) * 8, .size = sizeof(int) * 256};
    int entropy = 0;
    int num_rolls = 0;
    char const *prompt = "Enter dice rolls ";
    int top, side, delim;
    int x = -1;
    int input_err = 0;

    if(is_interactive)
        fputs(prompt, err);
    do {
        top = fgetc(in);
        if(top == EOF || (char)top == '\n')
            break;
        if(is_extended == 0) {
            num_rolls += 1;
            x = roll_int((char)top);
            if(x < 0)
                continue;
            entropy += 2;
            fill_bitbuf(x, 2, &bits, out);
        } else {
            side = fgetc(in);
            if(side == EOF || (char)side == '\n') {
                input_err = 1;
                break;
            }
            num_rolls += 1;
            x = eroll_int((char)top, (char)side);
            if(x < 0)
                continue;
            entropy += 4;
            fill_bitbuf(x, 4, &bits, out);
            fprintf(err, prompt);
            fprintf(err, "(bits extracted: %i) ", entropy);
            delim = fgetc(in);
            if(delim == EOF || (char)delim == '\n')  /* TODO: Test for '\r'? */
                break;
            if((char)delim != ' ') {
                input_err = 1;
                break;
            }
        }
    } while (1)
    flush_bitbuf(&bits, out);
    if(input_err == 1)
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

    if(isatty(fileno(in))) {
        is_interactive = 1;
        tty_changed = (config_tty(in, &prev_tattr) == 0);
        if(tty_changed == 0)
            abort("ERROR: Unable to configure terminal");
    }
    roll_bits(in, out, err, is_interactive);
    if(is_interactive != 0 && tty_changed != 0) {
        if(restore_tty(prev_tattr) != 0)
            warn("WARNING: Unable to restore terminal settings");
    }
    exit(EXIT_SUCCESS);
}
