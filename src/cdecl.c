/*
**    cdecl -- C gibberish translator
**    src/cdecl.c
*/

// local
#include "config.h"
#include "cdecl.h"
#include "cdgram.h"
#include "options.h"
#include "util.h"

// standard
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

extern FILE *yyin;

int yyparse( void );

#ifdef USE_READLINE
# include <readline/readline.h>
  /* prototypes for functions related to readline() */
char*   getline();
char**  attempt_completion(char *, int, int);
char*   keyword_completion(char *, int);
char*   command_completion(char *, int);
#endif

/* maximum # of chars from progname to display in prompt */
#define MAX_NAME 32

/* this is the prompt for readline() to display */
char cdecl_prompt[MAX_NAME+3];

/* backup copy of prompt to save it while prompting is off */
char real_prompt[MAX_NAME+3];

char *cat(char const *, ...);
int dostdin(void);
void c_type_check(void);
void prompt(void), doprompt(void), noprompt(void);
void unsupp(char const*, char const*);
void notsupported(char const *, char const *, char const *);
void doset(char const *);
void dodeclare(char*, char*, char*, char*, char*);
void docast(char*, char*, char*, char*);
void dodexplain(char*, char*, char*, char*, char*);
void docexplain(char*, char*, char*, char*);
void cdecl_setprogname(char *);
int dotmpfile(int, char const**), dofileargs(int, char const**);

FILE *tmpfile();

char const *me; // program name

/* variables used during parsing */
unsigned modbits = 0;
int arbdims = 1;
char const *savedname = 0;
char const unknown_name[] = "unknown_name";
char prev = 0;    /* the current type of the variable being examined */
                  /*    values  type           */
                  /*  p pointer          */
                  /*  r reference        */
                  /*  f function         */
                  /*  a array (of arbitrary dimensions)    */
                  /*  A array with dimensions      */
                  /*  n name           */
                  /*  v void           */
                  /*  s struct | class         */
                  /*  t simple type (int, long, etc.)    */

/* options */
bool MkProgramFlag;                     // -c, output {} and ; after declarations
bool opt_pre_ansi_c;                    // -p, assume pre-ANSI C language
bool OnATty;                            // stdin is coming from a terminal
int KeywordName;                        // $0 is a keyword (declare, explain, cast)
char const *progname = "cdecl";         // $0
bool quiet;                             // -q, quiets prompt and initial help msg

#if dodebug
bool DebugFlag = 0;    /* -d, output debugging trace info */
#endif

#ifdef doyydebug    /* compile in yacc trace statements */
#define YYDEBUG 1
#endif /* doyydebug */

/* the names and bits checked for each row in the above array */
struct c_type_info {
  char const *name;
  unsigned    bit;
};
typedef struct c_type_info c_type_info_t;

c_type_info_t const C_TYPE_INFO[] = {
  { "void",     C_TYPE_VOID     },
  { "bool",     C_TYPE_BOOL     },
  { "char",     C_TYPE_CHAR     },
  { "wchar_t",  C_TYPE_WCHAR_T  },
  { "short",    C_TYPE_SHORT    },
  { "int",      C_TYPE_INT      },
  { "long",     C_TYPE_LONG     },
  { "signed",   C_TYPE_SIGNED   },
  { "unsigned", C_TYPE_UNSIGNED },
  { "float",    C_TYPE_FLOAT    },
  { "double",   C_TYPE_DOUBLE   }
};

/* definitions (and abbreviations) for type combinations cross check table */
#define ALWAYS  0                       // always okay 
#define NEVER   1                       // never allowed 
#define KNR     3                       // not allowed in Pre-ANSI compiler 
#define ANSI    4                       // not allowed anymore in ANSI compiler 

#define _ ALWAYS
#define X NEVER
#define K KNR
#define A ANSI

