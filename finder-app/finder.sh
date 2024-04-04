#!/bin/sh

if [ "$#" -ne 2 ];
then
	echo "Error: Invalid number of arguments"
	exit 1
fi

if [ ! -d "$1" ];
then
	echo "Error: $1 is not a directory"
	exit 1
fi
dir="$1"
str="$2"
count=0
lineCount=0

inFileSearch() {
	local file="$1"
	local count2=$(grep -c "$str" "$file")
	lineCount=$((lineCount + count2))
}

fileSearch() {
	local dir2="$1"
	local files=$(find "$dir2" -type f)
	for file in $files;
	do
		inFileSearch "$file"
		count=$((count + 1))
	done
}

fileSearch "$dir"

echo "The number of files are $count and the number of matching lines are $lineCount"
