#!/bin/sh

#Simple shell script to start/stop the aesdsocket daemon

case "$1" in
	start)
		echo "Starting aesdsocket"
		start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
		;;
	stop)   
		echo "Stopping aesdsocket"
		start-stop-daemon -K -n aesdsocket
		;;
	*)      
		echo "Usage: $0 {start|stop}"
		exit 1
 	esac    
exit 0 
