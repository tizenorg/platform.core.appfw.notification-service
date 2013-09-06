#! /bin/sh

autoreconf --force -v --install
test -n "$NOCONFIGURE" || "./configure" "$@"
