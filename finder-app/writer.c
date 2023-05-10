#include <stdio.h>
#include <syslog.h>
#define ERROR_EXIT 1
#define NO_ERROR 0

int main (int numarg, char *textarg[]){

FILE *file; //FILE variable type is provided by stdio.h and is needed for any file systemcall
file = fopen(textarg[1], "w"); 
int close;

	if (numarg == 3){
		fprintf(file, "%s\n", textarg[2]); //Calls FILE file which in turn opens it, and either creates the file or overwrites the file with the new txt string
		syslog(LOG_DEBUG, "Writing %s to %s", textarg[1], textarg[2]);
		close = fclose(file); //Weirdly, close has to be defined here for the file to close and changes be saved

		if (close != 0) { //Checking to see if the fclose function defined by varible close returns good or bad
			syslog(LOG_ERR, "ERROR: Unable to close file!");
		       return ERROR_EXIT;	
		} 
	}
	if (numarg != 3){ //If theres anything but 3 environment varibles passed were going to assume bad
		syslog(LOG_ERR, "ERROR: Invalid number of arguments passed! Must be \"/dir/file\" \"text\"");
		return ERROR_EXIT;
	}
	
	return NO_ERROR;
}
