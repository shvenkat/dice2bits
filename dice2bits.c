#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <termios.h>
#include <errno.h>
#include "bitbuf.h"
#include "d6.h"

#ifndef TCSASOFT
#define TCSASOFT 0
#endif

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

    if (setvbuf(in, NULL, _IONBF, 0) != 0)
        return -1;
    if (tcgetattr(fileno(in), prev_tattr) == 0) {
        t = *prev_tattr;
        cfmakeraw(&t);
        t.c_lflag &= ~(ECHO|ISIG);
        if (tcsetattr(fileno(in), TCSAFLUSH|TCSASOFT|TCSANOW, &t) == 0)
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
    return tcsetattr(fileno(in), TCSAFLUSH|TCSASOFT|TCSANOW, prev_tattr);
}

/** Convert ASCII string representing a sequence of dice rolls to bit output
 */
int roll_bits(FILE *in, FILE *out, FILE *err, int is_interactive,
              int is_extended)
{
    struct bitbuf bits = {.pos  = 0,
                          .unit = sizeof(int) * 8,
                          .size = sizeof(int) * 8 * BUFSIZE};
    char const *prompt = "\rEnter dice rolls ";
    int top, side, delim;
    int input_err = 0;
    unsigned int num_rolls = 0;
    unsigned int x = 0;
    unsigned int e = 0;
    unsigned int entropy = 0;

    if (is_interactive)
        fputs(prompt, err);
    do {
        top = fgetc(in);
        if (top == EOF || (char)top == '\n' || (char)top == '\r')
            break;
        if (is_extended == 0) {
            num_rolls += 1;
            if (roll_int_greedy((char)top, &x, &e) != 0)
                continue;
            if (fill_bitbuf(x, e, &bits, out) != 0)
                continue;
            entropy += e;
            if (is_interactive) {
                fputs(prompt, err);
                fprintf(err, "(bits extracted: %i) ", entropy);
            }
        } else {
            side = fgetc(in);
            if (side == EOF || (char)side == '\n') {
                input_err = 1;
                break;
            }
            num_rolls += 1;
            if (eroll_int_greedy((char)top, (char)side, &x, &e) != 0)
                continue;
            fill_bitbuf(x, e, &bits, out);
            entropy += e;
            if (is_interactive) {
				fputs(prompt, err);
				fprintf(err, "(bits extracted: %i) ", entropy);
            }
            delim = fgetc(in);
            if (delim == EOF || (char)delim == '\n')  /* TODO: Test for '\r'? */
                break;
            if ((char)delim != ' ') {
                input_err = 1;
                break;
            }
        }
    } while (1);
    if (flush_bitbuf(&bits, out) != 0)
        warn("WARNING: Output error");
    if (fflush(out) != 0)
        warn("WARNING: Error flushing output");
    if (input_err == 1)
        warn("WARNING: Input error, see usage");
    fprintf(err, "\n%i bits generated from %i rolls\n", entropy, num_rolls);
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
    roll_bits(in, out, err, is_interactive, is_extended);
    if (is_interactive != 0 && tty_changed != 0) {
        if (restore_tty(in, &prev_tattr) != 0)
            warn("WARNING: Unable to restore terminal settings");
    }
    exit(EXIT_SUCCESS);
}
