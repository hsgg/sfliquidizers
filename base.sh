#!/bin/bash

log="output"
base="$1"

if [ -z "$base" ]; then
	echo "Usage: $0 <basename>"
	exit 1
fi

PATH="$PATH:."

time music2lily "$base.wav" "$base.ly" \
&& lilypond "$base.ly" \
&& evince "$base.pdf" \
