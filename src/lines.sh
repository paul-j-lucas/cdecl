#! /usr/bin/env bash
##
#       cdecl -- C gibberish translator
#       lines.sh
#
#       Copyright (C) 2022-2026  Paul J. Lucas
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

##
# Count the number of lines in current cdecl source files and compare them to
# Tony Hansen's 1988 version of cdecl.
##

set -e

CH_EXCLUDE='^(config\.h|lexer\.c|parser\.[ch]|.*test\.[ch])$'
CH_FILES=$(ls *.[ch] | egrep -v "$CH_EXCLUDE")

HANSEN_cdgram_y=855
HANSEN_cdlex_l=75
HANSEN_cdecl_c=1014

count_lines() {
  hansen_lines=$1
  label=$2
  shift
  [ $# -gt 2 ] && shift
  awk -v hansen_lines=$hansen_lines -v label="$label" '
    END { printf( "%-8s %5d %4.1f%%\n", label, NR, hansen_lines / NR * 100 ) }
  ' $*
}

HANSEN_total=$(expr $HANSEN_cdgram_y + $HANSEN_cdlex_l + $HANSEN_cdecl_c)
{
  count_lines $HANSEN_cdgram_y parser.y
  count_lines $HANSEN_cdlex_l lexer.l 
  count_lines $HANSEN_cdecl_c "*.[ch]" $CH_FILES
} |
awk -v hansen_total=$HANSEN_total '
      { total += $2; print }
END   {
  printf( "======== ===== =====\n" )
  printf( "%-8s %5d %4.1f%%\n", "Total", total, hansen_total / total * 100 )
}
'

# vim:set et sw=2 ts=2:
