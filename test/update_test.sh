#! /usr/bin/env bash
##
#       cdecl -- C gibberish translator
#       test/update_test.sh
#
#       Copyright (C) 2020-2026  Paul J. Lucas
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

# Uncomment the following line for shell tracing.
#set -x

########## Functions ##########################################################

local_basename() {
  ##
  # Autoconf, 11.15:
  #
  # basename
  #   Not all hosts have a working basename. You can use expr instead.
  ##
  expr "//$1" : '.*/\(.*\)'
}

fail() {
  echo FAIL $TEST_NAME
}

usage() {
  cat >&2 <<END
usage: $ME -s srcdir test ...
END
  exit 1
}

########## Begin ##############################################################

ME=$(local_basename "$0")

[ "$LINENO" ] || {
  echo "$ME: shell's \$LINENO not set" >&2
  exit 3
}

########## Process command-line ###############################################

while getopts s: opt
do
  case $opt in
  s) BUILD_SRC="$OPTARG" ;;
  ?) usage ;;
  esac
done
shift $(( OPTIND - 1 ))

[ $# -ge 1 ] || usage

########## Initialize #########################################################

[ "$BUILD_SRC" ] || {
  echo "$ME: \$BUILD_SRC not set" >&2
  exit 2
}

[ "$TMPDIR" ] || TMPDIR=/tmp
trap "x=$?; rm -f $TMPDIR/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM

##
# The automake framework sets $srcdir. If it's empty, it means this script was
# called by hand, so set it ourselves.
##
[ "$srcdir" ] || srcdir="."

DATA_DIR="$srcdir/data"
EXPECTED_DIR="$srcdir/expected"
ACTUAL_OUTPUT="$TMPDIR/cdecl_test_output_$$_"

##
# Must put BUILD_SRC first in PATH so we get the correct version of cdecl.
##
PATH=$BUILD_SRC:$PATH

##
# Disable core dumps so we won't fill up the disk with them if a bunch of tests
# crash.
##
ulimit -c 0

########## Update tests #######################################################

update_cdecl_test() {
  TEST_PATH="$1"
  TEST_NAME=$(local_basename "$TEST_PATH")
  EXPECTED_OUTPUT="$EXPECTED_DIR/$(echo $TEST_NAME | sed s/test$/out/)"

  echo $TEST_NAME

  # Dot-execute the test so we get its value of EXPECTED_EXIT.
  . $TEST > $ACTUAL_OUTPUT 2>&1

  ACTUAL_EXIT=$?
  if [ "$ACTUAL_EXIT" -eq "$EXPECTED_EXIT" ]
  then mv "$ACTUAL_OUTPUT" "$EXPECTED_OUTPUT"
  else fail
  fi
}

# Ensure cdecl knows it's being tested.
export CDECL_TEST=true

for TEST in $*
do
  case "$TEST" in
  *.test) update_cdecl_test "$TEST" ;;
  esac
done

# vim:set et sw=2 ts=2:
