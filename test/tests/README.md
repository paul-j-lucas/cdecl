Test Names
==========

Test files are as follows:

+ `cast_*`             = A cast test.
+ `cdecl-*`            = A command-line option test.
+ `cl-*`               = A command-line test.
+ `declare_*`          = A declare test.
+ `explain_*`          = An explain test.
+ `file-*`             = A file test.
+ `help-*`             = A help test.
+ `*_mo_*`             = A member operator.
+ `namespace_*`        = A namespace test.
+ `*_ctor_*`           = A constructor.
+ `*_dtor_*`           = A destructor.
+ `*_nmo_*`            = A non-member operator.
+ `typedef_*`          = A typedef test.
+ `*_o_*`              = An operator.
+ `set_*`              = A set test.
+ `static_cast_*`      = A static cast test.
+ `reinterpret_cast_*` = A reinterpret cast test.
+ `*_udc_*`            = A user-defined conversion test.
+ `*_udl_*`            = A user-defined literal test.
| `*-ec`               = An "east const" test.
+ `*-`*lang*`*`        = A test using *lang* explicitly.
+ `xec_*`              = An Embedded C test.

Within each test, sequences of characters are used to denote a C/C++
declaration as it is in English.
The characters are:

+ `+` = scope (`S::x`; `2+` = `S::T::x`, etc.)
+ `=` = scope (`=` = `of scope S`; `2=` = `of scope S of scope T`, etc.)
+ `0` = pure virtual
+ `a` = array (following digits specify array size), `_Atomic`, or `auto`
+ `a_nua_` = `no_unique_address`
+ `al` = `_Alignas` or `alignas`
+ `b` = block (Apple extension) or `bool`
+ `c` = `char`, `class`, or `const`
+ `cd` = `carries_dependency`
+ `c8` = `char8_t`
+ `c16` = `char16_t`
+ `c32` = `char32_t`
+ `d` = `deprecated` or `double`
+ `e` = `enum` or `explicit`
+ `el` = `...` (ellipsis)
+ `et` = `extern`
+ `f` = `friend` or function
+ `g` = `register`
+ `ga` = GNU C `__auto_type`
+ `gi` = GNU C `__inline__`
+ `grt` = GNU C `__restrict__`
+ `h` = `short`
+ `i` = `inline` or `int`
+ `k` = `struct`
+ `kr` = untyped K&R C function argument
+ `l` = `long`
+ `m` = `_Imaginary`, member of class, or `mutable`
+ `n` = name, nested, or `union`
+ `nd` = `nodiscard`
+ `nr` = `noreturn`
+ `ns` = `namespace`
+ `nt` = non-throwing
+ `nx` = `noexcept`
+ `o` = `operator`, `override`, or `thread_local`
+ `p` = pointer
+ `r` = reference
+ `rr` = rvalue reference
+ `rt` = `restrict`
+ `s` = `signed` or `static`
+ `t` = `throw`, `typedef`, `typename`, or unknown type
+ `u` = `maybe_unused` or `unsigned`
+ `v` = `virtual`, `void`, or `volatile`
+ `w` = `wchar_t`
+ `xi` = `constinit`
+ `xx` = `constexpr`
+ `y` = `final`
+ `z` = `_Complex` or `size_t`
+ `zz` = `ssize_t`

For an Embedded C test, some characters change meaning:

+ `a` = `_Accum`
+ `r` = `_Fract`
+ `t` = `_Sat`

Characters may be preceded by a count,
e.g., `2i` for 2 consecutive `int`.

A function's return type is separated from its arguments by `_`
using the traditional "leading" syntax
or `__` using the C++11 "trailing" syntax.

Test File Format
================

Cdecl test files must be a single line in the following format:

*command* `@` *config* `@` *options* `@` *input* `@` *exit*

that is five fields separated by the at (`@`) character
(with optional whitespace)
where:

+ *command* = command to execute (`cdecl`)
+ *config*  = name of config file to use or `/dev/null` for none
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

that differ only in `i` vs. `I` will work fine on every other Unix filesystem
but cause a collision on HFS+.

One solution (the one used here) is to append a distinct number:

    cdecl-i-01.test
    cdecl-I-02.test

thus making the filenames unique regardless of case.
