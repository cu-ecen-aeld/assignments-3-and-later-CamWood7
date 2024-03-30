#!/bin/bash

if [ "$#" -ne 2 ];
then
	echo "Error: Invalid number of arguments"
	exit 1
fi
if [ -z "$1" ];
then
	echo "Error: No File Given"
	exit 1
fi
if [ -z "$2" ];
then
	echo "Error: No String Given"
	exit 1
fi
fileName="$1"
str="$2"
fileDir=$(dirname "$fileName")
if [ ! -d "$fileDir" ];
then
	mkdir = -p "$fileDir" || { echo "Error: Failed to make directory"; exit 1; }
fi
echo "$str" > "$fileName" || { echo "Error: Failed to write to file"; exit 1; }
echo "Content was successfully written"
