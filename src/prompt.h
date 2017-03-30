/*
**      cdecl -- C gibberish translator
**      src/prompt.h
*/

#ifndef cdecl_prompt_H
#define cdecl_prompt_H

// local
#include "config.h"                     /* must go first */

// standard
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * The prompt strings:
 *
 *  + 0 = The primary prompt.
 *  + 1 = The secondary prompt (used for continuation lines).
 */
extern char const  *prompt[2];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Enables or disables the prompt.
 *
 * @param enable If \c true, enables the prompt; else disables it.
 */
void cdecl_prompt_enable( bool enable );

/**
 * Initializes the prompt.
 *
 * @note This is called cdecl_prompt_init and not prompt_init so as not to
 * conflict with the latter function in libedit.
 */
void cdecl_prompt_init( void );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_prompt_H */
/* vim:set et sw=2 ts=2: */
