/* ==========================================================================*/
/* 
    Name:   Brian Bowden || 10060818
    Date:   October 10, 2018
    Class:  CPSC 441 || Tut 04
    Asg#:   1 - Bad Spelling Web Proxy
    Source file
*/
/* 
    Source: TCPProxy.c
    Executable: ./myProxy
-------------------------------------------------------------------------------
    Additional References:
        https://beej.us/guide/bgnet/
        https://www.mkssoftware.com/docs/man3/select.3.asp

-------------------------------------------------------------------------------
    
    Please read README file before running

-------------------------------------------------------------------------------
    Known Bugs:
        aggressive page refreshes results in stack smashing
        not multi-threaded (only one active page at a time gentlemen & ladies)
        any image type is not properly handled, can't figure this one out
    
    Additional Comments:
        Not gonna lie, I started off okay with inline comments, got very lazy 
        and complacent towards the end of the assignment.

        I did all of my testing via proxy settings in Firefox

        The Main loop will exit itself, the timeout is set to 10 seconds
*/
/* ==========================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>


#include "TCPProxy.h"


int main (int argc, char *argv[]){

    /* create all variables */

    // address_server init variables
    struct sockaddr_in server_address;
    uint16_t port;

    // socket variables
    int server_socket;
    int accept_socket;

    //binding variables
    int bind_status;

    //listening variables
    int listen_status;

    //timeout variables
    fd_set set;
    struct timeval timeout;
    int rv;

    //buffer variables
    char *receive_ptr;
    char *GET_request;
    char *header;
    char *host;
    char* end_of_line;
    int msg_length;
    int bytes_sent;
    int bytes_rcvd;

    //client proxy variables
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *attempt_connect;
    int addr_status;

    //connection request variables
    int conn_status;
    int client_socket;

    // send variables
    // reusing bytes_sent
    char* content_type;
    char* content_length;
    char* sent_protocol;
    char* modified;
    char* send_ptr;
    int body_length;


    /* address initialization */
    port = 6666;
    startProxy(&server_address, &server_socket, port);

    /* Binding Sockets on Server Side */
    bind_status = bind(server_socket, (struct sockaddr *) &server_address, sizeof(sockaddr_in));
    if (bind_status == -1){
        close(server_socket);
        handleError(2);
    }

    /* Listening/Accept/Main Loop */
    while(1){

        /* start listening */
        listen_status = listen(server_socket, 1);
        if (listen_status == -1){
            close(server_socket);
            handleError(3);
        }

        /* initialize timeout stuff */
        FD_ZERO(&set);
        FD_SET(server_socket, &set);
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        /* Don't listen forever */
        rv = select(server_socket + 1, &set, NULL, NULL, &timeout);
        if (rv == -1){
            close(server_socket);
            handleError(10);
        }
        if (rv == 0){
            printf("timeout\n");
            break;
        }
        /* credit to user: mpromonet from https://stackoverflow.com/questions/3444729/using-accept-and-select-at-the-same-time
        modified slightly */


        /* accept connection */
        accept_socket = accept(server_socket, NULL, NULL);
        if (accept_socket == -1){
            close(server_socket);
            handleError(4);
        }

        printf("accepted connection\n");

        /* Receive GET request */
        receive_ptr = (char*)malloc(1000 * sizeof(char));
        memset(receive_ptr, 0, 1000);
        bytes_rcvd = recv(accept_socket, receive_ptr, sizeof(char) * 1000, 0);
        if (bytes_rcvd == -1){
            close(server_socket);
            close(accept_socket);
            handleError(6);
        }

        printf("recieved something........ standby\n");
        printf("message received:\n%s", receive_ptr);

        /* Parse GET Request */
        GET_request = (char*)malloc(4000 * sizeof(char));
        memset(GET_request, 0, 4000);
        host = (char*)malloc(100 * sizeof(char));
        memset(host, 0, 100);

        // Only Accept requests in the form of "GET"
        header = strstr(receive_ptr, "GET");
        if (header == NULL){
            close(server_socket);
            close(accept_socket);
            handleError(9);
        }

        host = parseHost(receive_ptr);

        /* make sure we haven't loaded the page recently */
        char* range;
        modified = NULL; 
        range = NULL;
        modified = strstr(receive_ptr, "If-Modified-Since: ");
        range = strstr(receive_ptr, "Range: ");
        if (modified != NULL){
            strncpy(GET_request, receive_ptr, (modified - receive_ptr));
            modified = strstr(modified, "If-None-Match: ");
            modified = strchr(modified, '\n') + 1;
            strcat(GET_request, modified);
        }
        else if (range != NULL){
            strncpy(GET_request, receive_ptr, (range - receive_ptr));
            range = strstr(range, "If-Range: ");
            range = strchr(range, '\n') + 1;
            strcat(GET_request, range);
        }
        else{
            strcpy(GET_request, receive_ptr);
        }
        GET_request[strlen(GET_request)] = '\0';

        printf("\n%s\n", GET_request);

        // don't need receive pointer anymore
        free(receive_ptr);  
        receive_ptr = NULL;

        /* Pass to client side */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        addr_status = getaddrinfo(host, "http", &hints, &res);
        if (addr_status != 0){
            close(server_socket);
            close(accept_socket);
            printf("get addr error: %s\n", gai_strerror(addr_status));
            handleError(7);
        }
        printf("got address info\n");

        /* Connection Request */
        // loop through results and connect to the first socket we can
        attempt_connect = res;
        for (attempt_connect = res; attempt_connect != NULL; attempt_connect = attempt_connect->ai_next){
            if ((client_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
                printf("client: socket\n");
                continue;
            }
            if ((conn_status = connect(client_socket, res->ai_addr, res->ai_addrlen)) == -1){
                close(client_socket);
                printf("client: connect\n");
                continue;
            }
            break;
        }

        if (attempt_connect == NULL){
            close(server_socket);
            close(accept_socket);
            handleError(8);
        }

        printf("connected\n");

        /* Send GET */
        msg_length = strlen(GET_request);
        bytes_sent = send(client_socket, GET_request, msg_length, 0);
        if (bytes_sent == -1){
            close(server_socket);
            close(accept_socket);
            close(client_socket);
            handleError(5);
        }

        /* Receive Response && Handle */
        send_ptr = (char*)malloc(65535 * sizeof(char));
        memset(send_ptr, 0, 65535);
        receive_ptr = (char*)malloc(512 * sizeof(char)); // make sure this is big enough
        memset(receive_ptr, 0, 512);
        bytes_rcvd = recv(client_socket, receive_ptr, sizeof(char) * 512, 0);
        if (bytes_rcvd == -1){
            close(server_socket);
            close(accept_socket);
            close(client_socket);
            handleError(6);
        }

        printf("recieved something........ standby\n");
        printf("message received:\n%s", receive_ptr);

        /* Parse response and insert errors */
        char* end_of_response = NULL;
        int sanity = 0;
        content_type = NULL;
        content_length = NULL;

        sent_protocol = strstr(receive_ptr, "HTTP");
        content_type = strstr(receive_ptr, "Content-Type: ");
        content_length = strstr(receive_ptr, "Content-Length: ");
        body_length = actualLength(content_length);
        end_of_response = strstr(receive_ptr, "\r\n\r\n") + 4;
        sanity = (end_of_response - receive_ptr);
        strncpy(send_ptr, receive_ptr, sanity);
        if(strstr(sent_protocol, "404 Not Found") == NULL){
            if(strstr(content_type, "image") == NULL){
                strcat(send_ptr, end_of_response);
                body_length -= strlen(receive_ptr);
                body_length += sanity;
                
                while (body_length > 0){
                    bytes_rcvd = recv(client_socket, receive_ptr, MIN((sizeof(char) * 512), body_length), 0);
                    if (bytes_rcvd == -1){
                        close(server_socket);
                        close(accept_socket);
                        close(client_socket);
                        handleError(6);
                    }
                    strncat(send_ptr, receive_ptr, MIN((int)strlen(receive_ptr), body_length));
                    body_length -= MIN((int)strlen(receive_ptr), body_length);
                }


                /* Checking for html type */
                if (strstr(content_type, "html") != NULL){
                    printf("got into html\n");
                    // this function will add the errors
                    handleHTML(send_ptr, body_length);
                } 

                /* Check for Plain text type */
                if (strstr(content_type, "plain") != NULL){
                    handlePLAIN(send_ptr, body_length);
                }
            }
            else{
                /* This is where images would be handled, but it is not working......... */
                 memcpy(send_ptr + sanity, receive_ptr, 512 - sanity);
                sanity += (512 - sanity);
                while (body_length > 0){
                    bytes_rcvd = recv(client_socket, receive_ptr, MIN(512, body_length), 0);
                    if (bytes_rcvd == -1){
                        close(server_socket);
                        close(accept_socket);
                        close(client_socket);
                        handleError(6);
                    }
                    memcpy(send_ptr + sanity, receive_ptr, MIN(512, body_length));
                    sanity += MIN(512, body_length);
                    body_length -= MIN((int)strlen(receive_ptr), body_length);
                } 

            }
        }

        printf("\n\nsending:\n%s\n", send_ptr);

        msg_length = strlen(send_ptr);
        bytes_sent = send(accept_socket, send_ptr, msg_length, 0);
        if (bytes_sent == -1){
            close(server_socket);
            close(accept_socket);
            close(client_socket);
            handleError(5);
        }

        /* Close superfluous sockets */
        close(client_socket);
        close(accept_socket);


        /* deallocate memory */
        if (modified != NULL)
            printf("%p: modified pointer\n", (void*)modified);{
        }
        printf("%p: receive pointer\n", (void*)receive_ptr);
        free(receive_ptr);
        receive_ptr = NULL;
        printf("%p: GET pointer\n", (void*)GET_request);
        strcpy(GET_request, "");
        printf("%p: content pointer\n", (void*)content_type);
        content_type = NULL;
        printf("%p: length pointer\n", (void*)content_length);
        content_length =NULL;
        printf("%p: header pointer\n", (void*)header);
        header = NULL;
        printf("%p: host pointer\n", (void*)host);
        free(host);
    }

    /* Terminate Connection */
    close(server_socket);

    return 0;
}

