//
// Created by benjamin on 29/03/17.
//

#ifndef SERVER_HTTP_PARSER_H
#define SERVER_HTTP_PARSER_H
#endif //SERVER_HTTP_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config-parse.h"

#define BUFFER_MAX	1024

struct request_line {
    char type[10];
    char path[256];
    char http_v[16];
};

struct header{
    char key[256];
    char value[512];
};

struct request{

    // CONTAINERS
    struct request_line rl;   //Contains the request line
    struct header hlines[512];//Contains all the header lines parsed.
    char contenttype[255];    //Contains
    char body[1024];          //Contains the body of the request.
    char incompleteLine[255]; //Contains any incomplete lines of body that couldn't fit in buffer.

    //REQUEST INFO
    int parsed_body;    // The amount of bytes that we have in the body
    int content_length; // The total amount of bytes of the body
    int header_entries; // The total number of header entries parsed.

    //FLAGS
    int is_header_ready;// Tells if the header is complete.
    int is_body_ready;  // Tells if the body is complete.
};

//This tells the http parser if the first line being read if the first line of the request.
int first_line_read;
int header_index;
struct request parsing_request;



void sanitize_path(struct request *r);
unsigned char* isHeaderComplete(unsigned char buffer[BUFFER_MAX]);
void parseRequest(unsigned char buffer[BUFFER_MAX], struct request *r);
