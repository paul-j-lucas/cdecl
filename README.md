# cdecl

> *I’m still uncertain about the language declaration syntax, where in
> declarations, syntax is used that mimics the use of the variables being
> declared.  It is one of the things that draws strong criticism, but it has a
> certain logic to it.*

<div style="text-align: center">
-- Dennis M. Ritchie
</div>
<p></p>

> *I consider the C declarator syntax an experiment that failed.*

<div style="text-align: center">
-- Bjarne Stroustrup
</div>
<p></p>

**cdecl** (_see-deh-kull_)
is a program for composing and deciphering C (or C++)
type declarations or casts, aka ‘‘gibberish.’’
It can be used interactively on a terminal or
accept input from either the command line or standard input.

This version fixes virtually all the deficiencies in earlier versions
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
  C18,
  C++98,
  C++03,
  C++11,
  C++14,
  C++17,
  and
  C++20.
* Support for C89
  `const`,
  `restrict`,
  and
  `volatile`
  declarations.
* Support for the standard C95 type
  `wchar_t`.
* Support for the standard C99 types
  `_Bool`,
  `_Complex`,
  `_Imaginary`,
  `int8_t`,
  `int16_t`,
  `ptrdiff_t`,
  `size_t`,
  etc.
* Support for C99
  `static`,
  type-qualified,
  and
  variable length array
  function arguments.
* Support for the standard C11 atomic types
  `atomic_bool`,
  `atomic_char`,
  etc.
* Support for the standard C11 and C++11 types
  `char16_t`,
  `char32_t`,
  and
  `thread_local`.
* Support for `inline` function and variable declarations.
* Support for `typedef` declarations.
* Support for variadic function arguments.
* Support for C++
  `mutable` data members,
  new-style casts,
  `throw`,
  overloaded operators,
  and
  `friend`,
  `virtual`,
  and
  pure `virtual`
  member function declarations.
* Support for C++11
  `constexpr`,
  `enum class`,
  `final`,
  `noexcept`,
  `override`,
  rvalue references,
  `using` (as a `typedef` synonym),
  the function trailing return-type syntax,
  and
  ref-qualified member function declarations.
* Support for C++
  `[[carries_dependency]]`,
  `[[deprecated]]`,
  `[[maybe_unused]]`,
  `[[nodiscard]]`,
  and
  `[[noreturn]]`
  attribute specifiers.
* Better warning and error messages
  complete with location information and color.

## Installation

The git repository contains only the necessary source code.
Things like `configure` are _derived_ sources and
[should not be included in repositories](http://stackoverflow.com/a/18732931).
If you have `autoconf`, `automake`, and `m4` installed,
you can generate `configure` yourself by doing:

    autoreconf -fiv

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
