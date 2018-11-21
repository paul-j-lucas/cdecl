/*
**      cdecl -- C gibberish translator
**      src/literals.c
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

/**
 * @file
 * Defines constants for cdecl and C/C++ language literals.
 */

///////////////////////////////////////////////////////////////////////////////

// english
char const L_ALL[]                  = "all";
char const L_ARRAY[]                = "array";
char const L_AS[]                   = "as";
char const L_BLOCK[]                = "block";    // Apple: English for '^'
char const L_CAST[]                 = "cast";
char const L_DECLARE[]              = "declare";
char const L_DEFINE[]               = "define";
char const L_EXIT[]                 = "exit";
char const L_EXPLAIN[]              = "explain";
char const L_FUNC[]                 = "func";
char const L_FUNCTION[]             = "function";
char const L_HELP[]                 = "help";
char const L_INTO[]                 = "into";
char const L_MBR[]                  = "mbr";
char const L_MEMBER[]               = "member";
char const L_OF[]                   = "of";
char const L_POINTER[]              = "pointer";
char const L_PREDEF[]               = "predef";
char const L_PREDEFINED[]           = "predefined";
char const L_PTR[]                  = "ptr";
char const L_Q[]                    = "q";
char const L_QUIT[]                 = "quit";
char const L_REF[]                  = "ref";
char const L_REFERENCE[]            = "reference";
char const L_RET[]                  = "ret";
char const L_RETURNING[]            = "returning";
char const L_RVALUE[]               = "rvalue";
char const L_SET[]                  = "set";
char const L_SHOW[]                 = "show";
char const L_TO[]                   = "to";
char const L_USER[]                 = "user";
char const L_VECTOR[]               = "vector";

// K&R
char const L_AUTO[]                 = "auto";
char const L_AUTOMATIC[]            = "automatic";
char const L_BREAK[]                = "break";
char const L_CASE[]                 = "case";
char const L_CHAR[]                 = "char";
char const L_CHARACTER[]            = "character";
char const L_CONTINUE[]             = "continue";
char const L_DEFAULT[]              = "default";
char const L_DO[]                   = "do";
char const L_DOUBLE[]               = "double";
char const L_ELSE[]                 = "else";
char const L_EXTERN[]               = "extern";
char const L_EXTERNAL[]             = "external";
char const L_FLOAT[]                = "float";
char const L_FOR[]                  = "for";
char const L_GOTO[]                 = "goto";
char const L_IF[]                   = "if";
char const L_INT[]                  = "int";
char const L_INTEGER[]              = "integer";
char const L_LONG[]                 = "long";
char const L_REGISTER[]             = "register";
char const L_RETURN[]               = "return";
char const L_SHORT[]                = "short";
char const L_SIZEOF[]               = "sizeof";
char const L_STATIC[]               = "static";
char const L_STRUCT[]               = "struct";
char const L_STRUCTURE[]            = "structure";
char const L_SWITCH[]               = "switch";
char const L_TYPE[]                 = "type";
char const L_TYPEDEF[]              = "typedef";
char const L_UNION[]                = "union";
char const L_UNSIGNED[]             = "unsigned";
char const L_WHILE[]                = "while";

// C89
char const L_CONST[]                = "const";
char const L_CONSTANT[]             = "constant";
char const L_ELLIPSIS[]             = "...";
char const L_ENUM[]                 = "enum";
char const L_ENUMERATION[]          = "enumeration";
char const L_SIGNED[]               = "signed";
char const L_VARARGS[]              = "varargs";
char const L_VARIADIC[]             = "variadic";
char const L_VOID[]                 = "void";
char const L_VOLATILE[]             = "volatile";

// C99
char const L_BOOL[]                 = "bool";
char const L__BOOL[]                = "_Bool";
char const L__COMPLEX[]             = "_Complex";
char const L_COMPLEX[]              = "complex";
char const L__IMAGINARY[]           = "_Imaginary";
char const L_IMAGINARY[]            = "imaginary";
char const L_INLINE[]               = "inline";
char const L_LEN[]                  = "len";
char const L_LENGTH[]               = "length";
char const L_RESTRICT[]             = "restrict";
char const L_RESTRICTED[]           = "restricted";
char const L_VAR[]                  = "var";
char const L_VARIABLE[]             = "variable";
char const L_WCHAR_T[]              = "wchar_t";

// C11
char const L__ALIGNAS[]             = "_Alignas";
char const L__ALIGNOF[]             = "_Alignof";
char const L__ATOMIC[]              = "_Atomic";
char const L_ATOMIC[]               = "atomic";
char const L__GENERIC[]             = "_Generic";
char const L__NORETURN[]            = "_Noreturn";
char const L_NON_RETURNING[]        = "non-returning";
char const L_NORETURN[]             = "noreturn";
char const L__STATIC_ASSERT[]       = "_Static_assert";
char const L_STATIC_ASSERT[]        = "static_assert";

// C++
char const L_AND[]                  = "and";
char const L_AND_EQ[]               = "and_eq";
char const L_BITAND[]               = "bitand";
char const L_BITOR[]                = "bitor";
char const L_CATCH[]                = "catch";
char const L_CLASS[]                = "class";
char const L_COMPL[]                = "compl";
char const L_CONST_CAST[]           = "const_cast";
char const L_DELETE[]               = "delete";
char const L_DYNAMIC[]              = "dynamic";
char const L_DYNAMIC_CAST[]         = "dynamic_cast";
char const L_EXPLICIT[]             = "explicit";
char const L_EXPORT[]               = "export";
char const L_FALSE[]                = "false";
char const L_FRIEND[]               = "friend";
char const L_MUTABLE[]              = "mutable";
char const L_NAMESPACE[]            = "namespace";
char const L_NEW[]                  = "new";
char const L_NON_THROWING[]         = "non-throwing";
char const L_NOT[]                  = "not";
char const L_NOT_EQ[]               = "not_eq";
char const L_OR[]                   = "or";
char const L_OR_EQ[]                = "or_eq";
char const L_PRIVATE[]              = "private";
char const L_PROTECTED[]            = "protected";
char const L_PUBLIC[]               = "public";
char const L_PURE[]                 = "pure";
char const L_REINTERPRET[]          = "reinterpret";
char const L_REINTERPRET_CAST[]     = "reinterpret_cast";
char const L_STATIC_CAST[]          = "static_cast";
char const L_TEMPLATE[]             = "template";
char const L_THIS[]                 = "this";
char const L_THROW[]                = "throw";
char const L_TRUE[]                 = "true";
char const L_TRY[]                  = "try";
char const L_TYPEID[]               = "typeid";
char const L_TYPENAME[]             = "typename";
char const L_USING[]                = "using";
char const L_VIRTUAL[]              = "virtual";
char const L_XOR[]                  = "xor";
char const L_XOR_EQ[]               = "xor_eq";

// C++11
char const L_ALIGNAS[]              = "alignas";
char const L_ALIGNOF[]              = "alignof";
char const L_CARRIES_DEPENDENCY[]   = "carries_dependency";
char const L_CARRIES_DEPENDENCY2[]  = "carries-dependency";
char const L_CONSTEXPR[]            = "constexpr";
char const L_DECLTYPE[]             = "decltype";
char const L_FINAL[]                = "final";
char const L_NOEXCEPT[]             = "noexcept";
char const L_NULLPTR[]              = "nullptr";
char const L_NO_EXCEPTION[]         = "no-exception";
char const L_OVERRIDE[]             = "override";
char const L_OVERRIDDEN[]           = "overridden";

// C11 & C++11
char const L_CHAR16_T[]             = "char16_t";
char const L_CHAR32_T[]             = "char32_t";
char const L_THREAD_LOCAL[]         = "thread_local";
char const L__THREAD_LOCAL[]        = "_Thread_local";

// C++14
char const L_DEPRECATED[]           = "deprecated";

// C++17
char const L_MAYBE_UNUSED[]         = "maybe_unused";
char const L_MAYBE_UNUSED2[]        = "maybe-unused";
char const L_NODISCARD[]            = "nodiscard";
char const L_NON_DISCARDABLE[]      = "non-discardable";

// Miscellaneous
char const L___BLOCK[]              = "__block";  // Apple: block storage class

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
