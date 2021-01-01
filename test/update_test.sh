#! /bin/sh
##
#       cdecl -- C gibberish translator
#       test/update_test.sh
#
#       Copyright (C) 2020-2021  Paul J. Lucas
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
usage: $ME test-path...
END
  exit 1
}

########## Begin ##############################################################

ME=`local_basename "$0"`

[ "$BUILD_SRC" ] || {
  echo "$ME: \$BUILD_SRC not set" >&2
  exit 2
}

########## Process command-line ###############################################

[ $# -ge 1 ] || usage

########## Initialize #########################################################

##
# The automake framework sets $srcdir. If it's empty, it means this script was
# called by hand, so set it ourselves.
##
[ "$srcdir" ] || srcdir="."

DATA_DIR=$srcdir/data
EXPECTED_DIR=$srcdir/expected
ACTUAL_OUTPUT=/tmp/cdecl_test_output_$$_

########## Run test ###########################################################

update_cdecl_test() {
  TEST_PATH=$1
  TEST_NAME=`local_basename "$TEST_PATH"`
  EXPECTED_OUTPUT="$EXPECTED_DIR/`echo $TEST_NAME | sed s/test$/out/`"

  IFS_old=$IFS
  IFS='@'; read COMMAND CONFIG OPTIONS INPUT EXPECTED_EXIT < $TEST_PATH
  [ "$IFS_old" ] && IFS=$IFS_old

  COMMAND=`echo $COMMAND`               # trims whitespace
  CONFIG=`echo $CONFIG`                 # trims whitespace
  [ "$CONFIG" ] && CONFIG="-c $DATA_DIR/$CONFIG"
  EXPECTED_EXIT=`echo $EXPECTED_EXIT`   # trims whitespace

  #echo "$INPUT" \| $COMMAND $CONFIG "$OPTIONS" \> $ACTUAL_OUTPUT
  if echo "$INPUT" | sed 's/^ //' | $COMMAND $CONFIG $OPTIONS > $ACTUAL_OUTPUT
  then
    if [ 0 -eq $EXPECTED_EXIT ]
    then mv $ACTUAL_OUTPUT $EXPECTED_OUTPUT
    else fail
    fi
  else
    ACTUAL_EXIT=$?
    if [ $ACTUAL_EXIT -ne $EXPECTED_EXIT ]
    then fail
    fi
  fi
}

##
# Must put BUILD_SRC first in PATH so we get the correct version of cdecl.
##
PATH=$BUILD_SRC:$PATH

trap "x=$?; rm -f /tmp/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM

for TEST in $*
do update_cdecl_test $TEST
done

# vim:set et sw=2 ts=2:
