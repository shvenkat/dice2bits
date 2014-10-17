# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <assert.h>
# include <termios.h>
# include <errno.h>
# include <bitbuf.h>

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

/** Convert ASCII string representing a sequence of dice rolls to bit output
 */
int roll_bits(FILE *in, FILE *out, FILE *err, int is_interactive,
              int is_extended)
{
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
