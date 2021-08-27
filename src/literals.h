/*
**      cdecl -- C gibberish translator
**      src/literals.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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

// local
#include "pjl_config.h"                 /* must go first */
#include "c_lang.h"

/// @cond DOXYGEN_IGNORE

_GL_INLINE_HEADER_BEGIN
#ifndef C_LITERALS_INLINE
# define C_LITERALS_INLINE _GL_INLINE
#endif /* C_LITERALS_INLINE */

/// @endcond

/**
 * @defgroup literals-group Literal Strings
 * Declares constants for cdecl and C/C++ language literals.
 * @{
 */

////////// literals ///////////////////////////////////////////////////////////

// A literal starting with "H_" is pseudo-English for the same literal starting
// with "L_" and are spelled the same except that '-' replaces '_' ("H_" for
// "hyphen").

// If you add a new literal and it is:
//
//  + A cdecl command, update CDECL_COMMANDS in autocomplete.c (if you want it
//    auto-completable).
//
//  + A cdecl or C/C++ keyword, update CDECL_KEYWORDS in autocomplete.c (if you
//    want it auto-completable).

// english
extern char const L_ALIGN[];
extern char const L_ALIGNED[];
extern char const L_ALL[];
extern char const L_ARRAY[];
extern char const L_AS[];
extern char const L_BITS[];
extern char const L_BYTES[];
extern char const L_CAST[];
extern char const L_COMMAND[];            // synonym for "commands"
extern char const L_COMMANDS[];
extern char const L_DECLARE[];
extern char const L_DEFAULTED[];          // synonym for "default"
extern char const L_DEFINE[];
extern char const L_DELETED[];            // synonym for "delete"
extern char const L_ENGLISH[];
extern char const L_EXIT[];
extern char const L_EXPLAIN[];
extern char const L_FUNC[];               // synonym for "function"
extern char const L_FUNCTION[];
extern char const L_HELP[];
extern char const L_INTO[];
extern char const L_LINKAGE[];
extern char const L_MBR[];                // synonym for "member"
extern char const L_MEMBER[];
extern char const H_NON_MBR[];            // synonym for "non-member"
extern char const H_NON_MEMBER[];
extern char const L_OF[];
extern char const L_POINTER[];
extern char const L_PREDEF[];             // synonym for "predefined"
extern char const L_PREDEFINED[];
extern char const L_PTR[];                // synonym for "pointer"
extern char const L_Q[];                  // synonym for "quit"
extern char const L_QUIT[];
extern char const L_REF[];                // synonym for "reference"
extern char const L_REFERENCE[];
extern char const L_RET[];                // synonym for "returning"
extern char const L_RETURNING[];
extern char const L_RVALUE[];
extern char const L_SET_COMMAND[];        // L_SET is synonym for SEEK_SET
extern char const L_SHOW[];
extern char const L_TO[];
extern char const L_USER[];
extern char const L_VECTOR[];             // synonym for "array"
extern char const L_WIDTH[];

// K&R
extern char const L_AUTO[];
extern char const L_AUTOMATIC[];          // synonym for "auto", "__auto_type"
extern char const L_BREAK[];
extern char const L_CASE[];
extern char const L_CHAR[];
extern char const L_CHARACTER[];          // English for "char"
extern char const L_CONTINUE[];
extern char const L_DEFAULT[];
extern char const L_DO[];
extern char const L_DOUBLE[];
extern char const L_ELSE[];
extern char const L_EXTERN[];
extern char const L_EXTERNAL[];           // English for "extern"
extern char const L_FLOAT[];
extern char const L_FOR[];
extern char const L_GOTO[];
extern char const L_IF[];
extern char const L_INT[];
extern char const L_INTEGER[];            // English for "int"
extern char const L_LONG[];
extern char const L_REGISTER[];
extern char const L_RETURN[];
extern char const L_SHORT[];
extern char const L_SIZEOF[];
extern char const L_STATIC[];
extern char const L_STRUCT[];
extern char const L_STRUCTURE[];          // English for "struct"
extern char const L_SWITCH[];
extern char const L_TYPE[];               // English for "typedef"
extern char const L_TYPEDEF[];
extern char const L_UNION[];
extern char const L_UNSIGNED[];
extern char const L_WHILE[];

// C89
extern char const L_ASM[];
extern char const L_CONST[];
extern char const L_CONSTANT[];           // English for "const"
extern char const L_ELLIPSIS[];           // ...
extern char const L_ENUM[];
extern char const L_ENUMERATION[];        // English for "enum"
extern char const L_SIGNED[];
extern char const L_VARARGS[];            // synonym for "..."
extern char const L_VARIADIC[];           // synonym for "..."
extern char const L_VOID[];
extern char const L_VOLATILE[];

// C99
extern char const L__BOOL[];
extern char const L__COMPLEX[];
extern char const L_COMPLEX[];            // synonym for "_Complex"
extern char const L__IMAGINARY[];
extern char const L_IMAGINARY[];          // synonym for "_Imaginary"
extern char const L_INLINE[];
extern char const L_LEN[];                // synonym for "length"
extern char const L_LENGTH[];             // for "variable [length] array"
extern char const L_RESTRICT[];
extern char const L_RESTRICTED[];         // synonym for "restrict"
extern char const L_VAR[];                // synonym for "variable"
extern char const L_VARIABLE[];           // for "variable [length] array"
extern char const L_WCHAR_T[];

// C11
extern char const L__ALIGNAS[];
extern char const L__ALIGNOF[];
extern char const L__ATOMIC[];
extern char const L_ATOMIC[];             // synonym for "_Atomic"
extern char const L__GENERIC[];
extern char const L__NORETURN[];
extern char const H_NO_RETURN[];          // synonym for "_Noreturn"
extern char const H_NON_RETURNING[];      // English for "_Noreturn"
extern char const L__STATIC_ASSERT[];
extern char const L__THREAD_LOCAL[];
extern char const H_THREAD_LOCAL[];       // English for "_Thread_local"

// C++
extern char const L_BOOL[];
extern char const L_CATCH[];
extern char const L_CLASS[];
extern char const L_CONSTRUCTOR[];
extern char const L_CONST_CAST[];
extern char const L_CONV[];               // synonym for "conversion"
extern char const L_CONVERSION[];
extern char const L_CTOR[];               // synonym for "constructor"
extern char const L_DELETE[];
extern char const L_DESTRUCTOR[];
extern char const L_DTOR[];               // synonym for "destructor"
extern char const L_DYNAMIC[];
extern char const L_DYNAMIC_CAST[];
extern char const L_EXPLICIT[];
extern char const L_FALSE[];
extern char const L_FRIEND[];
extern char const L_MUTABLE[];
extern char const L_NAMESPACE[];
extern char const L_NEW[];
extern char const H_NON_THROWING[];       // synonym for "throw"
extern char const L_OPER[];               // synonym for "operator"
extern char const L_OPERATOR[];
extern char const L_PRIVATE[];
extern char const L_PROTECTED[];
extern char const L_PUBLIC[];
extern char const L_PURE[];
extern char const L_REINTERPRET[];
extern char const L_REINTERPRET_CAST[];
extern char const L_SCOPE[];
extern char const L_STATIC_CAST[];
extern char const L_TEMPLATE[];
extern char const L_THIS[];
extern char const L_THROW[];
extern char const L_TRUE[];
extern char const L_TRY[];
extern char const L_TYPEID[];
extern char const L_TYPENAME[];
extern char const L_USING[];
extern char const L_VIRTUAL[];

// C++11
extern char const L_ALIGNAS[];
extern char const L_ALIGNOF[];
extern char const L_CARRIES_DEPENDENCY[];
extern char const H_CARRIES_DEPENDENCY[]; // English for "carries_dependency"
extern char const L_CONSTEXPR[];
extern char const L_DECLTYPE[];
extern char const L_FINAL[];
extern char const L_NOEXCEPT[];
extern char const L_NORETURN[];
extern char const L_LITERAL[];
extern char const H_NO_EXCEPT[];
extern char const H_NO_EXCEPTION[];       // English for "noexcept"
extern char const L_NULLPTR[];
extern char const L_OVERRIDE[];
extern char const L_OVERRIDDEN[];         // English for "override"
extern char const L_STATIC_ASSERT[];
extern char const L_THREAD_LOCAL[];
extern char const H_USER_DEF[];           // synonym for "user-defined"
extern char const H_USER_DEFINED[];

// C11 & C++11
extern char const L_CHAR16_T[];
extern char const L_CHAR32_T[];

// C2X & C++14
extern char const L_DEPRECATED[];
extern char const L___DEPRECATED__[];

// C++17
extern char const L_MAYBE_UNUSED[];
extern char const H_MAYBE_UNUSED[];       // English for "maybe_unused"
extern char const L___MAYBE_UNUSED__[];
extern char const L_NODISCARD[];
extern char const H_NO_DISCARD[];
extern char const L___NODISCARD__[];
extern char const H_NON_DISCARDABLE[];    // English for "nodiscard"

// C++20
extern char const L_CHAR8_T[];
extern char const L_CONCEPT[];
extern char const L_CONSTEVAL[];
extern char const L_CONSTINIT[];
extern char const L_CO_AWAIT[];
extern char const L_CO_RETURN[];
extern char const L_CO_YIELD[];
extern char const L_EXPORT[];
extern char const L_EXPORTED[];           // English for "export"
extern char const L_NO_UNIQUE_ADDRESS[];
extern char const H_NO_UNIQUE_ADDRESS[];
extern char const H_NON_UNIQUE_ADDRESS[];
extern char const L_REQUIRES[];

// Alternative tokens
extern char const L_AND[];                // &&
extern char const L_AND_EQ[];             // &=
extern char const L_BITAND[];             // &
extern char const L_BITOR[];              // |
extern char const L_COMPL[];              // ~
extern char const L_NOT[];                // !
extern char const L_NOT_EQ[];             // !=
extern char const L_OR[];                 // ||
extern char const L_OR_EQ[];              // |=
extern char const L_XOR[];                // ^
extern char const L_XOR_EQ[];             // ^=

// Embedded C extensions
extern char const L_EMC__ACCUM[];
extern char const L_EMC_ACCUM[];          // synonym for "_Accum"
extern char const L_EMC__FRACT[];
extern char const L_EMC_FRACT[];          // synonym for "_Fract"
extern char const L_EMC__SAT[];
extern char const L_EMC_SAT[];            // synonym for "_Sat"
extern char const L_EMC_SATURATED[];      // English for "_Sat"

// Unified Parallel C extensions
extern char const L_UPC_RELAXED[];
extern char const L_UPC_SHARED[];
extern char const L_UPC_STRICT[];

// GNU extensions
extern char const L_GNU___ATTRIBUTE__[];
extern char const L_GNU___AUTO_TYPE[];
extern char const L_GNU___COMPLEX[];
extern char const L_GNU___COMPLEX__[];
extern char const L_GNU___CONST[];
extern char const L_GNU___INLINE[];
extern char const L_GNU___INLINE__[];
extern char const L_GNU___RESTRICT[];
extern char const L_GNU___RESTRICT__[];
extern char const L_GNU___SIGNED[];
extern char const L_GNU___SIGNED__[];
extern char const L_GNU___THREAD[];
extern char const L_GNU___VOLATILE[];
extern char const L_GNU___VOLATILE__[];

// Apple extensions
extern char const L_APPLE___BLOCK[];      // storage class
extern char const L_APPLE_BLOCK[];        // Engligh for '^'

// Microsoft extensions
extern char const L_MSC___ASM[];
extern char const L_MSC___CDECL[];
extern char const L_MSC_CDECL[];
extern char const L_MSC___CLRCALL[];
extern char const L_MSC_CLRCALL[];
extern char const L_MSC___DECLSPEC[];
extern char const L_MSC___FASTCALL[];
extern char const L_MSC_FASTCALL[];
extern char const L_MSC___STDCALL[];
extern char const L_MSC_STDCALL[];
extern char const L_MSC___THISCALL[];
extern char const L_MSC_THISCALL[];
extern char const L_MSC___VECTORCALL[];
extern char const L_MSC_VECTORCALL[];
extern char const L_MSC_WINAPI[];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the alignas literal for the current language.
 *
 * @return Returns either `_Alignas` (for C) or `alignas` (for C++).
 */
C_LITERALS_INLINE PJL_WARN_UNUSED_RESULT
char const* alignas_lang( void ) {
  return OPT_LANG_IS(C_ANY) ? L__ALIGNAS : L_ALIGNAS;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_literals_H */
/* vim:set et sw=2 ts=2: */
