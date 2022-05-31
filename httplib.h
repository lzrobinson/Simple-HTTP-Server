#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define GET_METHOD "GET"
#define HTTP_VERSION "HTTP/1.0"
#define CRLF "\r\n\r\n"
#define MAX_GET_SIZE 2000
#define CRLF_OFFSET 4

#define FNF_MSG "HTTP/1.0 404 NOT FOUND\r\n\r\n"
#define CONTENT_TYPE_HTML "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define CONTENT_TYPE_JPEG "HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n"
#define CONTENT_TYPE_CSS "HTTP/1.0 200 OK\r\nContent-Type: text/css\r\n\r\n"
#define CONTENT_TYPE_JAVASCRIPT "HTTP/1.0 200 OK\r\nContent-Type: text/javascript\r\n\r\n"
#define CONTENT_TYPE_UNDEFINED "HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n"

// function which checks if a buffer of size message_size ends in a CRLF
int end_in_crlf(int message_size, char buffer[]) {
    char lastfourchars[5] = {buffer[message_size-CRLF_OFFSET], buffer[message_size-CRLF_OFFSET+1], buffer[message_size-CRLF_OFFSET+2], buffer[message_size-CRLF_OFFSET+3], '\0'};
    if (strcmp(lastfourchars, CRLF) == 0) {
        return 1;
    }
    return 0;
}

/* reads in a request on socketfd, 
but in doing so ensures that the full request has been received before reacting to it */
int request_complete(int socketfd, char buffer[]) {
    int message_size = 0;
    while (1) {

        message_size += read(socketfd, &buffer[message_size], MAX_GET_SIZE);
        
        if (message_size < 0) {
            return -1;
        }
        else if (message_size >= CRLF_OFFSET) {
            if (end_in_crlf(message_size, buffer)) {
                break;
            }
        }
    }
    return message_size;
}

// use strstr() to check if given relative file dir in argv[3] to check if client has 'zoomed out and back in'
int filepath_zoomout_handler(char given_fp[], char relative_directory[]) {
    char* token;
    char dummy[strlen(relative_directory)];
    strcpy(dummy, relative_directory);
    token = strtok(dummy, "/");
	while (token != NULL) {
        if (strstr(given_fp, token) != NULL) {
            return 1;
        }
        token = strtok(NULL, "/");
    }
    return 0;
}

// function which processes a HTTP client request, returns 1 if valid request, -1 if invalid.
int request_handler(int sockfd, char buffer[], int message_size, char abs_path[]) {
    char* tokens[3];
    char* token;
    int lastindex = strlen(abs_path) - 1;

    // tokenize request then pick out the method, uri http version and CRLF
    token = strtok(buffer, " ");
    for(int i=0; i<3; i++) {
        tokens[i] = token;
        token = strtok(NULL, " ");
    }

    token = strtok(tokens[2], CRLF);
    tokens[2] = token;
    
    if (filepath_zoomout_handler(tokens[1], abs_path)) {
        write(sockfd, FNF_MSG, sizeof(FNF_MSG)-1);
        close(sockfd);
        return -1;
    }

    strcat(abs_path, tokens[1]);

    // ensuring correct request format
    if (tokens[0] == NULL || tokens[1] == NULL || tokens[2] == NULL) {
        write(sockfd, FNF_MSG, sizeof(FNF_MSG)-1);
        close(sockfd);
        return -1;
    }

    if (strcmp(tokens[0], GET_METHOD) != 0) {
        write(sockfd, FNF_MSG, sizeof(FNF_MSG)-1);
        close(sockfd);
        return -1;
    }
    
    else if (strcmp(tokens[2], HTTP_VERSION) != 0) {
        write(sockfd, FNF_MSG, sizeof(FNF_MSG)-1);
        close(sockfd);
        return -1;
    }

    else if (abs_path[lastindex] == '/') {
        write(sockfd, FNF_MSG, sizeof(FNF_MSG)-1);
        close(sockfd);
        return -1;
    }

    return 1;

}

// function which handles the response to a valid HTTP GET request
void response_handler(int sockfd, char abs_path[], char file_type[]) {
    int filereader;
    FILE *flptr;
    
    // checking that the requested filepath is valid
    flptr = fopen(abs_path, "r");


    if (flptr == NULL) {
        write(sockfd, FNF_MSG, sizeof(FNF_MSG)-1);
        close(sockfd);
        return;
    }
    
    // this if-else guard is for writing the HTTP header to the client
    if (strcmp(file_type, "html") == 0) {
        write(sockfd, CONTENT_TYPE_HTML, sizeof(CONTENT_TYPE_HTML)-1);
    }


    else if (strcmp(file_type, "jpg") == 0) {
        write(sockfd, CONTENT_TYPE_JPEG, sizeof(CONTENT_TYPE_JPEG)-1);
    }

    
    else if (strcmp(file_type, "css") == 0) {
        write(sockfd, CONTENT_TYPE_CSS, sizeof(CONTENT_TYPE_CSS)-1);
    }
    
    
    else if (strcmp(file_type, "js") == 0) {
        write(sockfd, CONTENT_TYPE_JAVASCRIPT, sizeof(CONTENT_TYPE_JAVASCRIPT)-1);
    }

    
    else {
        write(sockfd, CONTENT_TYPE_UNDEFINED, sizeof(CONTENT_TYPE_UNDEFINED)-1);
    }

    // get length of file
    fseek(flptr, 0L, SEEK_END);
    long int flen = ftell(flptr);
    fclose(flptr);


    // now use open() to enable use of sendfile()
    filereader = open(abs_path, 0, "r");

    /*  I found sendfile() more practical than alternatively reading the file into a buffer and using send(),
    as this is easier and more efficient than manually transferring the contents of the file. 
    This also means there is no chance of problems occuring when reading the file into a buffer,
    and no mutation of the file contents.  */
    sendfile(sockfd, filereader, 0, flen);
    close(filereader);
}