##
# SYNOPSIS
#
#     PJL_CHECK_COMPILE_FLAG(flag, variable)
#
# DESCRIPTION
#
#     Shorthand for calling AX_CHECK_COMPILE_FLAG().
#
# PARAMETERS
#
#     $1  Compiler flag.
#     $2  Flags variable.
#
# LICENSE
#
#     Copyright (C) 2025  Paul J. Lucas
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

#serial 1

AC_DEFUN([PJL_CHECK_COMPILE_FLAG], [
  AC_REQUIRE([AX_CHECK_COMPILE_FLAG])
  AX_CHECK_COMPILE_FLAG([$1], [$2="$[$2] $1"])
])

dnl vim:set et sw=2 ts=2:
