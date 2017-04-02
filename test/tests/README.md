Test Names
==========

Test files are as follows:

+ `cast_*`       = A cast test.
+ `cdecl-*`      = A command-line option test.
+ `cl-*`         = A command-line test.
+ `declare_*`    = A declare test.
+ `explain_*`    = An explain test.
+ `file-*`       = A file test.
+ `help-*`       = A help test.
+ `set_*`        = A set test.
+ `*-`*lang*`*` = A test using *lang* explicitly.

Within each test, sequences of single characters are used to denote a C/C++
declaration as it is in English.  The characters are:

+ `a` = array (following digit 1-9 specifies array dimension) or `auto`
+ `b` = block (Apple extension) or `bool`
+ `c` = `char` or `const`
+ `c16` = `char16_t`
+ `c32` = `char32_t`
+ `d` = `double`
+ `e` = `extern` or `...` (ellipsis)
+ `f` = function
+ `g` = `register`
+ `h` = `short`
+ `i` = `int`
+ `k` = `struct`
+ `l` = `long`
+ `m` = member of class
+ `p` = pointer
+ `r` = `restrict` or reference
+ `s` = `signed` or `static`
+ `t` = `typedef`
+ `u` = `union` or `unsigned`
+ `v` = `virtual`, `void`, or `volatile`
+ `w` = `wchar_t`
+ `x` = untyped K&R function argument

A function's return type is separated from its arguments by `_`.

Test File Format
================

cdecl test files must be a single line in the following format:

*command* `|` *options* `|` *input* `|` *exit*

that is four fields separated by the pipe (`|`) character
(with optional whitespace)
where:

+ *command* = command to execute (`cdecl`)
+ *options* = command-line options or blank for none
+ *input*   = command-line arguments
+ *exit*    = expected exit status code

Note on Test Names
------------------

Care must be taken when naming files that differ only in case
because of case-insensitive (but preserving) filesystems like HFS+
used on macOS.

For example, tests such as these:

    cdecl-i.test
    cdecl-I.test

that differ only in 'i' vs. 'I' will work fine on every other Unix filesystem
but cause a collision on HFS+.

One solution (the one used here) is to append a distinct number:

    cdecl-i-01.test
    cdecl-I-02.test

thus making the filenames unique regardless of case.
