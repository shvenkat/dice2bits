#include "d6.h"

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

    t = (int)(top - '0');
    if (t <= 1 || t >= 6) {
        *x = 0;
        *entropy = 0;
        return -1;                /* reduce sample space to a power of 2 */
    }
    t -= 2;
    /* assert(t >= 0 && t < 4); */
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

    t = (int)(top  - '0');
    if (t <= 1 || t >= 6) {       /* reduce sample space to a power of 2 */
        *x = 0;
        *entropy = 0;
        return -1;
    }
    s = (int)(side - '0');
    if (s < 1 || s > 6 ||
        s == t || s == (7 - t)) { /* side cannot match top or bottom face */
        *x = 0;
        *entropy = 0;
        return -1;
    }
    s -= ((s > t) ? 1 : 0) + ((s > (7 - t)) ? 1 : 0) + 1; /* map s to 0..3 */
    t -= 2;                       /* map t to 0..3 */
    r = 4 * t + s;
    /* assert(r >= 0 && r < 16); */
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

    t = (int)(top - '0');
    s = (int)(side - '0');
    if (t < 1 || t > 6 || s < 1 || s > 6 || s == t || s == (7 - t)) {
        *x = 0;
        *entropy = 0;
        return -1;
    }
    s -= ((s > t) ? 1 : 0) + ((s > (7 - t)) ? 1 : 0) + 1;  /* s: [0, 3] */
    if (t > 1 && t < 6) {
        t -= 2;                       /* t: [0, 3] */
        r = 4 * t + s;
        /* assert(r >= 0 && r < 16); */
        *x = (unsigned int)r;
        *entropy = 4;
        return 0;
    } else {
        if (t == 1)                   /* t: [0, 1] */
            t = 0;
        else
            t = 1;
        r = 4 * t + s;
        /* assert(r >= 0 && r < 8); */
        *x = (unsigned int)r;
        *entropy = 3;
        return 0;
    }
}
