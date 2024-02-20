# cdecl

## Introduction

> *I’m still uncertain about the language declaration syntax, where in
> declarations, syntax is used that mimics the use of the variables being
> declared.  It is one of the things that draws strong criticism, but it has a
> certain logic to it.*

<div style="text-align: center">
&mdash; Dennis M. Ritchie, Creator of C
</div>
<p></p>

> *I consider the C declarator syntax an experiment that failed.*

<div style="text-align: center">
&mdash; Bjarne Stroustrup, Creator of C++
</div>
<p></p>

**cdecl** (_see-deh-kull_)
is a program
primarily
for composing
and deciphering
C (or C++) declarations
or casts,
aka ‘‘gibberish.’’
It can be used interactively on a terminal
or accept input
from either the command line
or standard input.
For example:

```
cdecl> explain int *const (*p)[4]
declare p as pointer to array 4 of constant pointer to integer

cdecl> declare p as pointer to const pointer to const char
const char *const *p;
```

Additionally,
**cdecl**
is also for
developing
and
debugging
C preprocessor macros
by performing expansion step-by-step.
For example:

```
cdecl> #define NAME2_HELPER(A,B)         A ## B
cdecl> #define NAME2(A,B)                NAME2_HELPER(A,B)
cdecl> expand NAME2(var_, __LINE__)
NAME2(var_, __LINE__) => NAME2_HELPER(A,B)
| A => var_
| B => __LINE__
| | __LINE__ => 42
| B => 42
NAME2(var_, 42) => NAME2_HELPER(var_,42)
| NAME2_HELPER(var_, 42) => A ## B
| NAME2_HELPER(var_, 42) => var_ ## 42
| NAME2_HELPER(var_, 42) => var_42
NAME2(var_, 42) => var_42
```

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
  C23,
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
  for many POSIX and standard library types
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
  lambdas,
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
* Support for C++23 explicit object parameters.
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
If you have
[`autoconf`](https://www.gnu.org/software/autoconf/),
[`automake`](https://www.gnu.org/software/automake/),
and
[`m4`](https://www.gnu.org/software/m4/)
installed,
you can generate `configure` yourself by doing:

    ./bootstrap

You will also need
[`flex`](https://github.com/westes/flex)
and
[`bison`](https://www.gnu.org/software/bison/)
(`lex` and `yacc` won't do);
or you can download a
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

## Licensing

**cdecl** was originally written by Graham Ross
sometime in the mid-1980s.
Tony Hansen,
a major contributor,
posted the source code with his changes
(parts [1](https://groups.google.com/g/comp.sources.unix/c/Y76scbXQQBk/m/MVrZZBG0nNwJ)
&
[2](https://groups.google.com/g/comp.sources.unix/c/yzWbI4agBE0/m/ddqzmuiEidwJ))
to `comp.sources.unix` in 1988.

The source files have never had either a copyright notice
or license.
Attempts have been made independently
by several people
to contact Graham over the years
to clear-up the licensing issue,
all without success.

In 1996,
David Conrad made contributions for version 2.5
and wrote:

> *I have no reason to believe there are any limitations on [**cdecl**'s] use,
> and strongly believe it to be in the Public Domain.*

(See the included
[`README-2.5.txt`](https://github.com/paul-j-lucas/cdecl/blob/master/README-2.5.txt)
for David's full comments.)

Something in the Public Domain allows anyone to do whatever they want with it.
Assuming that David is correct
and that **cdecl** is in the Public Domain,
I therefore am licensing **cdecl** 3.0
(and later)
under the
[GNU General Public License, v3](https://www.gnu.org/licenses/gpl-3.0.en.html).

However,
even if you do _not_ assume that the original version of **cdecl**
is in the public domain,
copyright law allows for
["fair use"](https://www.copyright.gov/fair-use/more-info.html)
for which there are four factors to consider:

1. **Purpose and character of the use,
   including whether the use is of a commercial nature
   or is for nonprofit educational purposes.**

   My version of **cdecl**
   is not of a commercial nature
   and is for nonprofit educational purposes.

2. **Nature of the copyrighted work.**

   The purpose of **cdecl**
   is to provide a tool
   for C and C++ developers.
   Unlike, say, a literary work,
   the _output_ of **cdecl**
   based on user input
   is what is of primary importance,
   not the source code itself.

3. **Amount and substantiality of the portion used
   in relation to the copyrighted work as a whole.**

   The table below
   shows a comparison
   between Hansen's version of **cdecl**
   and a recent version of my **cdecl**.
   Hansen's files on the left
   map to my files on the right,
   both with total numbers of lines of code
   and the perecentage of my lines
   that Hansen's version is:
   | Hansen's   | Lines | Lucas's    | Lines | %     |
   | :--------- | ----: | :--------- | ----: | ----: |
   | `cdgram.y` |  855  | `parser.y` |  9288 |  9.2% |
   | `cdlex.l`  |   75  | `lexer.l`  |  1727 |  4.3% |
   | `cdecl.c`  | 1014  | `*.[ch]`   | 40248 |  2.5% |
   | **Total**  | 1944  |            | 51263 |  3.8% |

   Hansen's version of **cdecl** accounts for only 3.8%
   of a recent total
   of the number of lines of code
   of my version of **cdecl**.
   Additionally,
   of that small percentage,
   there are very likely
   no lines that haven't been modified substantially.

4. **Effect of the use upon the potential market for
   or value of the copyrighted work.**

   Given that there are millions of C and C++ developers,
   the potential market is huge;
   however,
   all versions of **cdecl**
   were released as open-source
   and distributed widely
   for free.
   Additionally,
   **cdecl** prior to 3.0
   has limited value
   since it has not kept pace
   with the evolution
   of C and C++.
   Hence,
   it has no market value.

Note, however, that I am not a lawyer;
but my layman's anaysis
seems reasonable.

## Notice of Non-Affiliation

The site `https://github.com/paul-j-lucas/cdecl`
is the _only_ offical site for **cdecl**
versions 3.0 and later.
I am _not_ affiliated with any other site
you might find some version of **cdecl** on.

**Paul J. Lucas**  
San Francisco Bay Area, California, USA  
5 January 2023
