# cdecl

> *I’m still uncertain about the language declaration syntax, where in
> declarations, syntax is used that mimics the use of the variables being
> declared.  It is one of the things that draws strong criticism, but it has a
> certain logic to it.*

<div style="text-align: center">
-- Dennis M. Richie
</div>
<p></p>

> *I consider the C declarator syntax an experiment that failed.*

<div style="text-align: center">
-- Bjarne Stroustrup
</div>
<p></p>

**cdecl** is a program for composing and deciphering C (or C++)
type declarations or casts, aka ‘‘gibberish.’’
It can be used interactively on a terminal or
accept input from either the command line or standard input.

This version fixes virtually all the deficiencies in earlier versions
as well as adds many new features,
most notably:

* Using GNU Autotools for building.
* Command-line long-options.
* Support for C11 and C++ keywords
  `bool`,
  `char16_t`,
  `char32_t`,
  `complex`,
  `restrict`,
  `_Thread_local`,
  and
  `wchar_t`.
* Support for `typedef` declarations.
* Support for variadic function arguments.
* Better warning and error messages
  complete with location information and color.

## Installation

The git repository contains only the necessary source code.
Things like `configure` are _derived_ sources and
[should not be included in repositories](http://stackoverflow.com/a/18732931).
If you have `autoconf`, `automake`, and `m4` installed,
you can generate `configure` yourself by doing:

    autoreconf -fiv

Then follow the generic installation instructions given in `INSTALL`.
