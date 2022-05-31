#define IMPLEMENTS_IPV6
#define MULTITHREADED
#define _POSIX_C_SOURCE 200112L
#define CMD_LINE_PROMPTS_NO 4
#define IPv4_PROTOCOL_NO "4"
#define IPv6_PROTOCOL_NO "6"
#define MAX_CONCURR_REQ_NO 10 // Making this well above the upper limit of 5 concurrent HTTP requests
#define MAX_FILEPATH 2000

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sendfile.h>

#include "httplib.h"

void * connection_handler(void* p_socket);
void get_filetype(char combinedfp[], char filetype[]);

char relative_directory[MAX_FILEPATH];

/* Much of the socket-related code was adapted from code provided from the week 9 practical class.*/
int main(int argc, char** argv) {
    int sockfd, newsockfd, re, s;
	struct addrinfo hints, *res;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;

    // Guard to prevent invalid inputs such as incorrect no of args or invalid protocol
    if (argc != CMD_LINE_PROMPTS_NO || 
        (strcmp(argv[1], IPv4_PROTOCOL_NO) != 0 && strcmp(argv[1], IPv6_PROTOCOL_NO) != 0)) {
		perror("invalid input");
		exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // If protocol in args is IPv4
    if (strcmp(argv[1], IPv4_PROTOCOL_NO) == 0) {
        hints.ai_family = AF_INET;
    }
    // If protocol in args is IPv6
    else {
        hints.ai_family = AF_INET6;
    }

	// Taking the relative directory specified in command line
	int lenargv_3 = strlen(argv[3]);

	
	
	if (argv[3][lenargv_3-1] == '/') {
		strncpy(relative_directory, argv[3], lenargv_3-1);
	}
	else {
		strcpy(relative_directory, argv[3]);
	}

    s = getaddrinfo(NULL, argv[2], &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

    // Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

    // Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		close(sockfd);
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		close(sockfd);
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(sockfd, MAX_CONCURR_REQ_NO) < 0) {
		close(sockfd);
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// Accept a connection - blocks until a connection is ready to be accepted
	// Get back a new file descriptor to communicate on
	client_addr_size = sizeof client_addr;
	
	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
		if (newsockfd < 0) {
			close(newsockfd);
			perror("accept");
			exit(EXIT_FAILURE);
		}
		// now connected, handle with threads
		pthread_t thread;
		int *pclient = malloc(sizeof(int));
		*pclient = newsockfd;
		// creating thread to handle accepted client on the socket
		pthread_create(&thread, NULL, connection_handler, pclient);

		//detaching thread
		pthread_detach(thread);
		
	}
}

// function which is used by the pthread_create() call in main() to handle all client-server socket connection relations
void * connection_handler(void* p_socket) {
	int socketfd = *((int*)p_socket);
	free(p_socket);
	int message_size;
	char abs_path[MAX_GET_SIZE];
	char buffer[MAX_GET_SIZE+1];

	for (int i=0; i<=MAX_GET_SIZE; i++) {
		buffer[i] = '\0';
	}

	// reading in request via request_complete()
	message_size = request_complete(socketfd, buffer);
	
	if (message_size < 0) {
		perror("read");
		exit(EXIT_FAILURE);
		return NULL;
	}

	// taking the requested filepath in the HTTP GET request and concatenating with the relative directory
	strcpy(abs_path, relative_directory);

	// function which processes the request received on socket
	int rh = request_handler(socketfd, buffer, message_size, abs_path);

	char file_type[MAX_GET_SIZE];

	if (rh > 0) {
		get_filetype(abs_path, file_type);
		response_handler(socketfd, abs_path, file_type);
	}
	close(socketfd);
	return NULL;

}

// function which given a filepath, returns its filetype
void get_filetype(char combinedfp[], char filetype[]) {
	char *dummy;
    char tempfp[MAX_GET_SIZE];

    strcpy(tempfp, combinedfp);
	// tokenizing the abs_path by '.', retrieving the final token to obtain the filetype
	
    dummy = strtok(tempfp, ".");
	strcpy(filetype, dummy);

	while (1) {

		dummy = strtok(NULL, ".");

		if (dummy == NULL) {
			break;
		}

		strcpy(filetype, dummy);
	}

	return;
}