Test Names
==========

Test files are as follows:

+ `ac-*.exp`           = Auto-completion test.
+ `cast_*`             = A cast test.
+ `cdecl-*`            = A command-line option test.
+ `cl-*`               = A command-line test.
+ `declare_*`          = A `declare` test.
+ `explain_*`          = An `explain` test.
+ `*-ec`               = An "east const" test.
+ `file-*`             = A file test.
+ `help-*`             = A help test.
+ `include-*`          = An `include` test.
+ `*-`_lang_`*`        = A test using _lang_ explicitly.
+ `reinterpret_cast_*` = A `reinterpret_cast` test.
+ `set_*`              = A `set` test.
+ `static_cast_*`      = A `static_cast` test.
+ `*typedef*`          = A `typedef` test.
+ `using_*`            = A `using` test.
+ `xec_*`              = An Embedded C test.
+ `xupc_*`             = A Unified Parallel C test.

Within each test, sequences of characters are used to denote a C/C++
declaration as it is in English.
The characters are:

+ `+` = scope (`S::x`; `2+` = `S::T::x`, etc.)
+ `++` = scope (`++` = `of scope S`; `2++` = `of scope S of scope T`, etc.)
+ `0` = pure virtual
+ `a` = array (following digits specify array size)
+ `a_cd_` = `carries_dependency`
+ `a_dep_` = `deprecated`
+ `a_mu_` = `maybe_unused`
+ `a_nd_` = `nodiscard`
+ `a_nua_` = `no_unique_address`
+ `a_re_` = `reproducible`
+ `a_us_` = `unsequenced`
+ `a_using_` = attribute with `using`
+ `al` = `_Alignas` or `alignas`
+ `an` = array with name for size, e.g., `a[n]`
+ `at` = `_Atomic` or `atomic`
+ `au` = `auto`
+ `b` = `bool`
+ `bi` = `_BitInt` (following digits specify number of bits)
+ `bl` = block (Apple extension)
+ `c` = `char`, `class`, or `const`
+ `c8` = `char8_t`
+ `c16` = `char16_t`
+ `c32` = `char32_t`
+ `co` = copy (for lambda capture)
+ `ctor` = constructor
+ `cx` = `_Complex`
+ `d` = `double`
+ `dtor` = destructor
+ `ec` = `extern "C"`
+ `ef` = `enum` with fixed type (that follows)
+ `el` = `...` (ellipsis)
+ `en` = `enum`
+ `ep` = `explicit`
+ `et` = `extern`
+ `ex` = `export`
+ `f` = `friend` or function
+ `gat` = GNU C `__attribute__`
+ `gau` = GNU C `__auto_type`
+ `gi` = GNU C `__inline__`
+ `grt` = GNU C `__restrict__`
+ `h` = `short`
+ `i` = `inline` or `int`
+ `ii` = Pre-C99 implicit `int`
+ `im` = `_Imaginary` or `imaginary`
+ `k` = `struct`
+ `kr` = untyped K&R C function argument
+ `l` = `long`
+ `la` = lambda
+ `m` = member of class or `mutable`
+ `mo` = member operator
+ `mscc` = Microsoft C `__clrcall`
+ `mscd` = Microsoft C `__cdecl`
+ `msds` = Microsoft C `__declspec`
+ `msfc` = Microsoft C `__fastcall`
+ `mssc` = Microsoft C `__stdcall`
+ `mstc` = Microsoft C `__thiscall`
+ `msvc` = Microsoft C `__vectorcall`
+ `mswa` = Microsoft C `WINAPI`
+ `n` = name, nested, or `union`
+ `ne` = `non-empty`
+ `nm` = non-member
+ `nr` = `noreturn`
+ `ns` = `namespace`
+ `nt` = non-throwing
+ `nx` = `noexcept`
+ `o` = `operator`
+ `ov` = `override`
+ `p` = pointer
+ `r` = reference
+ `rg` = `register`
+ `rr` = rvalue reference
+ `rt` = `restrict`
+ `si` = `signed`
+ `st` = `static`
+ `t` = unknown type
+ `td` = `typedef`
+ `th` = `throw`
+ `tl` = `thread_local`
+ `tn` = `typename`
+ `ts` = `this`
+ `u` = `unsigned`
+ `udc` = user-defined conversion
+ `udl` = user-defined literal
+ `v` = `virtual`, `void`, or `volatile`
+ `w` = bit-field width (following digits specify width)
+ `wt` = `wchar_t`
+ `xi` = `constinit`
+ `xv` = `consteval`
+ `xx` = `constexpr`
+ `y` = `final`
+ `z` = `size_t`
+ `zz` = `ssize_t`

For an Embedded C test, some characters change meaning:

+ `a` = `_Accum`
+ `r` = `_Fract`
+ `t` = `_Sat`

For a Unified Parallel C test, some characters change meaning:

+ `r` = `relaxed`
+ `d` = `shared`
+ `dq` = `shared` followed by layout qualifier (followed by digits or + for *)
+ `t` = `strict`

Characters may be preceded by a count,
e.g., `2i` for 2 consecutive `int`.

A function's return type is separated from its arguments by `_`
using the traditional "leading" syntax
or `__` using the C++11 "trailing" syntax.

Note on Test Names
------------------

Care must be taken when naming files that differ only in case
because of case-insensitive (but preserving) filesystems
like those generally used on macOS.

For example, tests such as these:

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
However, the exit status of the test script as a whole
is considered its _actual exit status_
and is compared against the value of `EXPECTED_EXIT`.
The test is considered failed if those values are unequal
and those values being equal is a prerequisite for the test passing.

A test is typically like:

    EXPECTED_EXIT=0
    cdecl -xc++ <<END
    explain int s::x
    END

that is **cdecl** is invoked
along with any options necessary for the test
followed by a shell "here" document.
Use of "here" documents is preferred
since shell metacharacters don't have to be quoted.