static char const crosscheck[][ ARRAY_SIZE( C_TYPE_INFO ) ] = {
  /*               v b c w s i l s u f d */
  /* void     */ { _,X,X,X,X,X,X,X,X,X,X },
  /* bool     */ { X,_,X,X,X,X,X,X,X,X,X },
  /* char     */ { X,X,_,X,X,X,X,_,_,X,X },
  /* wchar_t  */ { X,X,X,_,X,X,X,X,X,X,X },
  /* short    */ { X,X,X,X,X,_,X,_,_,X,X },
  /* int      */ { X,X,X,X,_,X,_,_,_,X,X },
  /* long     */ { X,X,X,X,X,_,X,_,_,X,_ },
  /* signed   */ { X,X,_,X,_,_,_,X,X,X,X },
  /* unsigned */ { X,X,_,X,_,_,_,X,X,X,X },
  /* float    */ { X,X,X,X,X,X,X,X,X,X,X },
  /* double   */ { X,X,X,X,X,X,_,X,X,X,X }
};

#if 0
/* This is an lower left triangular array. If we needed */
/* to save 9 bytes, the "long" row can be removed. */
char const crosscheck_old[9][9] = {
  /*              L, I, S, C, V, U, S, F, D, */
  /* long */      _, _, _, _, _, _, _, _, _,
  /* int */       _, _, _, _, _, _, _, _, _,
  /* short */     X, _, _, _, _, _, _, _, _,
  /* char */      X, X, X, _, _, _, _, _, _,
  /* void */      X, X, X, X, _, _, _, _, _,
  /* unsigned */  R, _, R, R, X, _, _, _, _,
  /* signed */    K, K, K, K, X, X, _, _, _,
  /* float */     A, X, X, X, X, X, X, _, _,
  /* double */    K, X, X, X, X, X, X, X, _
};
#endif


/* Run through the crosscheck array looking */
/* for unsupported combinations of types. */
void c_type_check() {

  /* Loop through the types */
  /* (skip the "long" row) */
  for ( size_t i = 1; i < ARRAY_SIZE( C_TYPE_INFO ); ++i ) {
    /* if this type is in use */
    if ((modbits & C_TYPE_INFO[i].bit) != 0) {
      /* check for other types also in use */
      for ( size_t j = 0; j < i; ++j ) {
        /* this type is not in use */
        if (!(modbits & C_TYPE_INFO[j].bit))
            continue;
        /* check the type of restriction */
        int restriction = crosscheck[i][j];
        if (restriction == ALWAYS)
            continue;
        char const *t1 = C_TYPE_INFO[i].name;
        char const *t2 = C_TYPE_INFO[j].name;
        if (restriction == NEVER) {
          notsupported("", t1, t2);
        }
        else if (restriction == KNR) {
          if (opt_pre_ansi_c)
            notsupported(" (Pre-ANSI Compiler)", t1, t2);
        }
        else if (restriction == ANSI) {
          if (!opt_pre_ansi_c)
            notsupported(" (ANSI Compiler)", t1, t2);
        }
        else {
          fprintf (stderr,
            "%s: Internal error in crosscheck[%zu,%zu]=%d!\n",
            progname, i, j, restriction);
          exit(1);
        }
      } // for
    }
  } // for
}

/* undefine these as they are no longer needed */
#undef _
#undef ALWAYS
#undef X
#undef NEVER
#undef P
#undef KNR
#undef A
#undef ANSI

#ifdef USE_READLINE

/* this section contains functions and declarations used with readline() */

/* the readline info pages make this clearer than any comments possibly
 * could, so see them for more information
 */

char const *const commands[] = {
  "declare",
  "explain",
  "cast",
  "help",
  "set",
  "exit",
  "quit",
  NULL
};

char const *const keywords[] = {
  "function",
  "returning",
  "array",
  "pointer",
  "reference",
  "member",
  "const",
  "volatile",
  "noalias",
  "struct",
  "union",
  "enum",
  "class",
  "extern",
  "static",
  "auto",
  "register",
  "short",
  "long",
  "signed",
  "unsigned",
  "char",
  "float",
  "double",
  "void",
  NULL
};

const char *const options[] = {
  "options",
  "create",
  "nocreate",
  "prompt",
  "noprompt",
#if 0
  "interactive",
  "nointeractive",
#endif
  "preansi",
  "ansi",
  "cplusplus",
  NULL
};

/* A static variable for holding the line. */
static char *line_read = NULL;

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char * getline ()
{
  /* If the buffer has already been allocated, return the memory
     to the free pool. */
  if (line_read != NULL) {
    free (line_read);
    line_read = NULL;
  }

  /* Get a line from the user. */
  line_read = readline (cdecl_prompt);

  /* If the line has any text in it, save it on the history. */
  if (line_read && *line_read)
    add_history (line_read);

  return (line_read);
}

char ** attempt_completion(char *text, int start, int end) {
  char **matches = NULL;
  if ( start == 0 )
    matches = completion_matches(text, command_completion);
  return matches;
}

char* command_completion(char *text, int flag)
{
  static int index, len;
  char const *command;

  if ( !flag ) {
    index = 0;
    len = strlen(text);
  }

  while ( command = commands[index] ) {
    ++index;
    if ( strncmp( command, text, len ) == 0 )
      return strdup(command);
  }
  return NULL;
}

char * keyword_completion(char *text, int flag)
{
  static int index, len, set, into;
  char *keyword, *option;

  if (!flag) {
    index = 0;
    len = strlen(text);
    /* completion works differently if the line begins with "set" */
    set = !strncmp(rl_line_buffer, "set", 3);
    into = 0;
  }

  if (set) {
    while (option = options[index]) {
      index++;
      if (!strncmp(option, text, len)) return strdup(option);
    }
  } else {
    /* handle "int" and "into" as special cases */
    if (!into) {
      into = 1;
      if (!strncmp(text, "into", len) && strncmp(text, "int", len))
        return strdup("into");
      if (strncmp(text, "int", len)) return keyword_completion(text, into);
      /* normally "int" and "into" would conflict with one another when
       * completing; cdecl tries to guess which one you wanted, and it
       * always guesses correctly
       */
      if (!strncmp(rl_line_buffer, "cast", 4)
          && !strstr(rl_line_buffer, "into"))
        return strdup("into");
      else
        return strdup("int");
    } else while (keyword = keywords[index]) {
      index++;
      if (!strncmp(keyword, text, len)) return strdup(keyword);
    }
  }
  return NULL;
}
#endif /* USE_READLINE */

/* Write out a message about something */
/* being unsupported, possibly with a hint. */
void unsupp( char const *s, char const *hint ) {
  notsupported("", s, NULL);
  if (hint)
    fprintf( stderr, "\t(maybe you mean \"%s\")\n", hint );
}

/* Write out a message about something */
/* being unsupported on a particular compiler. */
void notsupported( char const *compiler, char const *type1, char const *type2)
{
    if (type2)
  fprintf(stderr,
      "Warning: Unsupported in%s C%s -- '%s' with '%s'\n",
      compiler, opt_lang == LANG_CXX ? "++" : "", type1, type2);
    else
  fprintf(stderr,
      "Warning: Unsupported in%s C%s -- '%s'\n",
      compiler, opt_lang == LANG_CXX ? "++" : "", type1);
}

char* cat( char const *s1, ... ) {
  size_t len = 1;
  va_list args;

  /* find the length which needs to be allocated */
  va_start( args, s1 );
  for ( char const *s = s1; s; s = va_arg( args, char const* ) )
    len += strlen( s );
  va_end( args );

  /* allocate it */
  char *const newstr = MALLOC( char, len );
  newstr[0] = '\0';

  /* copy in the strings */
  va_start( args, s1 );
  for ( char const *s = s1; s; s = va_arg( args, char const* ) ) {
    strcat( newstr, s );
    free( (void*)s );
  } // for
  va_end( args );

  return newstr;
}

#ifdef NOTMPFILE
/* provide a conservative version of tmpfile() */
/* for those systems without it. */
/* tmpfile() returns a FILE* of a file opened */
/* for read&write. It is supposed to be */
/* automatically removed when it gets closed, */
/* but here we provide a separate rmtmpfile() */
/* function to perform that function. */
/* Also provide several possible file names to */
/* try for opening. */
static char *file4tmpfile = 0;

