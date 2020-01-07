/*
**      cdecl -- C gibberish translator
**      src/literals.h
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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

/**
 * @defgroup literals-group Literal Strings
 * Constants for cdecl and C/C++ language literals.
 * @{
 */

////////// literals ///////////////////////////////////////////////////////////

// english
extern char const L_ALIGNED[];
extern char const L_ALL[];
extern char const L_ALT_TOKENS[];
extern char const L_ARRAY[];
extern char const L_AS[];
extern char const L_BLOCK[];                // Apple: Engligh for '^'
extern char const L_BYTES[];
extern char const L_CAST[];
extern char const L_COMMAND[];              // synonym for "commands"
extern char const L_COMMANDS[];
extern char const L_DECLARE[];
extern char const L_DEFAULTED[];            // synonym for "default"
extern char const L_DEFINE[];
extern char const L_DELETED[];              // synonym for "delete"
extern char const L_ENGLISH[];
extern char const L_EXIT[];
extern char const L_EXPLAIN[];
extern char const L_FUNC[];                 // synonym for "function"
extern char const L_FUNCTION[];
extern char const L_HELP[];
extern char const L_INTO[];
extern char const L_MBR[];                  // synonym for "member"
extern char const L_MEMBER[];
extern char const L_NON_MBR[];              // synonym for "non-member"
extern char const L_NON_MEMBER[];
extern char const L_OF[];
extern char const L_POINTER[];
extern char const L_PREDEF[];               // synonym for "predefined"
extern char const L_PREDEFINED[];
extern char const L_PTR[];                  // synonym for "pointer"
extern char const L_Q[];                    // synonym for "quit"
extern char const L_QUIT[];
extern char const L_REF[];                  // synonym for "reference"
extern char const L_REFERENCE[];
extern char const L_RET[];                  // synonym for "returning"
extern char const L_RETURNING[];
extern char const L_RVALUE[];
extern char const L_SET_COMMAND[];          // L_SET is synonym for SEEK_SET
extern char const L_SHOW[];
extern char const L_TO[];
extern char const L_USER[];
extern char const L_VECTOR[];               // synonym for "array"

// K&R
extern char const L_AUTO[];
extern char const L_AUTOMATIC[];            // synonym for "auto"
extern char const L_BREAK[];
extern char const L_CASE[];
extern char const L_CHAR[];
extern char const L_CHARACTER[];            // synonym for "char"
extern char const L_CONTINUE[];
extern char const L_DEFAULT[];
extern char const L_DO[];
extern char const L_DOUBLE[];
extern char const L_ELSE[];
extern char const L_EXTERN[];
extern char const L_EXTERNAL[];             // synonym for "extern"
extern char const L_FLOAT[];
extern char const L_FOR[];
extern char const L_GOTO[];
extern char const L_IF[];
extern char const L_INT[];
extern char const L_INTEGER[];              // synonym for "int"
extern char const L_LONG[];
extern char const L_REGISTER[];
extern char const L_RETURN[];
extern char const L_SHORT[];
extern char const L_SIZEOF[];
extern char const L_STATIC[];
extern char const L_STRUCT[];
extern char const L_STRUCTURE[];            // synonym for "struct"
extern char const L_SWITCH[];
extern char const L_TYPE[];                 // synonym for "typedef"
extern char const L_TYPEDEF[];
extern char const L_UNION[];
extern char const L_UNSIGNED[];
extern char const L_WHILE[];

// C89
extern char const L_CONST[];
extern char const L_CONSTANT[];             // synonym for "const"
extern char const L_ELLIPSIS[];             // ...
extern char const L_ENUM[];
extern char const L_ENUMERATION[];          // synonym for "enum"
extern char const L_SIGNED[];
extern char const L_VARARGS[];              // synonym for "..."
extern char const L_VARIADIC[];             // synonym for "..."
extern char const L_VOID[];
extern char const L_VOLATILE[];

// C99
extern char const L_BOOL[];
extern char const L__BOOL[];                // synonym for "bool"
extern char const L__COMPLEX[];
extern char const L_COMPLEX[];              // synonym for "_Complex"
extern char const L__IMAGINARY[];
extern char const L_IMAGINARY[];            // synonym for "_Imaginary"
extern char const L_INLINE[];
extern char const L_LEN[];                  // synonym for "length"
extern char const L_LENGTH[];
extern char const L_RESTRICT[];
extern char const L_RESTRICTED[];           // synonym for "restrict"
extern char const L_VAR[];                  // synonym for "variable"
extern char const L_VARIABLE[];
extern char const L_WCHAR_T[];

// C11
extern char const L__ALIGNAS[];
extern char const L__ALIGNOF[];
extern char const L__ATOMIC[];
extern char const L_ATOMIC[];               // synonym for "_Atomic"
extern char const L__GENERIC[];
extern char const L__NORETURN[];
extern char const L_NON_RETURNING_ENG[];    // English for "_Noreturn"
extern char const L_NORETURN[];             // synonym for "_Noreturn"
extern char const L__STATIC_ASSERT[];
extern char const L_STATIC_ASSERT[];

// C++
extern char const L_CATCH[];
extern char const L_CLASS[];
extern char const L_CONSTRUCTOR[];
extern char const L_CONST_CAST[];
extern char const L_CONVERSION[];
extern char const L_DELETE[];
extern char const L_DESTRUCTOR[];
extern char const L_DYNAMIC[];
extern char const L_DYNAMIC_CAST[];
extern char const L_EXPLICIT[];
extern char const L_EXPORT[];
extern char const L_FALSE[];
extern char const L_FRIEND[];
extern char const L_MUTABLE[];
extern char const L_NAMESPACE[];
extern char const L_NEW[];
extern char const L_NON_THROWING_ENG[];     // synonym for "throw"
extern char const L_OPER[];                 // synonym for "operator"
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
extern char const L_CARRIES_DEPENDENCY_ENG[];
extern char const L_CONSTEXPR[];
extern char const L_DECLTYPE[];
extern char const L_FINAL[];
extern char const L_NOEXCEPT[];
extern char const L_LITERAL[];
extern char const L_NO_EXCEPTION_ENG[];     // English for "noexcept"
extern char const L_NULLPTR[];
extern char const L_OVERRIDE[];
extern char const L_OVERRIDDEN_ENG[];       // English for "override"
extern char const L_USER_DEFINED[];

// C11 & C++11
extern char const L_CHAR16_T[];
extern char const L_CHAR32_T[];
extern char const L_THREAD_LOCAL[];
extern char const L_THREAD_LOCAL_ENG[];     // English for "thread_local"
extern char const L__THREAD_LOCAL[];        // synonym for "thread_local"

// C++14
extern char const L_DEPRECATED[];

// C++17
extern char const L_MAYBE_UNUSED[];
extern char const L_MAYBE_UNUSED_ENG[];     // English for "maybe_unused"
extern char const L_NODISCARD[];
extern char const L_NON_DISCARDABLE_ENG[];  // English for "nodiscard"

// C++20
extern char const L_CHAR8_T[];
extern char const L_CONCEPT[];
extern char const L_CONSTEVAL[];
extern char const L_REQUIRES[];

// Alternative tokens
extern char const L_AND[];                  // &&
extern char const L_AND_EQ[];               // &=
extern char const L_BITAND[];               // &
extern char const L_BITOR[];                // |
extern char const L_COMPL[];                // ~
extern char const L_NOT[];                  // !
extern char const L_NOT_EQ[];               // !=
extern char const L_OR[];                   // ||
extern char const L_OR_EQ[];                // |=
extern char const L_XOR[];                  // ^
extern char const L_XOR_EQ[];               // ^=

// Miscellaneous
extern char const L___BLOCK[];              // Apple: storage class

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_literals_H */
/* vim:set et sw=2 ts=2: */
