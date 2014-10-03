# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <assert.h>

// TODO: Lock memory
// TODO: Prevent other process from attaching to or reading memory
// TODO: Prevent hardware from reading process memory
// TODO: Lock files, see getpass

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
 * @param side one of the characters '1'..'6', except top and 7 - top
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

/** Convert an ASCII string representing a sequence of dice rolls to bits
 */
int erolls_bits(char **bits, int *entropy, const char *rolls)
{
    int len, nrolls, i, x;
    char *bs;

    len = strlen(rolls);
    if (len % 3 != 2)
        return 1;
    nrolls = (len + 1) / 3;
    bs = malloc(sizeof(char) * (nrolls+1));
    *entropy = 4 * nrolls;
    for (i = 0; i < nrolls; i++) {
        x = eroll_int(rolls[3 * i], rolls[3 * i + 1]);
        if (x < 0)
            continue;
        /* TODO: use two counters to track rolls and bs */
        bs[i] = x;
    }
    bs[nrolls] = '\0';
    *bits = bs;
    return 0;
}

int main()
{
    char *prompt, *rolls, *bits;
    int entropy, err, i;

    prompt = "Dice rolls:";
    rolls = getpass(prompt);
    err = erollstr2bits(&bits, &entropy, rolls);
    if (err) {
        puts("Error");
    } else {
        for (i = 0; i < entropy/4; i++) {
            putchar(bits[i]);
        }
    }
    return 0;
}
