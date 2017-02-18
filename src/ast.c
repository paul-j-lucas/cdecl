/*
**      cdecl -- C gibberish translator
**      src/ast.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "util.h"

// system
#include <stdlib.h>

////////// extern functions ///////////////////////////////////////////////////

void c_object_free( c_object_t *obj ) {
  if ( obj->name )
    free( (void*)obj->name );
  switch ( obj->kind ) {
    case K_ARRAY:
      if ( obj->as.array.of_type )
        c_object_free( obj->as.array.of_type );
      break;
    case K_FUNCTION:
      for ( c_func_arg_t *arg = obj->as.func.arg; arg; arg = arg->next )
        c_object_free( arg->c_obj );
      break;
    case K_MEMBER:
      free( (void*)obj->as.member.class_name );
      c_object_free( obj->as.member.of_type );
      break;
    case K_POINTER:
      c_object_free( obj->as.ptr_to );
      break;
    case K_REFERENCE:
      c_object_free( obj->as.ref_to );
      break;
    default:
      break;
  } // switch
}

c_object_t* c_object_new( c_kind_t kind, char const *name ) {
  c_object_t *const obj = MALLOC( c_object_t, 1 );
  obj->kind = kind;
  obj->name = name ? check_strdup( name ) : NULL;
  return obj;
}

void c_object_english( c_object_t *obj, FILE *fout ) {
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
