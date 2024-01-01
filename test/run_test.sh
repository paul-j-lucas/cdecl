#! /bin/sh
##
#       cdecl -- C gibberish translator
#       test/run_test.sh
#
#       Copyright (C) 2017-2024  Paul J. Lucas
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

error() {
  exit_status=$1; shift
  echo $ME: $*
  exit $exit_status
}

assert_exists() {
  [ -e "$1" ] || error 66 $1: file not found
}

local_basename() {
  ##
  # Autoconf, 11.15:
  #
  # basename
  #   Not all hosts have a working basename. You can use expr instead.
  ##
  expr "//$1" : '.*/\(.*\)'
}

pass() {
  print_result PASS $TEST_NAME
  {
    echo ":test-result: PASS"
    echo ":copy-in-global-log: no"
  } > $TRS_FILE
}

fail() {
  result=$1; shift; [ "$result" ] || result=FAIL
  print_result $result $TEST_NAME $*
  {
    echo ":test-result: $result"
    echo ":copy-in-global-log: yes"
  } > $TRS_FILE
}

print_result() {
  result=$1; shift
  COLOR=`eval echo \\$COLOR_$result`
  if [ "$COLOR" ]
  then echo $COLOR$result$COLOR_NONE: $*
  else echo $result: $*
  fi
}

usage() {
  [ "$1" ] && { echo "$ME: $*" >&2; usage; }
  cat >&2 <<END
usage: $ME --log-file=PATH --trs-file=PATH [options] TEST-FILE
options:
  --color-tests={yes|no}
  --enable-hard-errors={yes|no}
  --expect-failure={yes|no}
  --test-name=NAME
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

while [ $# -gt 0 ]
do
  case $1 in
  --color-tests)
    COLOR_TESTS=$2; shift
    ;;
  --color-tests=*)
    COLOR_TESTS=`expr "x$1" : 'x--color-tests=\(.*\)'`
    ;;
  --enable-hard-errors)
    ENABLE_HARD_ERRORS=$2; shift
    ;;
  --enable-hard-errors=*)
    ENABLE_HARD_ERRORS=`expr "x$1" : 'x--enable-hard-errors=\(.*\)'`
    ;;
  --expect-failure)
    EXPECT_FAILURE=$2; shift
    ;;
  --expect-failure=*)
    EXPECT_FAILURE=`expr "x$1" : 'x--expect-failure=\(.*\)'`
    ;;
  --help)
    usage
    ;;
  --log-file)
    LOG_FILE=$2; shift
    ;;
  --log-file=*)
    LOG_FILE=`expr "x$1" : 'x--log-file=\(.*\)'`
    ;;
  --test-name)
    TEST_NAME=$2; shift
    ;;
  --test-name=*)
    TEST_NAME=`expr "x$1" : 'x--test-name=\(.*\)'`
    ;;
  --trs-file)
    TRS_FILE=$2; shift
    ;;
  --trs-file=*)
    TRS_FILE=`expr "x$1" : 'x--trs-file=\(.*\)'`
    ;;
  --)
    shift
    break
    ;;
  -*)
    usage
    ;;
  *)
    break
    ;;
  esac
  shift
done

TEST=$1
[ "$TEST_NAME" ] || TEST_NAME=$TEST
[ "$LOG_FILE"  ] || usage "required --log-file not given"
[ "$TRS_FILE"  ] || usage "required --trs-file not given"
[ $# -ge 1     ] || usage "required test-file not given"

TEST_NAME=`local_basename "$TEST_NAME"`

########## Initialize #########################################################

if [ "$COLOR_TESTS" = yes -a -t 1 ]
then
  COLOR_BLUE="[1;34m"
  COLOR_GREEN="[0;32m"
  COLOR_LIGHT_GREEN="[1;32m"
  COLOR_MAGENTA="[0;35m"
  COLOR_NONE="[m"
  COLOR_RED="[0;31m"

  COLOR_ERROR=$COLOR_MAGENTA
  COLOR_FAIL=$COLOR_RED
  COLOR_PASS=$COLOR_GREEN
  COLOR_SKIP=$COLOR_BLUE
  COLOR_XFAIL=$COLOR_LIGHT_GREEN
  COLOR_XPASS=$COLOR_RED
fi

case $EXPECT_FAILURE in
yes) EXPECT_FAILURE=1 ;;
  *) EXPECT_FAILURE=0 ;;
esac

##
# The automake framework sets $srcdir. If it's empty, it means this script was
# called by hand, so set it ourselves.
##
[ "$srcdir" ] || srcdir="."

DATA_DIR=$srcdir/data
EXPECTED_DIR=$srcdir/expected
DIFF_FILE=/tmp/cdecl_diff_$$_

##
# Must fix the number of terminal columns at 80 overriding the actual TERM
# number of columns so that columns-dependent tests pass.
##
COLUMNS=80
export COLUMNS

##
# Must put BUILD_SRC first in PATH so we get the correct version of cdecl.
##
PATH=$BUILD_SRC:$PATH

trap "x=$?; rm -f /tmp/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM

##
# Disable core dumps so we won't fill up the disk with them if a bunch of tests
# crash.
##
ulimit -c 0

########## Run test ###########################################################

run_cdecl_test() {
  EXPECTED_OUTPUT="$EXPECTED_DIR/`echo $TEST_NAME | sed s/test$/out/`"
  assert_exists $EXPECTED_OUTPUT

  # Dot-execute the test so we get its value of EXPECTED_EXIT.
  . $TEST > $LOG_FILE 2>&1

  ACTUAL_EXIT=$?
  if [ $ACTUAL_EXIT -eq $EXPECTED_EXIT ]
  then
    if diff -u $EXPECTED_OUTPUT $LOG_FILE > $DIFF_FILE
    then pass
    else fail; cp $DIFF_FILE $LOG_FILE
    fi
  else
    case $ACTUAL_EXIT in
    0|65) fail ;;
    *)    fail ERROR $ACTUAL_EXIT ;;
    esac
  fi
}

run_script_test() {
  if $TEST > $LOG_FILE 2>&1
  then pass
  else fail
  fi
}

assert_exists $TEST
case $TEST in
*.exp)  run_script_test ;;
*.test) run_cdecl_test ;;
esac

# vim:set et sw=2 ts=2:
