# H-Code compilers

Two different compilers.

legible tape on these shows:
~~~
H-CODE COMPILER
H-CODE COMPILER PLUS.
~~~

# Execution of the compiler

 Entry  Description
======  =========================================
   512  translate program and execute if a **:START(N,subr)** is specified


# list of H-Code reserved words

Note that only the first four characters are stored in the symbol
tables. All reserved words must be preceded by a colon ":" e.g. **:SETS(I4,j7)**

Double colon "::" pases until word generator LSB is toggled to allow
swapping of program tapes.

## Global scope

* **SET** Reset  definitions?  (**LR**, **LS**, **LV**) ???
* **SETR** Define names of subroutines
* **SETS** Define storage (integers)
* **SETV** Define variables (floating-point)
* **DEFI** Define a subroutine up to **CLOS**
* **LIST** Compile and execute e.g. to store machine code in variables
  of run code to create tables at compile time
* **LR**, **LS**, **LV** define locals that can be hidden by **SET** ???
* **STAR** Set the program entry point e.g. **START(4,MAIN)**

## Unknown gobal scope words

* **OBEY**
* **PAGE**
* **PREL**
* **STOR**
* possible words: **LP** **XP** **PL** **PR**

## Used in subroutines

* **GOTO** either **GOTO 4** or **GOTO(7,subr)** (inter subroutine jump)
* **BRAN** Conditional branch e.g., **J+1:BRANCH(4,7,19)** (-,0,+) ???need double check for label order???
* **IF** **THEN** **ELSE**  the end of an if is **:,** (simulation of semicolon perhaps)
  also see condionals below
* **LOOP** .. **REPE**  an infinite loop
* **DO(expr)** .. **REPE**  a finite loop
* **EXIT** As return in most C-like languages
* **WAIT** Make sound and wait for word generator LSB to be toggled (allows for swapping data tapes)
* **STOP** Forms a equivalent to `n: 40 n:00 0`
* **'..'** Any text between single quotes is a print string (including CR and LF
* **I** Print integer e.g., **X4*2:I:4**
* **V** Print floating point possibly **XI7:V:4.2** ???check???
* **ON** Like C `+=` but for multiple integer variables e.g., **I+1:ON(J4,K,LI8)**
* **<**..**)** Inline machine code block can use variables **73 B:40 B1** (T2 relative ???check??? as **42 5,:41 15,**)
* **CLOS** Ends a subroutine definition

## Unknown

* **READ** film? read chars? read numbers? ???
* **WRIT** film?
* **FIND** film?
* **SUBR**
* **NUM**
* **TFS**
* **LOC**
* **SX**
* **SET** other use inside a subroutine

## Conditionals

* **NEGA**
* **NOTN**
* **POSI**
* **NOTP**
* **ZERO**
* **NOTN**
* **AND**
* **OR**

possibly like???
~~~
:IF(A-2):NEGA:GOTO 1
:IF(A<2):THEN
  ...
:ELSE
  ...
:,
~~~

## Expressions

* **SQ** ??need test to see if `x*x`
* **SQRT**
* **SIN**
* **COS**
* **TAN**
* **ARCT**  arctan
* **LOG** natural log
* **EXP** `e^x`
* **INT**
* **FRAC**
* **STAN** possibly `65 4096`  ???
* **MOD** remainder? float/int???

## LIST

* a parenthesised block of code
* an address (or variable name) followed by a block of machine code
* an asterisk to exit **LIST** mode
