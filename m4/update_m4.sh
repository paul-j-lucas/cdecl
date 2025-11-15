#! /usr/bin/env bash
##
#       update_m4.sh
#
#       Copyright (C) 2025  Paul J. Lucas
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

set -eo pipefail

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

if command -v curl >/dev/null 2>&1
then
  download() {
    curl -fsSL "$1" -o "$2"
  }
elif command -v wget >/dev/null 2>&1
then
  download() {
    wget -qO "$2" "$1"
  }
else
  echo "$ME: neither curl nor wget is available" >&2
  exit 1
fi

########## Begin ##############################################################

ME=$(local_basename "$0")

[ "$TMPDIR" ] || TMPDIR=/tmp
trap "x=$?; rm -f $TMPDIR/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM
TMP_FILE=$TMPDIR/${ME}__$$__

for file in ax_*.m4
do
  url="https://gitweb.git.savannah.gnu.org/gitweb/?p=autoconf-archive.git;a=blob_plain;f=m4/$file"

  echo "$ME: checking $file ..."
  download "$url" "$TMP_FILE" || continue
  diff -q "$file" "$TMP_FILE" >/dev/null 2>&1 && continue
  mv "$TMP_FILE" "$file"
  echo "-> updated"
done

# vim:set et sw=2 ts=2:
