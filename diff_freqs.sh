#!/bin/bash

filelen() {
	echo $(wc -l "$1" | cut -d' ' -f1)
}

getline() {
	local file="$1"
	local i="$2"
	echo "$(sed -e "1,$(( $i - 1 ))d" -e "$(( $i + 1 )),${MAX}d" "$file")"
}

output() {
	local first="$1"
	local secnd="$2"

	first_x=$(echo $first | cut -d' ' -f1)
	secnd_x=$(echo $secnd | cut -d' ' -f1)
	echo "first_x=$first_x"
	echo "secnd_x=$secnd_x"
	if [[ "$first_x" != "$secnd_x" ]]; then
		echo "x values disagree!"
		exit 1
	fi

	first_y=$(echo $first | cut -d' ' -f2)
	secnd_y=$(echo $secnd | cut -d' ' -f2)
	echo "first_y=$first_y"
	echo "secnd_y=$secnd_y"

	diff_y=$(wcalc "$secnd_y - $first_y" |  cut -d' ' -f3)
	echo "diff_y=			$diff_y"
	echo "$first_x	$diff_y" >> "$OUTFILE"
}
	

FIRST_INFILE="freqs_intg.dat"
SECND_INFILE="freqs_fftw3.dat"
OUTFILE="freqs.diff"

MAX=$(filelen "$FIRST_INFILE")
if [[ ! $MAX -eq $(filelen "$SECND_INFILE") ]]; then
	echo "Error: files don't have same length!"
	exit 1
fi
echo "MAX = $MAX"


if [[ -e "$OUTFILE" ]]; then
	echo "Removing outfile \"$OUTFILE\"..."
	rm "$OUTFILE"
fi



# line 1 needs special handling
first="$(head -n1 "$FIRST_INFILE")"
secnd="$(head -n1 "$SECND_INFILE")"
echo "first[1]=\"$first\""
echo "secnd[1]=\"$secnd\""

output "$first" "$secnd"

echo


i=2
while [[ $i -lt $MAX ]]; do
	first="$(getline "$FIRST_INFILE" "$i")"
	secnd="$(getline "$SECND_INFILE" "$i")"
	echo "first[$i]=\"$first\""
	echo "secnd[$i]=\"$secnd\""

	output "$first" "$secnd"

	i=$(( $i + 1 ))
	echo
done


# the last line needs special handling
first="$(tail -n1 "$FIRST_INFILE")"
secnd="$(tail -n1 "$SECND_INFILE")"
echo "first[1]=\"$first\""
echo "secnd[1]=\"$secnd\""

output "$first" "$secnd"
