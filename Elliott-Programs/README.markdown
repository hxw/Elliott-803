# Elliott 803 programs

These are the standard machine code translators:

* T2 - integer version
* T102 - floating point version

Both tapes are source tapes to allow the translators to be moved to high memory
so that programs starting at location 5 can be created.


## com-208  Memory Test

This is a binary program to chek if first memory bank is working.
It tests locations 4 to 4095 (0 to 3 are the built-in boot loader T1.


## T1 the boot loader

How I remember the first few locations of memory

~~~
0:  06    0:26    4         READ-ONLY created by logic
1:  22    4/16    3         READ-ONLY created by logic
2:  55    5:71    0         READ-ONLY created by logic
3:  43    1:40    2         READ-ONLY created by logic
4:  +0                      CORE, but used as general workspace
5:  First usable program location (also interrupt entry)
6:
7:  Algol60 compiler start location
~~~

The frst 4 loactions had specific logic circuits to override any value
read from core. 

An auto-starting seconstage faster boot loader can be auto started by
having T1 wrap round and write a value to location 4. Such that a jump
`40` opcode is created.

~~~
16    3    the store+clear in location 1
22 4093    value stored in location 4  (e.g. jump to 4096)
40 4096    the resulting addition and executed code
~~~

