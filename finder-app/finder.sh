#!/bin/bash

filesdir="$1"
searchstr="$2" #Declair both the input results as variables for ease of reading
error="parameters are passed with ./finder.sh '/dir' 'search term'"

if [[ ! $1 ]]; then #If there is no results passed to 1 we can reasonably believe nothing was passed
	echo "ERROR no parameters specified, $error"
	exit 1
fi
if [[ ! -d $1 ]]; then
	echo "ERROR directory passed was not valid, $error"
        exit 1
fi

strresult=$(grep -rwl $filesdir -e $searchstr | wc -l) #Greps every file contained in the passed directory with the requested critera and line counts the results
searchresult=$(find $filesdir -type f -exec echo {} \; | wc -l) #Finds every file in the passed directory, echos it in a second shell and line counts the results

echo "The number of files are $searchresult and the number of matching lines are $strresult"
