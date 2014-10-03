dice2bits: diceware for computers
(c) 2014 Shiv Venkatasubrahmanyam
Licensed under the GNU Public License version 2

dice2bits is a simple utility that facilitates the use of fair dice as a
hardware entropy source. It converts a sequence of dice rolls to bits, which
can be used to seed random number generators, create cryptographic keys and the
like. Inspired by diceware for generating memorable passphrases, dice2bits
assists the user in generating random bit sequences of known entropy, in a
transparent and easily verified manner.

dice2bits accepts text input representing a sequence of dice rolls, and
provides binary output and its entropy. Input is of the form "2533146...",
where each digit represents the conventional result of a dice roll i.e. the
number on the top face of the die. Alternatively, dice can be shaken in a box
and allowed to settle along an inside edge, such that two adjacent faces of
each die are in contact with the box. In this case, the result of each roll
is fully specified by the numbers on the faces opposite the contacts, read
in order. This method captures more entropy per roll, encoding it in the input
as pairs of digits separated by spaces.

Security

Verifying (samples of) the output against the input by hand
Whitening
XORing with other sources
Exposure to keyloggers, other userspace code, X clients, etc.
Exposure to untrusted compilers and libraries
