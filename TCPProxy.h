/* ==========================================================================*/
/* 
    Name:   Brian Bowden
    Date:   October 10, 2018
    Class:  CPSC 441 || Tut 04
    Asg#:   1 - Bad Spelling Web Proxy
    Header file
*/
/* 
    Source: TCPProxy.c
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
*/
/* ==========================================================================*/
#ifndef TCPPROXY_H
#define TCPPROXY_H

#define HTTP_PORT 8080
#define MAX(a,b)((a>b)?a:b)
#define MIN(a,b)((a<b)?a:b)

void startProxy(struct sockaddr_in *address, int *socket, int port);
char* parseAddress(char* request);
char* parseHost(char* request);
int actualLength(char* content_length);
void handleHTML(char* response, int length);
void handlePLAIN(char* response, int length);
void handleError(int error_code);

#endif