FILE *tmpfile()
{
  static char const *const listtmpfiles[] = {
    "/usr/tmp/cdeclXXXXXX",
    "/tmp/cdeclXXXXXX",
    "/cdeclXXXXXX",
    "cdeclXXXXXX",
    NULL
  };

  char const **listp = listtmpfiles;
  for ( ; *listp; listp++) {
    FILE *retfp;
    mktemp(*listp);
    retfp = fopen(*listp, "w+");
    if (!retfp)
        continue;
    file4tmpfile = *listp;
    return retfp;
  }

  return 0;
}

void rmtmpfile() {
  if ( file4tmpfile )
     unlink( file4tmpfile );
}
#else
/* provide a mock rmtmpfile() for normal systems */
# define rmtmpfile()  /* nothing */
#endif /* NOTMPFILE */

/* Tell how to invoke cdecl. */
/* Manage the prompts. */
static int prompting;

void doprompt() { prompting = 1; }
void noprompt() { prompting = 0; }

void prompt()
{
#ifndef USE_READLINE
  if ((OnATty || opt_interactive) && prompting) {
    printf("%s", cdecl_prompt);
# if 0
    printf("%s> ", progname);
# endif /* that was the old way to display the prompt */
    fflush(stdout);
  }
#endif
}

/* Save away the name of the program from argv[0] */
void cdecl_setprogname(char *argv0) {
#ifdef DOS
  char *dot;
#endif /* DOS */

  progname = strrchr(argv0, '/');

#ifdef DOS
  if (!progname)
    progname = strrchr(argv0, '\\');
#endif /* DOS */

  if (progname)
    progname++;
  else
    progname = argv0;

#ifdef DOS
  dot = strchr(progname, '.');
  if (dot)
    *dot = '\0';
  for (dot = progname; *dot; dot++)
    *dot = tolower(*dot);
#endif /* DOS */

  /* this sets up the prompt, which is on by default */
  int len = strlen(progname);
  if (len > MAX_NAME) len = MAX_NAME;
  strncpy(real_prompt, progname, len);
  real_prompt[len] = '>';
  real_prompt[len+1] = ' ';
  real_prompt[len+2] = '\0';
}

/* Run down the list of keywords to see if the */
/* program is being called named as one of them */
/* or the first argument is one of them. */
static int namedkeyword( char const *argn ) {
  static char const *const cmdlist[] = {
    "?",
    "cast",
    "declare",
    "explain",
    "help",
    "set",
    NULL
  };

  /* first check the program name */
  char const *const *cmdptr = cmdlist;
  for ( ; *cmdptr; cmdptr++)
    if (strcmp(*cmdptr, progname) == 0) {
      KeywordName = 1;
      return 1;
    }

  /* now check $1 */
  for (cmdptr = cmdlist; *cmdptr; cmdptr++)
    if (strcmp(*cmdptr, argn) == 0)
      return 1;

  /* nope, must be file name arguments */
  return 0;
}

/* Read from standard input, turning */
/* on prompting if necessary. */
int dostdin() {
  int ret;
  if (OnATty || opt_interactive) {
#ifndef USE_READLINE
  if (!quiet) printf("Type `help' or `?' for help\n");
  prompt();
#else
  char *line, *oldline;
  int len, newline;

  if (!quiet) printf("Type `help' or `?' for help\n");
  ret = 0;
  while ((line = getline())) {
      if (!strcmp(line, "quit") || !strcmp(line, "exit")) {
    free(line);
    return ret;
      }
      newline = 0;
      /* readline() strips newline, we add semicolon if necessary */
      len = strlen(line);
      if (len && line[len-1] != '\n' && line[len-1] != ';') {
    newline = 1;
    oldline = line;
    line = malloc(len+2);
    strcpy(line, oldline);
    line[len] = ';';
    line[len+1] = '\0';
      }
      if (len) ret = dotmpfile_from_string(line);
      if (newline) free(line);
  }
  puts("");
  return ret;
#endif
  }

    yyin = stdin;
    ret = yyparse();
    OnATty = 0;
    return ret;
}

#ifdef USE_READLINE
/* Write a string into a file and treat that file as the input. */
int dotmpfile_from_string( char const *s)
{
    int ret = 0;
    FILE *tmpfp = tmpfile();
    if (!tmpfp)
  {
  int sverrno = errno;
  fprintf (stderr, "%s: cannot open temp file\n",
      progname);
  errno = sverrno;
  perror(progname);
  return 1;
  }

    if (fputs(s, tmpfp) == EOF)
  {
  int sverrno;
  sverrno = errno;
  fprintf (stderr, "%s: error writing to temp file\n",
      progname);
  errno = sverrno;
  perror(progname);
  fclose(tmpfp);
  rmtmpfile();
  return 1;
  }

    rewind(tmpfp);
    yyin = tmpfp;
    ret += yyparse();
    fclose(tmpfp);
    rmtmpfile();

    return ret;
}
#endif /* USE_READLINE */

/* Write the arguments into a file */
/* and treat that file as the input. */
int dotmpfile(int argc, char const *argv[])
{
    int ret = 0;
    FILE *tmpfp = tmpfile();
    if (!tmpfp)
  {
  int sverrno = errno;
  fprintf (stderr, "%s: cannot open temp file\n",
      progname);
  errno = sverrno;
  perror(progname);
  return 1;
  }

    if (KeywordName)
  if (fputs(progname, tmpfp) == EOF)
      {
      int sverrno;
  errwrite:
      sverrno = errno;
      fprintf (stderr, "%s: error writing to temp file\n",
    progname);
      errno = sverrno;
      perror(progname);
      fclose(tmpfp);
      rmtmpfile();
      return 1;
      }

    for ( ; optind < argc; optind++)
  if (fprintf(tmpfp, " %s", argv[optind]) == EOF)
      goto errwrite;

    if (putc('\n', tmpfp) == EOF)
  goto errwrite;

    rewind(tmpfp);
    yyin = tmpfp;
    ret += yyparse();
    fclose(tmpfp);
    rmtmpfile();

    return ret;
}

/* Read each of the named files for input. */
int dofileargs(int argc, char const *argv[])
{
    FILE *ifp;
    int ret = 0;

    for ( ; optind < argc; optind++)
  if (strcmp(argv[optind], "-") == 0)
      ret += dostdin();

  else if ((ifp = fopen(argv[optind], "r")) == NULL)
      {
      int sverrno = errno;
      fprintf (stderr, "%s: cannot open %s\n",
    progname, argv[optind]);
      errno = sverrno;
      perror(argv[optind]);
      ret++;
      }

  else
      {
      yyin = ifp;
      ret += yyparse();
      }

    return ret;
}

/* print out a cast */
void docast(name, left, right, type)
char *name, *left, *right, *type;
{
    int lenl = strlen(left), lenr = strlen(right);

    if (prev == 'f')
      unsupp("Cast into function",
        "cast into pointer to function");
    else if (prev=='A' || prev=='a')
      unsupp("Cast into array","cast into pointer");
    printf("(%s%*s%s)%s\n",
      type, lenl+lenr?lenl+1:0,
      left, right, name ? name : "expression");
    free(left);
    free(right);
    free(type);
    if (name)
        free(name);
}

/* print out a declaration */
void dodeclare(name, storage, left, right, type)
char *name, *storage, *left, *right, *type;
{
    if (prev == 'v')
      unsupp("Variable of type void",
        "variable of type pointer to void");

    if (*storage == 'r')
  switch (prev)
      {
      case 'f': unsupp("Register function", NULL); break;
      case 'A':
      case 'a': unsupp("Register array", NULL); break;
      case 's': unsupp("Register struct/class", NULL); break;
      }

    if (*storage)
        printf("%s ", storage);
    printf("%s %s%s%s",
        type, left,
  name ? name : (prev == 'f') ? "f" : "var", right);
    if (MkProgramFlag) {
      if ((prev == 'f') && (*storage != 'e'))
        printf(" { }\n");
      else
        printf(";\n");
    } else {
      printf("\n");
    }
    free(storage);
    free(left);
    free(right);
    free(type);
    if (name)
        free(name);
}

void dodexplain(storage, constvol1, constvol2, type, decl)
  char *storage, *constvol1, *constvol2, *type, *decl;
{
  if (type && (strcmp(type, "void") == 0)) {
    if (prev == 'n')
      unsupp("Variable of type void", "variable of type pointer to void");
    else if (prev == 'a')
      unsupp("array of type void", "array of type pointer to void");
    else if (prev == 'r')
      unsupp("reference to type void", "pointer to void");
  }

  if (*storage == 'r')
    switch (prev) {
      case 'f': unsupp("Register function", NULL); break;
      case 'A':
      case 'a': unsupp("Register array", NULL); break;
      case 's': unsupp("Register struct/union/enum/class", NULL); break;
    } // switch

  printf("declare %s as ", savedname);
  if (*storage)
    printf("%s ", storage);
  printf("%s", decl);
  if (*constvol1)
    printf("%s ", constvol1);
  if (*constvol2)
    printf("%s ", constvol2);
  printf("%s\n", type ? type : "int");
}

void docexplain(constvol, type, cast, name)
char *constvol, *type, *cast, *name;
{
  if (strcmp(type, "void") == 0) {
    if (prev == 'a')
      unsupp("array of type void", "array of type pointer to void");
    else if (prev == 'r')
      unsupp("reference to type void", "pointer to void");
  }
  printf("cast %s into %s", name, cast);
  if (strlen(constvol) > 0)
    printf("%s ", constvol);
  printf("%s\n",type);
}

/* Do the appropriate things for the "set" command. */
void doset( char const *opt ) {
  if (strcmp(opt, "create") == 0)
    { MkProgramFlag = 1; }
  else if (strcmp(opt, "nocreate") == 0)
    { MkProgramFlag = 0; }
  else if (strcmp(opt, "prompt") == 0)
    { prompting = 1; strcpy(cdecl_prompt, real_prompt); }
  else if (strcmp(opt, "noprompt") == 0)
    { prompting = 0; cdecl_prompt[0] = '\0'; }
#ifndef USE_READLINE
    /* I cannot seem to figure out what nointeractive was intended to do --
     * it didn't work well to begin with, and it causes problem with
     * readline, so I'm removing it, for now.  -i still works.
     */
  else if (strcmp(opt, "interactive") == 0)
    { opt_interactive = 1; }
  else if (strcmp(opt, "nointeractive") == 0)
    { opt_interactive = 0; OnATty = 0; }
#endif
  else if (strcmp(opt, "preansi") == 0)
    { opt_lang = LANG_C_KNR; }
  else if (strcmp(opt, "ansi") == 0)
    { opt_lang = LANG_C_ANSI; }
  else if (strcmp(opt, "cplusplus") == 0)
    { opt_lang = LANG_CXX; }
#ifdef dodebug
  else if (strcmp(opt, "debug") == 0)
    { DebugFlag = 1; }
  else if (strcmp(opt, "nodebug") == 0)
    { DebugFlag = 0; }
#endif /* dodebug */
#ifdef doyydebug
  else if (strcmp(opt, "yydebug") == 0)
    { yydebug = 1; }
  else if (strcmp(opt, "noyydebug") == 0)
    { yydebug = 0; }
#endif /* doyydebug */
  else {
    if ((strcmp(opt, unknown_name) != 0) &&
        (strcmp(opt, "options") != 0))
      printf("Unknown set option: '%s'\n", opt);

    printf("Valid set options (and command line equivalents) are:\n");
    printf("\toptions\n");
    printf("\tcreate (-c), nocreate\n");
    printf("\tprompt, noprompt (-q)\n");
#ifndef USE_READLINE
    printf("\tinteractive (-i), nointeractive\n");
#endif
    printf("\tpreansi (-p), ansi (-a) or cplusplus (-+)\n");
#ifdef dodebug
    printf("\tdebug (-d), nodebug\n");
#endif /* dodebug */
#ifdef doyydebug
    printf("\tyydebug (-D), noyydebug\n");
#endif /* doyydebug */

    printf("\nCurrent set values are:\n");
    printf("\t%screate\n", MkProgramFlag ? "   " : " no");
    printf("\t%sprompt\n", cdecl_prompt[0] ? "   " : " no");
    printf("\t%sinteractive\n", (OnATty || opt_interactive) ? "   " : " no");
    if (opt_lang == LANG_C_KNR)
      printf("\t   preansi\n");
    else
      printf("\t(nopreansi)\n");
    if ( opt_lang == LANG_C_ANSI )
      printf("\t   ansi\n");
    else
      printf("\t(noansi)\n");
    if ( opt_lang == LANG_CXX )
      printf("\t   cplusplus\n");
    else
      printf("\t(nocplusplus)\n");
#ifdef dodebug
    printf("\t%sdebug\n", DebugFlag ? "   " : " no");
#endif /* dodebug */
#ifdef doyydebug
    printf("\t%syydebug\n", yydebug ? "   " : " no");
#endif /* doyydebug */
  }
}

///////////////////////////////////////////////////////////////////////////////

static void cdecl_cleanup( void ) {
  free_now();
}

int main( int argc, char const *argv[] ) {
  atexit( cdecl_cleanup );
  options_init( argc, argv );

  int ret = 0;

#ifdef USE_READLINE
  /* install completion handlers */
  rl_attempted_completion_function = (CPPFunction *)attempt_completion;
  rl_completion_entry_function = (Function *)keyword_completion;
#endif

  prompting = OnATty = isatty( STDIN_FILENO );

  /* Set up the prompt. */
  if (prompting)
    strcpy(cdecl_prompt, real_prompt);
  else
    cdecl_prompt[0] = '\0';

  /* Use standard input if no file names or "-" is found. */
  if (optind == argc)
    ret = dostdin();

  /* If called as explain, declare or cast, or first */
  /* argument is one of those, use the command line */
  /* as the input. */
  else if (namedkeyword(argv[optind]))
    ret = dotmpfile(argc, argv);
  else
    ret = dofileargs(argc, argv);

  exit( ret );
}

///////////////////////////////////////////////////////////////////////////////

/*
 * cdecl - ANSI C and C++ declaration composer & decoder
 *
 *  originally written
 *    Graham Ross
 *    once at tektronix!tekmdp!grahamr
 *    now at Context, Inc.
 *
 *  modified to provide hints for unsupported types
 *  added argument lists for functions
 *  added 'explain cast' grammar
 *  added #ifdef for 'create program' feature
 *    ???? (sorry, I lost your name and login)
 *
 *  conversion to ANSI C
 *    David Wolverton
 *    ihnp4!houxs!daw
 *
 *  merged D. Wolverton's ANSI C version w/ ????'s version
 *  added function prototypes
 *  added C++ declarations
 *  made type combination checking table driven
 *  added checks for void variable combinations
 *  made 'create program' feature a runtime option
 *  added file parsing as well as just stdin
 *  added help message at beginning
 *  added prompts when on a TTY or in interactive mode
 *  added getopt() usage
 *  added -a, -r, -p, -c, -d, -D, -V, -i and -+ options
 *  delinted
 *  added #defines for those without getopt or void
 *  added 'set options' command
 *  added 'quit/exit' command
 *  added synonyms
 *    Tony Hansen
 *    attmail!tony, ihnp4!pegasus!hansen
 *
 *  added extern, register, static
 *  added links to explain, cast, declare
 *  separately developed ANSI C support
 *    Merlyn LeRoy
 *    merlyn@rose3.rosemount.com
 *
 *  merged versions from LeRoy
 *  added tmpfile() support
 *  allow more parts to be missing during explanations
 *    Tony Hansen
 *    attmail!tony, ihnp4!pegasus!hansen
 *
 *  added GNU readline() support
 *  added dotmpfile_from_string() to support readline()
 *  outdented C help text to prevent line from wrapping
 *  minor tweaking of makefile && mv makefile Makefile
 *  took out interactive and nointeractive commands when
 *      compiled with readline support
 *  added prompt and noprompt commands, -q option
 *    Dave Conrad
 *    conrad@detroit.freenet.org
 *
 *  added support for Apple's "blocks"
 *          Peter Ammon
 *          cdecl@ridiculousfish.com
 */

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
