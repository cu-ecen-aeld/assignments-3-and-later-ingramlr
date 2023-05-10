#include <stdio.h>
#include <syslog.h>
#define ERROR_EXIT 1
#define NO_ERROR 0

int main (int numarg, char *textarg[]){

FILE *file;
file = fopen(textarg[1], "w"); 
int close;

	if (numarg == 3){
		fprintf(file, "%s\n", textarg[2]);
		syslog(LOG_DEBUG, "Writing %s to %s", textarg[1], textarg[2]);
		close = fclose(file);

		if (close != 0) { 
			syslog(LOG_ERR, "ERROR: Unable to close file!");
		       return ERROR_EXIT;	
		} 
	}
	if (numarg != 3){
		syslog(LOG_ERR, "ERROR: Invalid number of arguments passed! Must be \"/dir/file\" \"text\"");
		return ERROR_EXIT;
	}
	
	return NO_ERROR;
}
