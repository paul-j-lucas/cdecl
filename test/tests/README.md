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
