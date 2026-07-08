##
# SYNOPSIS
#
#     PJL_COMPILE(label, include(s), program)
#
# DESCRIPTION
#
#     Shorthand for calling AC_COMPILE_IFELSE() and defining HAVE_{toupper($1)}
#     if the compile succeeds.
#
# PARAMETERS
#
#     $1  Label; will define HAVE_{m4_toupper($1)} with spaces replaced by '_'.
#     $2  #include(s), if any.
#     $3  Code snippet to compile.
#
# LICENSE
#
#     Copyright (C) 2020-2026  Paul J. Lucas
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

#serial 5

AC_DEFUN([PJL_COMPILE], [
  m4_pushdef([PJL_WHAT], m4_translit(m4_toupper([$1]), [ ], [_]))dnl
  AC_CACHE_CHECK([for $1], [pjl_cv_]PJL_WHAT,
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([$2], [$3])],
      [pjl_cv_]PJL_WHAT[=yes],
      [pjl_cv_]PJL_WHAT[=no]
    )
  ])
  if test "$pjl_cv_[]PJL_WHAT" = "yes"; then
    AC_DEFINE([HAVE_]PJL_WHAT, [1], [Define to 1 if you have `$1'.])
  fi
  m4_popdef([PJL_WHAT])dnl
])

dnl vim:set et sw=2 ts=2:
