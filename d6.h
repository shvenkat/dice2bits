int roll_bits_discard(char top, unsigned int *x, unsigned int *entropy);

int roll_bits_greedy(char top, unsigned int *x, unsigned int *entropy);

int eroll_bits_discard(char top, char side, unsigned int *x,
                       unsigned int *entropy);

int eroll_bits_greedy(char top, char side, unsigned int *x,
                      unsigned int *entropy);
