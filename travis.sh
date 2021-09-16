#! /bin/sh
##
#       cdecl -- C gibberish translator
#       travis.sh
#
#       Copyright (C) 2021  Paul J. Lucas
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
# Build & Test Script for Travis CI <https://www.travis-ci.com>
#
# The reason for this script is because in Travis CI:
#
# + Although the "script" build phase can have multiple commands, a failure of
#   a command doesn't abort the phase.
#
# + If you have several build commands, it becomes unwieldy to chain them all
#   together using &&.
#
# + A failure of the "after_success" phase doesn't cause the build to fail.
#   This means you can't put "make check" in "after_success" and have the build
#   fail if the tests fail.
#
# Hence, rather than deal with Travis CI's quirks, just use our own build
# script.
##

set -ev # stop on error and be verbose

##
# Configure-time options for building & testing on Travis CI:
#
#     --distable-term-size  : Some platforms don't have terminfo
#     --without-readline    : BSD systems don't have readline
##
CONFIG_OPTS="--disable-term-size --without-readline"

# Build
./bootstrap
./configure $CONFIG_OPTS
make

# Run regression tests
make check

# Test installation
sudo make install

# vim: set et sw=2 ts=2:
