# H-Code compilers

| File                       | Description                                |
| -------------------------- | ------------------------------------------ |
| BSDmakefile                | Makefile to compile LaTeX Documents to PDF |
| H-Code\_Intro/             | Short introduction to H-Code               |
| H-Code\_Manual/            | More detailed version of H-Code Language   |
| OLD-MEMORY.markdown        | My initial memories of H-Code language prior to getting Intro/Manual   |
| README.markdown            | This file                                  |
| h-code-compiler-plus.hex5  | Plus version of the compiler               |
| h-code-compiler.hex5       | Normal version of the compiler             |

## Two different compilers.

legible tape on these shows:
~~~
H-CODE COMPILER
H-CODE COMPILER PLUS.
~~~

I do not know the exact difference between these.

## Recovered documents

The original scans of these are very poor quality large PDF scans of
1960's typewritten documents.  They are not suitable for storing in a
Git repository.  I have tried to replicate the contents of these and
tried to retain to look of these 1960's era files.  Since there are
many hand-written parts of these documents such as mathematical
equations, flow charts, and the use of some Greek or other special
characters, a simple text file does not work.

I used ImageMagick to try to clean these and Tesseract OCR on the
resulting images.  The results were very bad and the two column format
often confused the OCR.  In the end, after lot of retyping I converted
them into LaTeX files.  I tried to keep them looking like the
originals, but added an option to create a single column version of
the Manual see its `BSDmakefile` for details.

The top level `BSDmakefile` will produce the two column Info PDF and
both PDF versions version of the Manual.  The following files will be
created:

| File                       | Description                                |
| -------------------------- | ------------------------------------------ |
| H-Code\_Intro.pdf          | Two column introduction (like original)    |
| H-Code\_Manual.pdf         | Two column manual (like original)          |
| H-Code_Manual\_single.pdf  | Single column, larger font,  more readable |


## My old notes on H-Code

[Moved here](OLD-MEMORY.markdown)
