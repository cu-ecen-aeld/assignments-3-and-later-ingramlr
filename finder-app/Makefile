#Simple Makefile for the writer.c function

all :
	gcc writer.c -o writer

CROSS_COMPILE :
	aarch64-none-linux-gnu-gcc writer.c -o writer

clean :
	rm -f writer
	#-f is needed here to "force" removal of the file and prevent erroring out
