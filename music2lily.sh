#!/bin/bash

log="log"
base="$1"

if [ -z "$base" ] ; then
	echo "Usage: $0 <basename>"
	exit 1
else
	echo "base is \"$base\""
fi

PATH="$PATH:`dirname "$0"`"
tmpfifo="`mktemp -u "fifo.$base.XXXXXX"`"
mkfifo "$tmpfifo"
echo "using $tmpfifo"

synth <"$tmpfifo" >"$base.sau" &
time frequetize <"$base.wav" \
	| tee "$tmpfifo" \
	| lilyfy "$base" >"$base.ly" \
	|| (rm "$tmpfifo"; exit 2)
rm "$tmpfifo"
lilypond "$base.ly" || exit 3
evince "$base.pdf" || exit 4