void startProxy(struct sockaddr_in *address, int *prox_socket, int port){

    memset(address, 0, sizeof(&address)); //initialize address_server to zero
    address->sin_family = AF_INET;  //AF_INET == IPv4 protocol
    address->sin_port = htons(port); //convert little endian to big endian
    address->sin_addr.s_addr = htonl(INADDR_ANY); //IADDR_ANY == any local IP

    /* Create client socket, check for errors */
    *prox_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket %d opened\n", *prox_socket);
    if (*prox_socket == -1){
        handleError(1);
    }
}

char* parseAddress(char* request){
    // Create variable to pass back
    char *address;

    // Create helper vars
    char *host;
    char *after_GET;
    char *end_of_path;

    // find breakpoints
    after_GET = strchr(request, ' ') + 1;
    end_of_path = strchr(request, '\n');

    // allocate memory for strings
    host = (char*)malloc((end_of_path - after_GET) * sizeof(char));

    // copy host to respective var
    strncpy(host, after_GET, (end_of_path - after_GET));

    // add null terminator
    host[strlen(host)] = '\0';

    // allocate memory for the address
    address = (char*)malloc((strlen(host) * sizeof(char)) + 1);

    //put everything into address
    strcpy(address, host);
    free(host);

    return address;

}

char* parseHost(char* request){
    // create pass back variable
    char* host;

    // create helper vars
    char* GET_line;
    char* after_host;
    char* second_line;

    /* Find the Host: name */
    GET_line = strchr(request, '\n');
    after_host = strchr(GET_line, ' ') + 1;
    second_line = strchr(GET_line, 13); // carriage return == 13

    // allocate memory for the host
    host = (char*)malloc((second_line - after_host) * sizeof(char));

    // copy everythinginto the return
    strncpy(host, after_host, (second_line - after_host));

    // add NULL terminator
    host[strlen(host)] = '\0';

    return host;
}

