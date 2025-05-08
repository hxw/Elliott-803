# Elliott 803 Emulator and Programs

See the individual read me files for more details.

# Various Program Tapes

captured using https://github.com/hxw/paper-tape.git

File extensions used

* `.hex8` one 8 channel character per line represented as two hex digits [00.ff]
* `.hex5` one 5 channel character per line represented as two hex digits [00.1f]
* `.h-code` UTF-8 right arrow but the rest are ASCII
* `.elliott` UTF-8 UK currency sign and the rest are ASCII
* `.ascii` Substitute an ASCII `#` instead of UK currency sign

# Elliott 803 Programs

## Included

These are programs I kept copies of some tapes after I left
Loughborough Grammar School (Where the Brush 803 was) and was able to
recover using a home-built paper tape reader. These include:

* A104 - Algol 60 compiler (both tapes)
* T2 and T102 - The fixed and floating point machine code translators
* H-Code - A compiler developed for Brush Electrical Machines

A note about H-Code. As far as I remember it was written by a Dr. Hogg
who possibly later worked at Loughborough University (If anyone knows
more, I would be pleased to update).  The compiler is a super-set of
the Elliott Autocode Language and used the same variable/indexing
method (`AI4` is like the C equivalent of `a[i[4]]`).  Since variables
are only a single character it offered a way to define local variables
to provide more. It has subroutines and provides a method to pass
multiple parameters and return multiple results.

**New addition**: [H-Code Documentation](H-Code-Compilers/README.markdown)

## Missing

* Library routines - we did not have tape copies, only the printed
  manuals
* Elliott Autocode Compiler - I did not keep a copy of this tape
