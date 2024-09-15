Test Names
==========

Test files are as follows:

+ `ac-*.exp`  = Auto-completion test using **expect**(1).
+ `*.test`    = A regular **cdecl** test.

Note on Test Names
------------------

Do _not_ have files with the same name
except for case
since on case-insensitive (but preserving) filesystems,
e.g., APFS and HFS+,
those are considered the _same_ file.

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
    cdecl -bE -xc++ <<END
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

+ _Should_ include `-E`
  to echo the command
  before its corresponding output
  to aid debugging.

+ _Should_ include `-L$LINENO`
  to adjust the line number
  in error
  and warning
  messages
  to match the actual line number
  in the test file.

Use of "here" documents is preferred
since shell metacharacters don't have to be quoted.
