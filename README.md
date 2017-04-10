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
  `thread_local`,
  `virtual`,
  and
  `wchar_t`.
* Support for `typedef` declarations.
* Support for C++11 rvalue references.
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

(See the included `README-orig.txt` for David's full comments.)

One of David's contributions was adding GNU readline support.
However, GNU readline is under the GNU General Public License (GPL).
That means that any program that links with GNU readline
*must* also be under the GPL.

Something in the Public Domain allows anyone to do whatever they want with it.
Assuming that David is correct
and that **cdecl** is in the Public Domain,
and given the GNU readline GPL issue,
I therefore am licencing **cdecl** 3.0 under the GPLv3.

**Paul J. Lucas**  
*paul@lucasmail.org*  
San Francisco, California, USA  
8 April 2017
