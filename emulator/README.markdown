# Elliott 803 Emulator

## initial environment

Variable           Description
================   ============
**E803_TAPE_DIR**  colon separated list of paths to search for tapes


## Windows

Key  Top Window
===  ==========
F1   Punch 1
F2   Punch 2
F3   Teleprinter
F4   Console - transcript of console commands

## Console Commands

Command                   Abbr.  Description
========================  =====  ============
help                      (?)    this message
exit                      (x)    exit emulation
list [ADDR [COUNT]]       (l)    display memory words
mw ADDR CODE|±DEC                write memory word (machine code or signed decimal)
reset                            reset all regs and stop execution
reset run                        reset all regs and restart from zero
run [ADDR]                       run from address or continue after a stop
stop                             stop execution
regs                      (r)    display registers and status
hello [ADDR [1|2|3]]             load hello world [4096 1]
reader 1|2 [MODE] FILE           attach an existing file to a reader (hex5)
punch 1|2 [MODE] FILE            create file and attach to a punch (hex5)
wg                               display word generator
wg CODE|±DEC                     set word generator to machine code or signed decimal
wg f1|n1|f2|n2                   clear a set of bits (Note: n1 also clears B-Modifier)
wg f1|f2 F                       'or' octal value to function bits
wg n1|n2 N                       'or' decimal value to address bits (Note: '/' sets B-Modifier)
wg b                             set the B-Modifier bit


Abbreviation   Description
=============  ============
lower case     literal command text
digits         literal command text
space          literal command text
[…]            optional parameters
…|…            choice of parameter value
COUNT          unsigned decimal value
ADDR           decimal address 0..8191
CODE           machine code value (e.g. 74 29:40 123)
MODE           see table below
FILE           pathname of a file
±DEC           signed 39 bit decimal value (must have '+' or '-' sign prefix)
F              octal value 0…77 for logical or to word generator
N              decimal value 0…8191 for logical or to word generator

## File Reading MODE Values

Names               Description
==================  ==================
hex8 h8             One 8 channel character per line represented as two hex digits [00.ff]
hex5 h5             One 5 channel character per line represented as two hex digits [00.1f]
binary bin          Straight 8 bit or 5 bit binary data
elliott utf8 utf-8  ASCII/UTF-8 converted to/from Elliott 5 bit code


# Elliott 5 Bit Code

Figures columns Notes:
1. There are up to 4 characters in the figures column
2. First character is standard Elliott code
3. Second character is H-Code alternative print head
4  Third/fourth are convenience for ASCII/UTF-8 for Algol60 source
5. Case of ASCII letters is ignored

Code  Letter  Figure  Control
====  ======  ======  ==========
0                     Blank Tape
1     A a     1
2     B b     2
3     C c     *
4     D d     4
5     E e     $ <
6     F f     =
7     G g     7
8     H h     8
9     I i     '   ;
10    J j     ,
11    K k     +
12    L l     :
13    M m     -
14    N n     .
15    O o     % >
16    P p     0
17    Q q     (
18    R r     )
19    S s     3
20    T t     ?   ´
21    U u     5
22    V v     6
23    w w     /
24    X x     @
25    Y y     9
26    Z z     £ → # `
27                    Figure Shift
28                    Space
29                    Carriage Return
30                    Line Feed
31                    Letter Shift

## Bugs

- data read from initial instructions (0-3) must return zero, only
  execution and B-Line modification return bootloader values
