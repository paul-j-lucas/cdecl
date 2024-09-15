Test Names
==========

Test files are as follows:

+ `ac-*.exp`  = Auto-completion test.
+ `cast_*`    = A cast test.
+ `cdecl-*`   = A command-line option test.
+ `cl-*`      = A command-line test.
+ `declare*`  = A `declare` test.
+ `explain*`  = An `explain` test.
+ `file-*`    = A file test.
+ `help-*`    = A help test.
+ `include-*` = An `include` test.
+ `set_*`     = A `set` test.

Note on Test Names
------------------

Care must be taken when naming files that differ only in case
because of case-insensitive (but preserving) filesystems
like those generally used on macOS.

For example,
tests such as these:

    cdecl-i.test
    cdecl-I.test

that differ only in `i` vs. `I` will work fine on every other Unix filesystem
but cause a collision on macOS.

One solution (the one used here) is to append a distinct number:

    cdecl-i-01.test
    cdecl-I-02.test

thus making the filenames unique regardless of case.

Test File Format
================

**Cdecl** test files are mini shell scripts
that are included verbatim into `run_test.sh`
via the shell's `.` command.
However,
they must have a particular format.

The first executable line of a test
_must_ be a line like:

`EXPECTED_EXIT=`_n_

where _n_ is the expected integer exit status of **cdecl**.
Valid status codes are given in **cdecl**(1).

After the `EXPECTED_EXIT` line,
the remaing lines are more free-form.
However,
the exit status of the test script as a whole
is considered its _actual exit status_
and is compared against the value of `EXPECTED_EXIT`.
The test is considered failed if those values are unequal
and those values being equal is a prerequisite for the test passing.

A test is typically like:

    EXPECTED_EXIT=0
    cdecl -bO -xc++ <<END
    explain int s::x
    END

that is **cdecl** is invoked
along with any options necessary for the test
followed by a shell "here" document.
The options:

+ _Must_ include `-b`
  to disable buffering on stdout
  so output from stdout and stderr
  is printed in the correct order.

+ _Should_ include `-O`
  to echo the command
  before its corresponding output
  to aid debugging.

Use of "here" documents is preferred
since shell metacharacters don't have to be quoted.
