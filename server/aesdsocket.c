#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#define PORT "9000"
#define CONNECTIONS 10  //10 possible waiting connections
#define FILENAME "/var/tmp/aesdsocketdata"

//Declare our fds and set to -1 for no file error so they can be used globally for cleaner code
static volatile int sock_fd = -1, client_fd = -1, file_fd = -1;         //Socket connection fd, client connection fd, and the tmp file fd

void signals(){

    if (file_fd >= 0 && close(file_fd)) syslog(LOG_ERR, "%s: %m", "Close file"); //If a file is still open, close it and log it

    if (client_fd >= 0){                                                                       //If a client connection is still open, shutdown the IO, close it and log it
        if (shutdown(client_fd, SHUT_RDWR)) syslog(LOG_ERR, "%s: %m", "Shutdown connection");
        if (close(client_fd)) syslog(LOG_ERR, "%s: %m", "Close socket descriptor");
    }

    if (sock_fd >= 0 && close(sock_fd)) syslog(LOG_ERR, "%s: %m", "Close server descriptor");   //If the socket is still open, close it and log it

    if ((unlink(FILENAME)) == -1 ) syslog(LOG_ERR, "%s: %m", "Error deleting tmp file");   //Delete the tmp file we created and log if error

    if (SIGINT | SIGTERM){                                                                      //If SIGINT or SIGTERM is sent, log it and exit
        syslog(LOG_DEBUG,"%s", "Caught signal, exiting"); 
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[]){
    int  yes = 1;   //Using "yes" from Beejs Networking
    struct addrinfo hints, *server_addr;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[1024];  //Total buffer size is 1kB

    // Set the server address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int addrerror = getaddrinfo(NULL, PORT, &hints, &server_addr);

    if(addrerror != 0) syslog(LOG_ERR, "ERROR getaddrinfo: %s", gai_strerror(addrerror));

    // Create socket file descriptor
    if ((sock_fd = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol)) == -1){

        syslog(LOG_ERR, "ERROR socket error");
        return -1;
    }

    // Set socket options to reuse port
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){

        syslog(LOG_ERR, "ERROR sock options");
        close(sock_fd);
        return -1;
    }

    // Bind socket to port
    if (bind(sock_fd, server_addr->ai_addr, server_addr->ai_addrlen) == -1){

        syslog(LOG_ERR, "ERROR bind failed");
        close(sock_fd);
        return -1;
    }

    freeaddrinfo(server_addr); //Free the malloc called within getaddrinfo as its not needed anymore

    if (argc > 1){
        if (strcmp(argv[1], "-d") == 0){
            
            int pid = fork();

            if (pid == -1){ //Error checking with fork()
                syslog(LOG_ERR, "ERROR fork");
                close(sock_fd);
                return -1;
            }
            else if (pid != 0){ //Assuming fork() executes correctly we then close the main application for the daemon to exist
                close(sock_fd);
                exit(EXIT_SUCCESS);
            }
        }
        else {
            syslog(LOG_ERR, "ERROR Invalid argument specified");    //Just some error codes if someone/something tries anyhing other than "-d"
            printf("ERROR Invalid argument specified\n");
        }
    }

    // Listen for incoming connections
    if (listen(sock_fd, CONNECTIONS) == -1){

        syslog(LOG_ERR, "ERROR with listen");
        close(sock_fd);
        return -1;
    }

    // Set signal handlers for SIGINT and SIGTERM
    signal(SIGINT, signals);
    signal(SIGTERM, signals);

    // Loop forever accepting incoming connections
    ssize_t bytes_read=0;
    while (1){
        // Accept a new client connection
        if ((client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addrlen)) == -1) { //Error checking for a -1 return value
            syslog(LOG_ERR, "ERROR with accept");
            continue;
        }

        // Log connection details to syslog
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);    //Using this from Beejs guide
        syslog(LOG_DEBUG, "Accepted connection from %s", client_ip);

        // Read data from the client connection
        while ((bytes_read = recv(client_fd, buffer, 1024, 0)) > 0) {   //Buffer set to 1kB
            if (bytes_read == -1){
                syslog(LOG_ERR, "ERROR with recv");
                signals();
            }
            // Append the data to the file
            // Open if closed
            if (file_fd < 0) {
                file_fd = open(FILENAME, O_CREAT | O_RDWR | O_APPEND, 0644);
                if (file_fd < 0) {
                    syslog(LOG_ERR, "ERROR with file open");
                    signals();
                }
            }
            // write to file
            if ((write(file_fd, &buffer, bytes_read)) == -1){
                syslog(LOG_ERR, "ERROR with write");
            }
            // exit from reading loop if
            if (buffer[bytes_read-1] == '\n')
                break;
        }

        // Sending answer
        if (lseek(file_fd, (off_t) 0, SEEK_SET) == (off_t) -1){ //Check for error with (off_t) as defined by POSIX
            syslog(LOG_ERR, "ERROR with seek");
            signals();
        }

        int bytes_send;
        while ((bytes_read = read(file_fd, &buffer, 1024)) > 0){    //Bytes and buffer set to 1024, 1kB
            while ((bytes_send = send(client_fd, &buffer, bytes_read, 0)) < bytes_read){
                syslog(LOG_ERR, "ERROR with send");
                signals();
            }
        }

        // Close connnection and log details to syslog
        close(file_fd); //Close the tmp file
        file_fd = -1;       //Set to -1 so no other file can be opened
        close(client_fd);   //Close the client connection to the server
        client_fd = -1;     //Set to -1 so no other file can be opened
        syslog(LOG_DEBUG, "Closed connection from %s", client_ip);
    }
    exit(EXIT_SUCCESS);
}
