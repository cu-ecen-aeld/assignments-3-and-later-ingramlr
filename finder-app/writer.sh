#!/bin/bash

writefile="$1"
writestr="$2" #Declare a couple variables for ease of reading/use later

if [[ ! $1 ]]; then #We can reasonably assume that if theres no 1 argument theres nothing at all
	echo "ERROR no arguments were specified, usage ./writer.sh '/dir/name.txt' 'text'"
	exit 1
fi
directory="$(dirname $1)" #Put this here so BASH does not try to expand and error ofr undefined
if [[ ! -d $directory ]]; then
	mkdir -p $directory
	if [[ ! $? ]]; then
		echo "ERROR unable to generate file directory"
		exit 1
	fi
fi

echo "$writestr" > $writefile
