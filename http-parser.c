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

    printf("\nLOOKING FOR HOST'S PATH:\n");

    //Find the location for the host in the config file variables.
    for(int i = 0; i < r->header_entries; i++){

        if(strcmp(parsing_request.hlines[i].key, "Host:") == 0){
            printf("  Found Host Key in Headers. Now searching for Key Hosts in config file\n");

            for(int j = 0; j < number_of_host_entries; j++){
                if(strstr(parsing_request.hlines[i].value, hosts[j].name)){
                    strcpy(host, hosts[j].value);
                    printf("  the location for the host is: %s \n", host);
                    break;
                }
            }

            break;
        }
    }
    if(strcmp(r->rl.path, "/") == 0){
        strcpy(r->rl.path, concat(host, "/index.html"));
    }
    else{
        strcpy(r->rl.path, concat(host, r->rl.path));
    }
    printf("  Final path of file is: %s \n", r->rl.path);
}

int isHeaderComplete(unsigned char buffer[BUFFER_MAX]){
    char *endOfHeader = strstr(buffer,"\r\n\r\n");
    if(endOfHeader){
        *endOfHeader = '\0';
        return 1;
    }
    return 0;
}

void parseRequestLine(char* currentLine){
    struct request_line parsing_request_line; //
    if (sscanf(currentLine, "%s %s %s",
               parsing_request_line.type, parsing_request_line.path, parsing_request_line.http_v) == 3) {
        first_line_read = 1;
        parsing_request.rl = parsing_request_line;
    }
    else{
        printf("INVALID REQUEST LINE\n");
        return;
    }
}

void parseHeaderLine(char* currentLine){
    struct header parsing_header;
    if (sscanf(currentLine, "%s %s", parsing_header.key, parsing_header.value) == 2) {
        parsing_request.hlines[header_index++] = parsing_header;
    }
}

void parseHeader(unsigned char buffer[BUFFER_MAX], struct request *r){

    printf("\nPARSING HEADER\n");
    char *currentLine = (char *)&buffer[0];

    //READ LINE BY LINE
    while(currentLine) {
        char *nextLine = strstr(currentLine, "\r\n"); //Search for the string \r\n

        if (!r->is_header_ready && !nextLine) {
            strcpy(r->incompleteLine, currentLine);
            r->fragmented_line_waiting = 1;
            break;
        } else {
            if (nextLine) *nextLine = '\0';  // temporarily terminate the current line

            if(r->fragmented_line_waiting){

            }
            if (!first_line_read) {
                parseRequestLine(currentLine);
            }
            else {
                parseHeaderLine(currentLine);
            }

            printf("  parsed the line: %s \n", currentLine);
            if (nextLine) *nextLine = '\n';  // then restore newline-char, just to be tidy
            currentLine = nextLine ? (nextLine + 2) : NULL;
        }
    }
    parsing_request.header_entries = header_index;
}

void getContentType(struct request *r, char *contentType){
    printf("\nFIGURING CONTENT TYPE\n");
    char* extension;
    char *c = strchr(r->rl.path,'.');
    if(c){
        extension = (char*)malloc(sizeof(char)*strlen(c));
        strcpy(extension, c);
        printf("  Found extension %s\n", extension);
    }
    if(++extension){
        for(int i = 0; i < number_of_media_entries; i++){
            if(strcmp(media[i].type, extension) == 0){
                strcpy(contentType, media[i].value);
                printf("  Found content type for extension: %s\n", contentType);
                return;
            }
        }
    }

    printf("File has no extension. Setting content type to default: text/plain");
    strcpy(contentType,"text/plain");
    free(extension);
}


void executeGet(struct request *r, int sock){
    printf("\nEVALUATING GET REQUEST\n");
    struct stat sb;
    //-----------------------GET FILE INFO----------------------//
    char *path = (char*) &r->rl.path;
    if (stat(path, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    //---------------------GET RESPONSE CODE--------------------//
    char responseCode[32];
    FILE* sendFile;
    if ( ( sendFile = fopen( r->rl.path, "r" ) ) == NULL ) {
            perror("fopen");
             strcpy(responseCode,"404 Not Found");
    }
    else{
        strcpy(responseCode,"200 OK");
    }

    //------------------------GET TIME--------------------------//
    char timeString[40];
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(timeString, sizeof(timeString), "%a, %d %b %Y %H:%M:%S %Z", timeinfo);

    //------------------------GET LAST MODIFIED TIME----------------//
    char lastModified[40];
    struct tm * mTime;
    mTime = localtime (&sb.st_mtim);
    strftime(lastModified, sizeof(lastModified), "%a, %d %b %Y %H:%M:%S %Z", mTime);

    //---------------------GET CONTENT TYPE---------------------//
    char contentType[200];
    char* contentTypePtr = (char*)&contentType;
    getContentType(r, contentTypePtr);

    //---------------------PRINT COMPLETED RESPONSE HEADER---------------------//
    unsigned char send_buffer[MAX_LEN];
    if(snprintf(send_buffer, MAX_LEN,
            "HTTP/1.1 %s\r\n"
                    "Date: %s\r\n"
                    "Server: Sadam Husein\r\n"
                    "Content-Type: %s\n"
                    "Content-Length: %i\r\n"
                    "Last-Modified: %s\r\n"
                    "\r\n", responseCode, timeString, contentType, sb.st_size, lastModified)< 0){
        printf("sprintf Failed\n");
    }
    printf("  Response Header: \n%s\n", send_buffer);
    //---------------------SEND HEADER---------------------------------//
    unsigned char *send_buffer_ptr = send_buffer;
    int bytes_sent;
    do {
        bytes_sent = send(sock, send_buffer_ptr, strlen(send_buffer)+1, 0);
        printf("  Sent %i bytes\n", bytes_sent);
        send_buffer_ptr += bytes_sent;
        printf("  Still have to send: %s \n", send_buffer_ptr);
    }while(bytes_sent < strlen(send_buffer));

    //---------------------SEND RESPONSE BODY--------------------------//
    while( !feof(sendFile) )
    {
        int numread = fread(send_buffer, sizeof(unsigned char), MAX_LEN, sendFile);
        if( numread < 1 ) break; // EOF or error

        unsigned char *send_buffer_ptr = send_buffer;
        do
        {
            int numsent = send(sock, send_buffer_ptr, numread, 0);
            if( numsent < 1 ) // 0 if disconnected, otherwise error
            {

                break; // timeout or error
            }

            send_buffer_ptr += numsent;
            numread -= numsent;
        }
        while( numread > 0 );
    }
    send(sock, send_buffer, strlen(send_buffer)+1, 0);
}

void executeRequest(struct request *r, int sock){
    if(strcmp(r->rl.type, "GET") == 0){
        executeGet(r, sock);
    }
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