int actualLength(char* content_length){

    // create pass back
    int actual;

    // helper variables
    char* start;
    char* end;
    char* length;

    // find the ' ' and the CR
    start = strchr(content_length, ' ') + 1;
    end = strchr(content_length, '\r');

    // allocate
    length = (char*)malloc((end - start)*sizeof(char));

    // copy the line
    strncpy(length, start, (end - start));

    // add NULL terminator
    actual = atoi(length);

    return(actual);

}

void handleHTML(char* response, int length){

    char* start_of_body;
    char* end_of_body;
    char random[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
    char dont_change[] = "\n\r\0\\\"\',.!@$#&{}()*+-=_:;?|1234567890<> ";

    start_of_body = strstr(response, "<html>") + 6;
    end_of_body = strstr(response, "</html>");
    srand(time(NULL));

    // move the deallocted pointer up by one
    while(start_of_body < end_of_body){
        if (*start_of_body == '<'){
            //do nothing, skip to the end of it
            start_of_body = strchr(start_of_body, '>') + 1;
        }
        else if (strchr(dont_change, *start_of_body) != NULL || *start_of_body == ' '){
            *start_of_body++;
        }
        else{
            if ((rand()%100 + 1) < 5){
                // "randomly" insert a letter
                *start_of_body++ = random[rand()%52];
            }
            else {
                *start_of_body++;
            }
        }
    }


}

void handlePLAIN(char* response, int length){

    char* start_of_body;
    char random[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
    char dont_change[] = "\n\r\\\"\',.!@$#&{}()*+-=_:;?|1234567890<> ";

    start_of_body = strstr(response, "\n\n") + 2;
    srand(time(NULL));

    // move the deallocted pointer up by one

    while(*start_of_body != '\0'){
        if (strchr(dont_change, *start_of_body) != NULL || *start_of_body == ' '){
            *start_of_body++;
        }
        else{
            if ((rand()%100 + 1) < 5){
                // "randomly" insert a letter
                *start_of_body++ = random[rand()%52];
            }
            else {
                *start_of_body++;
            }
        }
    }
}

void handleError(int error_code){

    switch(error_code){
        case 1:
            printf("socket creation failed, exiting proxy...\n");
            exit(-1);
        case 2:
            printf("socket binding failed, exiting proxy...\n");
            exit(-1);
        case 3:
            printf("listening failed, exiting proxy... \n");
            exit(-1);
        case 4:
            printf("accept call failed, exiting proxy...\n");
            exit(-1);
        case 5:
            printf("message sending failed, exiting proxy...\n");
            exit(-1);
        case 6:
            printf("message receiving failed, exiting proxy...\n");
            exit(-1);
        case 7:
            printf("getting address info failed, exiting proxy... \n");
            exit(-1);
        case 8:
            printf("connection failed, exiting proxy... \n");
            exit(-1);
        case 9:
            printf("Not a GET request, exiting proxy... \n");
            exit(-1);
        case 10: 
            printf("Timeval not cooperating, exiting proxy... \n");
            exit(-1);   
        default:
            printf("default is exiting program...\n" );
            exit(-1);
    }
}
