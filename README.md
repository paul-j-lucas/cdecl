# cdecl

[![Build Status](https://app.travis-ci.com/paul-j-lucas/cdecl.svg?branch=master)](https://app.travis-ci.com/paul-j-lucas/cdecl)

## Introduction

> *I’m still uncertain about the language declaration syntax, where in
> declarations, syntax is used that mimics the use of the variables being
> declared.  It is one of the things that draws strong criticism, but it has a
> certain logic to it.*

<div style="text-align: center">
&mdash; Dennis M. Ritchie
</div>
<p></p>

> *I consider the C declarator syntax an experiment that failed.*

<div style="text-align: center">
&mdash; Bjarne Stroustrup
</div>
<p></p>

**cdecl** (_see-deh-kull_)
is a program for composing and deciphering C (or C++)
declarations or casts, aka ‘‘gibberish.’’
It can be used interactively on a terminal or
accept input from either the command line or standard input.

This codebase fixes virtually all the deficiencies in earlier versions
as well as adds many new features,
most notably:

* Using GNU Autotools for building.
* Command-line long-options.
* Distinguishes among
  K&R C,
  C89,
  C95,
  C99,
  C11,
  C17,
  C2X,
  C++98,
  C++03,
  C++11,
  C++14,
  C++17,
  C++20,
  and
  C++23.
* Support for C89
  `const`,
  `restrict`,
  and
  `volatile`
  qualifiers.
* Support for C99
  `static`,
  type-qualified,
  and
  variable length array
  function parameters.
* Support for C99 extensions
  [Embedded C](http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1021.pdf)
  and
  [Unified Parallel C](https://upc.lbl.gov/).
* Support for `inline` function and variable declarations.
* Support for `typedef` declarations.
* Pre-defined `typedef` declarations
  for all standard C & C++ language types
  (`_Bool`,
   `_Complex`, `_Imaginary`,
   `char8_t`,
   `wchar_t`,
   etc.),
  many POSIX and standard library types
  (`FILE`,
   `in_addr_t`,
   `int8_t`,
   `pid_t`,
   `pthread_t`,
   `ptrdiff_t`,
   `size_t`,
   `std::ostream`,
   `std::string`,
   etc.),
  and all
  [Microsoft Windows types](https://docs.microsoft.com/en-us/windows/win32/winprog/windows-data-types).
* Support for [Microsoft Windows calling conventions](https://docs.microsoft.com/en-us/cpp/cpp/argument-passing-and-naming-conventions).
* Support for variadic function parameters.
* Support for C and C++ alternative tokens
  (`and`, `and_eq`, etc.).
* Support for C++
  constructors,
  destructors,
  `mutable` data members,
  namespaces and scoped names,
  new-style casts,
  pointers to members,
  `throw`,
  overloaded operators,
  and
  `friend`,
  `virtual` and pure `virtual`
  member function declarations,
  and
  user-defined conversion operators.
* Support for C++11
  `auto` (as a deduced type),
  `constexpr`,
  `enum class`,
  fixed-type enumerations,
  `final`,
  `noexcept`,
  `override`,
  rvalue references,
  user-defined literals,
  `using` (as a `typedef` synonym),
  the function trailing return-type syntax,
  and
  ref-qualified member function declarations.
* Support for C++20
  `consteval`,
  `constinit`,
  and
  `export`
  declarations.
* Support for C++
  `[[carries_dependency]]`,
  `[[deprecated]]`,
  `[[maybe_unused]]`,
  `[[nodiscard]]`,
  and
  `[[noreturn]]`
  attribute specifiers.
* Better warning and error messages
  complete with location information,
  color,
  and "Did you mean ...?" suggestions.

## Installation

The git repository contains only the necessary source code.
Things like `configure` are _derived_ sources and
[should not be included in repositories](http://stackoverflow.com/a/18732931).
If you have `autoconf`, `automake`, and `m4` installed,
you can generate `configure` yourself by doing:

    ./bootstrap

You will also need
[`flex`](https://github.com/westes/flex)
and
[`bison`](https://www.gnu.org/software/bison/)
(`lex` and `yacc` won't do).
Or you can download a
[released version](https://github.com/paul-j-lucas/cdecl/releases)
that contains `configure`
and the generated lexer and parser.
In either case,
then follow the generic installation instructions given in
[`INSTALL`](https://github.com/paul-j-lucas/cdecl/blob/master/INSTALL).

If you would like to generate code coverage reports,
you will also need
`gcov` (part of `gcc`)
and
[`lcov`](https://github.com/linux-test-project/lcov);
then instead do:

    ./configure --enable-coverage
    make check-coverage

If you would like to generate the developer documentation,
you will also need
[Doxygen](http://www.doxygen.org/);
then do:

    make doc                            # or: make docs

and then open `docs/html/index.html`
with a browser.

## Licensing

**cdecl** was originally written by Graham Ross
sometime in the mid-1980s.
Tony Hansen, a major contributor,
[posted the source code with his changes](https://groups.google.com/d/msg/comp.sources.unix/Y76scbXQQBk/MVrZZBG0nNwJ)
to `comp.sources.unix` in 1988.

The source files have never had either a copyright notice or license.
Attempts have been made independently by several people
to contact Graham over the years to clear-up the licensing issue,
all without success.

In 1996,
David Conrad made contributions for version 2.5 and wrote:

> *I have no reason to believe there are any limitations on [**cdecl**'s] use,
> and strongly believe it to be in the Public Domain.*

(See the included
[`README-2.5.txt`](https://github.com/paul-j-lucas/cdecl/blob/master/README-2.5.txt)
for David's full comments.)

Something in the Public Domain allows anyone to do whatever they want with it.
Assuming that David is correct
and that **cdecl** is in the Public Domain,
I therefore am licensing **cdecl** 3.0 (and later) under the
[GNU General Public License, v3](https://www.gnu.org/licenses/gpl-3.0.en.html).

**Paul J. Lucas**  
San Francisco, California, USA  
16 April 2017
