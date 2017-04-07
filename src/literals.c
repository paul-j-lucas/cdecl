/*
**      cdecl -- C gibberish translator
**      src/literals.c
**
**      Paul J. Lucas
*/

///////////////////////////////////////////////////////////////////////////////

// english
char const L_ARRAY[]          = "array";
char const L_AS[]             = "as";
char const L_BLOCK[]          = "block";    // Apple: English for '^'
char const L_CAST[]           = "cast";
char const L_DECLARE[]        = "declare";
char const L_EXIT[]           = "exit";
char const L_EXPLAIN[]        = "explain";
char const L_FUNC[]           = "func";
char const L_FUNCTION[]       = "function";
char const L_HELP[]           = "help";
char const L_INTO[]           = "into";
char const L_MBR[]            = "mbr";
char const L_MEMBER[]         = "member";
char const L_OF[]             = "of";
char const L_POINTER[]        = "pointer";
char const L_PTR[]            = "ptr";
char const L_Q[]              = "q";
char const L_QUIT[]           = "quit";
char const L_REF[]            = "ref";
char const L_REFERENCE[]      = "reference";
char const L_RET[]            = "ret";
char const L_RETURNING[]      = "returning";
char const L_RVALUE[]         = "rvalue";
char const L_SET[]            = "set";
char const L_TO[]             = "to";
char const L_VECTOR[]         = "vector";

// K&R
char const L_AUTO[]           = "auto";
char const L_CHAR[]           = "char";
char const L_CHARACTER[]      = "character";
char const L_DOUBLE[]         = "double";
char const L_EXTERN[]         = "extern";
char const L_FLOAT[]          = "float";
char const L_INT[]            = "int";
char const L_INTEGER[]        = "integer";
char const L_LONG[]           = "long";
char const L_REGISTER[]       = "register";
char const L_SHORT[]          = "short";
char const L_STATIC[]         = "static";
char const L_STRUCT[]         = "struct";
char const L_STRUCTURE[]      = "structure";
char const L_TYPE[]           = "type";
char const L_TYPEDEF[]        = "typedef";
char const L_UNION[]          = "union";
char const L_UNSIGNED[]       = "unsigned";

// C89
char const L_CONST[]          = "const";
char const L_CONSTANT[]       = "constant";
char const L_ELLIPSIS[]       = "...";
char const L_ENUM[]           = "enum";
char const L_ENUMERATION[]    = "enumeration";
char const L_SIGNED[]         = "signed";
char const L_VARARGS[]        = "varargs";
char const L_VARIADIC[]       = "variadic";
char const L_VOID[]           = "void";
char const L_VOLATILE[]       = "volatile";

// C99
char const L__BOOL[]          = "_Bool";
char const L_BOOL[]           = "bool";
char const L__COMPLEX[]       = "_Complex";
char const L_COMPLEX[]        = "complex";
char const L_RESTRICT[]       = "restrict";
char const L_RESTRICTED[]     = "restricted";
char const L_WCHAR_T[]        = "wchar_t";

// C11
char const L__NORETURN[]      = "_Noreturn";
char const L_NORETURN[]       = "Noreturn";
char const L__THREAD_LOCAL[]  = "_Thread_local";

// C++
char const L_CLASS[]          = "class";
char const L_PURE[]           = "pure";
char const L_VIRTUAL[]        = "virtual";

// C11 & C++11
char const L_CHAR16_T[]       = "char16_t";
char const L_CHAR32_T[]       = "char32_t";
char const L_THREAD_LOCAL[]   = "thread_local";

// Miscellaneous
char const L___BLOCK[]        = "__block";  // Apple: block storage class

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
