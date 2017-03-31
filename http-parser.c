//
// Created by benjamin on 29/03/17.
//

#include "http-parser.h"

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

//This will add the right path depending on the host
void sanitize_path(struct request *r){
    char host[255];

    printf("-- Looking for the host's path --");

    //Find the location for the host in the config file variables.
    for(int i = 0; i < header_index; i++){
        printf("%s\n", parsing_request.hlines[i].key);
        if(strcmp(parsing_request.hlines[i].key, "Host:")){
            for(int j = 0; j < number_of_host_entries; j++){
                if(strcmp(parsing_request.hlines[i].value, hosts[j].name)){
                    strcpy(host, hosts[j].value);
                    printf("  the location for the host is: %s \n", host);
                }
            }
        }
    }
    if(strcmp(r->rl.path, "/")){
        strcpy(r->rl.path, concat(host, "/index.html"));
    }
    else{
        strcpy(r->rl.path, concat(host, r->rl.path));
    }
    printf("  Final path of file is: %s \n", r->rl.path);
}

unsigned char* isHeaderComplete(unsigned char buffer[BUFFER_MAX]){
    return strstr(buffer,"\r\n\r\n");
}

void parseRequest(unsigned char buffer[BUFFER_MAX], struct request *r){

    char *string = (char *) malloc( BUFFER_MAX * sizeof(char));
    string = (char *) &buffer[0];

    char * curLine = (char *)&buffer[0];
    while(curLine)
    {
        char * nextLine = strstr(curLine, "\r\n");
        char * sectionEnd = strstr(curLine, "\r\n\r\n");
        if (nextLine) *nextLine = '\0';  // temporarily terminate the current line

        if(!parsing_request.is_header_ready){

            //Check if this is the last line and if it does set a flag
            if(strcmp(nextLine+2,"\n") ){
                printf("Headers end after parsing this line\n");
                parsing_request.is_header_ready = 1;
            }

            //If request line (the first line of the request haven't been read then parse it)
            if(!first_line_read) {
                //this struct is the container for the request line
                struct request_line parsing_request_line;
                if (sscanf(curLine, "%s %s %s",
                           parsing_request_line.type, parsing_request_line.path, parsing_request_line.http_v) == 3) {
                    first_line_read = 1;
                    parsing_request.rl = parsing_request_line;
                }
            }
            //Parse the header line and insert in the headers array
            else {
                struct header parsing_header;
                if (sscanf(curLine, "%s %s", parsing_header.key, parsing_header.value) == 2) {
                    parsing_request.hlines[header_index++] = parsing_header;
                }
            }
        }

        printf("parsed the line: %s in the request\n", curLine);

        if (nextLine) *nextLine = '\n';  // then restore newline-char, just to be tidy
        curLine = nextLine ? (nextLine+2) : NULL;
    }

    parsing_request.header_entries = header_index;
}



