#! /usr/bin/env bash
##
#       cdecl -- C gibberish translator
#       test/not_in_Makefile.sh
#
#       Copyright (C) 2020-2025  Paul J. Lucas
#
#       This program is free software: you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation, either version 3 of the License, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

local_basename() {
  ##
  # Autoconf, 11.15:
  #
  # basename
  #   Not all hosts have a working basename. You can use expr instead.
  ##
  expr "//$1" : '.*/\(.*\)'
}

# Prints *.test files that exist but are not in Makefile.am.
ls tests/*.test | while read TEST
do fgrep $TEST Makefile.am >/dev/null 2>&1 || echo $TEST
done

# Prints *.out files that exist but have no corresponding test in Makefile.am.
ls expected/*.out | while read EXPECTED
do
  TEST=$(local_basename "$EXPECTED" | sed s/out/test/)
  fgrep $TEST Makefile.am >/dev/null 2>&1 || echo $EXPECTED
done

# vim:set et sw=2 ts=2:
