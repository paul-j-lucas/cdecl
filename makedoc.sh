#! /usr/bin/env bash
##
#       makedoc.sh
#
#       Copyright (C) 2021-2026  Paul J. Lucas
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

set -e

# Uncomment the following line for shell tracing.
#set -x

########## Functions ##########################################################

is_remote_login() {
  [ "$SSH_CLIENT" -o "$SSH_CONNECTION" -o "$SSH_TTY" ]
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

usage() {
  cat >&2 <<END
usage: $ME package-name docs-dir
END
  exit 1
}

########## Process command-line ###############################################

ME=$(local_basename "$0")

(( $# == 2 )) || usage
PACKAGE=$1
DOCS_DIR=$2

########## Begin ##############################################################

INDEX_HTML="$DOCS_DIR/index.html"

echo "Generating $PACKAGE documentation ..."
doxygen
echo "... HTML documentation generated at: $INDEX_HTML"

is_remote_login && exit 0

echo
echo "To view in a local web browser, go to:"
echo "    file://$PWD/$INDEX_HTML"

[ `uname` = Darwin ] && command -v open >/dev/null && {
  echo "or run:"
  echo "    open $INDEX_HTML"
}

# vim:set et sw=2 ts=2:
