#! /bin/sh
##
#       cdecl -- C gibberish translator
#       makedoc.sh
#
#       Copyright (C) 2021-2024  Paul J. Lucas
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

########## Begin ##############################################################

echo "Generating documentation..."
doxygen

INDEX_HTML="docs/index.html"

echo
echo "HTML documentation generated at: $INDEX_HTML"

is_remote_login && exit 0

echo
echo "To view in a local web browser, go to:"
echo "    file://$PWD/$INDEX_HTML"

[ `uname` = Darwin ] && command -v open >/dev/null && {
  echo "or run:"
  echo "    open $INDEX_HTML"
}

# vim:set et sw=2 ts=2:
