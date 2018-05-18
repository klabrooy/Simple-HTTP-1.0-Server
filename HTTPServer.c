/* A simple HTTP server in the internet domain using TCP
The port number is passed as the first argument
The root of the HTTP server is passed as the second argument
e.g. ./server 9999 ~/COMP30023

It responds to client GET requests with HTTP headers depending on whether
the resource exists
A new thread is created for each client request

To compile: gcc server.c -o server
* Written by Kara La'Brooy - 757553 - klabrooy@student.unimelb.edu.au
*/

#include "HTTPServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define MAX_BYTES_TO_WRITE 1024

// Data passed to each thread for a connection - file descriptor for communication
// and root directory as specified in command line args
struct thread_struct {
    int *fd;
    char *path;
};

int main(int argc, char **argv) {
	int sockfd, clientfd, portno, clilen, *newfd;
	struct sockaddr_in serv_addr, cli_addr;
	int n;
    char *root;

    signal(SIGPIPE, sigpipe_handler);

	if (argc < 3) {
		fprintf(stderr,"Incorrect Command Line args\n");
		exit(1);
	}

	 /* Create TCP socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)	{
		perror("ERROR opening socket");
		exit(1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);

    root = malloc(strlen(argv[2]));
	strcpy(root, argv[2]);

	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for
	 this machine */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	/* Listen on socket - ready to accept connections -
	 incoming connection requests will be queued */
	listen(sockfd, 5);

	clilen = sizeof(cli_addr);

	/* Accept a connection - block until a connection is ready to
	 be accepted. Get back a new file descriptor to communicate on. */
	while((clientfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen))) {
        char *ip = inet_ntoa(cli_addr.sin_addr);
        printf("New Connection from %s\r\n", ip);

        pthread_t a_thread;

        // Allocate memory for the size of the clientfd
        newfd = malloc(1);
        *newfd = clientfd;

        // Allocate memory for the struct to send to pthread
        struct thread_struct *ts = malloc(sizeof(struct thread_struct));
        ts->path = root;
        ts->fd = newfd;

        if (pthread_create(&a_thread, NULL, process, (void*)ts) < 0) {
            perror("Thread creation failed");
            exit(1);
        }

        printf("New Thread Created\r\n");
    }

    if (clientfd < 0) {
        perror("accept() failure");
        exit(1);
    }

    //new
    free(root);
    return 0;
}


void *process(void *ts) {
    char buffer[256];
    struct thread_struct *args = ts;
    char *result;

    // Set client socket
    int sock = *(int*)args->fd;
    // Set path to root set by thread creator
    char *path = args->path;

    printf("PATH:  %s\r\n", path);

    int n;

    // Peer into socket to get IP address
    socklen_t len;
    struct sockaddr_storage addr;
    len = sizeof(addr);
    getpeername(sock, (struct sockaddr*)&addr, &len);

    bzero(buffer, 256);

	// Read characters from the connection, then process
    n = read(sock, buffer, 255);

    if (n == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if (n == -1){
        perror("Read failed");
    }

    // ---------- START CHECKING HEADER ---------- //
    // REPSONSES MUST END WITH CARRAIGE RETURN /r AND NEWLINE /n
	char *get = "GET";
    char *reply;

    if(strncmp(get, buffer, strlen(get)) == 0) {
        // Set sizes for expected incoming items
        char request_uri[100];
        char http_version[100];

        // Slice buffer into variables delimited by " "
        strtok(buffer , " ");
        memset(request_uri, '\0', sizeof(request_uri));
        strcpy(request_uri, strtok(NULL, " "));
        memset(http_version, '\0', sizeof(http_version));
        strcpy(http_version , strtok(NULL, " "));

        // Print these values to Server console
        printf("REQUEST URI:  %s\r\n", request_uri);
        printf("HTTP VERSION:  %s\r\n", http_version);

        // The GET request was bad -> SEND 400 ERROR
        if (strncmp(http_version, "HTTP/1.0", 8)!=0 && strncmp(http_version, "HTTP/1.1", 8)!=0) {
            reply = "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n";
            write(sock, reply, 26);
            if (n < 0) {
    			       perror("ERROR writing req reply to socket");
    			       exit(1);
    	    }
		}
        // The GET request was OK -> CHECK FILE EXISTS
        else {
            // Create resulting file path by concatenating command line root with request_uri
            char *result = malloc(strlen(request_uri)+strlen(path)+1);
            strcpy(result, path);
            strcat(result, request_uri);
	        printf("FILE LOCATION: %s\n", result);

            // Try to open the file in read only binary mode
            FILE *file = fopen(result, "rb");

            // Check file exists (file pointer returned)
            if(file == NULL) {
                // File does NOT exist -> SEND 404 ERROR
                reply = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n";
                write(sock, reply, strlen(reply));
                if (n < 0) {
        	        perror("ERROR writing req reply to socket");
                    exit(1);
        	    }
            }

            else {
                // File exists -> SEND 200 RESPONSE, HEADER AND FILE
                // Find the file extension - the segment of path after '.'
                char fileExtension[6];
                strtok(request_uri, ".");
                memset(fileExtension, '\0', sizeof(fileExtension));
                strcpy(fileExtension, strtok(NULL, "."));
                printf("FILE EXTENSION: %s\n", fileExtension);

                // MIME Types
                char *html = "html";
                char *htmlType = "Content-Type: text/html\r\n\r\n";
                char *js = "js";
                char *jsType = "Content-Type: application/javascript\r\n\r\n";
                char *jpg = "jpg";
                char *jpgType = "Content-Type: image/jpeg\r\n\r\n";
                char *css = "css";
                char *cssType = "Content-Type: text/css\r\n\r\n";

                // Find MIME type based on file EXTENSION
                char *mimeType;
                if(strncmp(html, fileExtension, strlen(html)) == 0) {
                    mimeType = htmlType;
                }
                else if(strncmp(js, fileExtension, strlen(js)) == 0) {
                    mimeType = jsType;
                }
                else if(strncmp(jpg, fileExtension, strlen(jpg)) == 0) {
                    mimeType = jpgType;
                }
                else if(strncmp(css, fileExtension, strlen(css)) == 0) {
                    mimeType = cssType;
                }

                // ---------- START FILE MANIPULATION ---------- //
                int fileLen;
                // Get file length
                // Seek to end of file
                fseek(file, 0, SEEK_END);
                // Report position
                fileLen = ftell(file);
                // Reset pointer to start of file
                fseek(file, 0, SEEK_SET);

                // ---------- START HEADER MANIPULATION ---------- //
                // Response string ending in " \n" for concatenation
                char *httpResponse;
                httpResponse = "HTTP/1.0 200 OK\r\n";

                // Content-Length header
                char contentHeader[50];
                sprintf(contentHeader, "Content-Length: %d\r\n", fileLen);
                printf("char* contentHeader is %d char long\n", strlen(contentHeader));

                // Concatenate all pieces of HTTP header to deliver
                char *reply = concatenate(httpResponse, contentHeader, mimeType);

                // Write the header to the socket
                write(sock, reply, strlen(reply));
                if (n < 0) {
        	        perror("ERROR writing req reply to socket");
        		    exit(1);
        		}
                // ---------- END HEADER MANIPULATION ---------- //
                // The file buffer contains the maximum number of bytes to write per loop
                char fileBuffer[MAX_BYTES_TO_WRITE];

                // Keep looping and sending through data until end of file
                while (1) {
                    // Reset file buffer each time
                    memset(fileBuffer, 0, MAX_BYTES_TO_WRITE);
                    // Read one byte at a time from the open file into the file buffer
                    // Keep track of position using bytesRead
                    int bytesRead = fread(fileBuffer, 1, MAX_BYTES_TO_WRITE, file);
                    // Print to server console for debug
                    printf("Bytes Read: %d\n", bytesRead);

                    if (bytesRead > 0) {
                        // Write what is in buffer to socket
                        write(sock, fileBuffer, bytesRead);
                    }
                    if (bytesRead < MAX_BYTES_TO_WRITE) {
                        // Should have reached the end of file
                        if (feof(file)) {
                            printf("End of File\n");
                            printf("File Transfer Complete\n");
                        }
                        if (ferror(file)) {
                            printf("Error Reading\n");
                        }
                        // Break out of loop
                        break;
                    }
                }
                fclose(file);
                // ---------- END FILE MANIPULATION ---------- //
                // Free memory associated with reply
                free(reply);
            }
            // Free memory associated with concatenated file path
            free(result);
        }
    }
    // ---------- END CHECKING HEADER ---------- //
    // Free structs passed to thread
    free(ts);
    // Return thread
    return 0;
}

void sigpipe_handler(int signo) {
    // Do not terminate
}

/* Concatenates three strings a, b, c using memcpy */
char *concatenate(const char *a, const char *b, const char *c) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    size_t clen = strlen(c);
    char *res = malloc(alen + blen + clen + 1);
    memcpy(res, a, alen);
    memcpy(res + alen, b, blen);
    memcpy(res + alen + blen, c, clen);
    return res;
}
