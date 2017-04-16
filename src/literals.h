/*
**      cdecl -- C gibberish translator
**      src/literals.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef cdecl_literals_H
#define cdecl_literals_H

/**
 * @file
 * Declares constants for cdecl and C/C++ language literals.
 */

////////// literals ///////////////////////////////////////////////////////////

// english
extern char const L_ARRAY[];
extern char const L_AS[];
extern char const L_BLOCK[];            // Apple: Engligh for '^'
extern char const L_CAST[];
extern char const L_DECLARE[];
extern char const L_EXIT[];
extern char const L_EXPLAIN[];
extern char const L_FUNC[];             // synonym for "function"
extern char const L_FUNCTION[];
extern char const L_HELP[];
extern char const L_INTO[];
extern char const L_MBR[];              // synonym for "member"
extern char const L_MEMBER[];
extern char const L_OF[];
extern char const L_POINTER[];
extern char const L_PTR[];              // synonym for "pointer"
extern char const L_Q[];                // synonym for "quit"
extern char const L_QUIT[];
extern char const L_REF[];              // synonym for "reference"
extern char const L_REFERENCE[];
extern char const L_RET[];              // synonym for "returning"
extern char const L_RETURNING[];
extern char const L_RVALUE[];
extern char const L_SET[];
extern char const L_TO[];
extern char const L_VECTOR[];           // synonym for "array"

// K&R
extern char const L_AUTO[];
extern char const L_CHAR[];
extern char const L_CHARACTER[];        // synonym for "char"
extern char const L_DOUBLE[];
extern char const L_EXTERN[];
extern char const L_FLOAT[];
extern char const L_INT[];
extern char const L_INTEGER[];          // synonym for "int"
extern char const L_LONG[];
extern char const L_REGISTER[];
extern char const L_SHORT[];
extern char const L_STATIC[];
extern char const L_STRUCT[];
extern char const L_STRUCTURE[];        // synonym for "struct"
extern char const L_TYPE[];             // synonym for "typedef"
extern char const L_TYPEDEF[];
extern char const L_UNION[];
extern char const L_UNSIGNED[];

// C89
extern char const L_CONST[];
extern char const L_CONSTANT[];         // synonym for "const"
extern char const L_ELLIPSIS[];         // ...
extern char const L_ENUM[];
extern char const L_ENUMERATION[];      // synonym for "enum"
extern char const L_SIGNED[];
extern char const L_SIZE_T[];
extern char const L_VARARGS[];          // synonym for "..."
extern char const L_VARIADIC[];         // synonym for "..."
extern char const L_VOID[];
extern char const L_VOLATILE[];

// C99
extern char const L_BOOL[];
extern char const L__BOOL[];            // synonym for "bool"
extern char const L_COMPLEX[];
extern char const L__COMPLEX[];         // synonym for "complex"
extern char const L_RESTRICT[];
extern char const L_RESTRICTED[];       // synonym for "restrict"
extern char const L_WCHAR_T[];

// C11
extern char const L_NORETURN[];
extern char const L_NON_RETURNING[];    // synonym for "noreturn"
extern char const L__NORETURN[];        // synonym for "noreturn"

// C++
extern char const L_CLASS[];
extern char const L_FRIEND[];
extern char const L_PURE[];
extern char const L_VIRTUAL[];

// C++11
extern char const L_CONSTEXPR[];
extern char const L_FINAL[];
extern char const L_OVERRIDE[];
extern char const L_OVERRIDDEN[];       // synonym for "override"

// C11 & C++11
extern char const L_CHAR16_T[];
extern char const L_CHAR32_T[];
extern char const L_THREAD_LOCAL[];
extern char const L__THREAD_LOCAL[];    // synonym for "thread_local"

// miscellaneous
extern char const L___BLOCK[];          // Apple: storage class

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_literals_H */
/* vim:set et sw=2 ts=2: */
