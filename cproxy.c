#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h> 


#define MAX_URL_LENGTH 512
#define MAX_REQUEST_LENGTH 1048
#define MAX_RESPONSE_LENGTH 4096

void createDirectories(char *filepath_part2) {
    char *file =basename(filepath_part2);
    char currentPath[MAX_URL_LENGTH]="";  
    filepath_part2[strlen(filepath_part2)-strlen(file)]='\0';

    char *token =strtok(filepath_part2, "/");
    while (token!= NULL) {
        strcat(currentPath,token);
        strcat(currentPath,"/");
        struct stat st;
        if (stat(currentPath,&st) != 0) {// if directory doesn't exist, create it
            if (mkdir(currentPath,0777)== -1) {
                    perror("mkdir");
                    exit(EXIT_FAILURE); 
            }
        } 
        token=strtok(NULL,"/");
    }
}
int main(int argc,char *argv[]) {
    // check the number of command line arguments
    if (argc< 2 || argc >3) {
        fprintf(stderr, "Usage: cproxy <URL> [-s]\n");
        exit(EXIT_FAILURE);
    }

    char url[MAX_URL_LENGTH];
    strncpy(url, argv[1], sizeof(url));
    url[sizeof(url) - 1] = '\0'; 

    // check if the URL starts with "http://"
    if (strncmp(url,"http://",7)!= 0) {
        fprintf(stderr,"Usage: cproxy <URL> [-s]\n");
        exit(EXIT_FAILURE);
    }

   int useBrowser=0; // 0 for false, 1 for true for -s
    if (argc==3) {
        if (strcmp(argv[2],"-s") == 0) {
            useBrowser=1;
        } 
        else {
            fprintf(stderr,"Usage: cproxy <URL> [-s]\n");
            exit(EXIT_FAILURE);
        }
    }

    char hostname[MAX_URL_LENGTH];
    int port=80;
    char filepath[MAX_URL_LENGTH];
    filepath[0]='\0';
    hostname[0]='\0';

    if (sscanf(url,"http://%[^:/]:%d/%[^\n]",hostname,&port,filepath) == 3) {//check url format 
        if(url[strlen(url)-1]=='/'){strcat(filepath,"index.html");}
    } 
    else if (sscanf(url,"http://%[^:/]:%d%*[^\n]",hostname,&port) == 2) {
        strcat(filepath,"/index.html");

    }                  
    else if (sscanf(url,"http://%[^/]%[^\n]",hostname,filepath) == 2) {
        if(url[strlen(url)-1]=='/'){strcat(filepath,"index.html");}
    } 
    else if (sscanf(url,"http://%[^/]%*[^\n]",hostname) == 1) {
        strcpy(filepath,"/index.html");
        
    } 
    else {
        fprintf(stderr,"Usage: cproxy <URL> [-s]\n");
        exit(EXIT_FAILURE);
    }

    char filepath_part[MAX_URL_LENGTH];
    strcpy(filepath_part,hostname);
    strcat(filepath_part,filepath);

    FILE *file_handle=fopen(filepath_part,"r"); 
    if (useBrowser==1) {
        if (file_handle==NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        char browserCommand[MAX_URL_LENGTH + 15]; // add space for the command "firefox"
        snprintf(browserCommand,sizeof(browserCommand),"firefox %s",filepath_part);
        system(browserCommand);
        fclose(file_handle);
    }
    else if (file_handle!=NULL) {
        printf("File is given from local filesystem\n");
        int totalbytes=0;

        // get the size of the file
        fseek(file_handle,0,SEEK_END);
        long file_size=ftell(file_handle);
        fseek(file_handle,0,SEEK_SET);

        // Calculate the size of the HTTP response 
        long headers_size = snprintf(NULL,0,"HTTP/1.0 200 OK\nContent-Length: %ld\n\n",file_size);

        // print HTTP response 
        printf("HTTP/1.0 200 OK\n");
        printf("Content-Length: %ld\n",file_size);
        printf("\n");

        // Read and print the content of the file using fread
        char buffer[MAX_RESPONSE_LENGTH];
        size_t bytesRead;
        while ((bytesRead=fread(buffer, 1, sizeof(buffer), file_handle)) > 0) {
            printf("%.*s",(int)bytesRead, buffer); 
            totalbytes+=bytesRead;
        }

        fclose(file_handle);
        // total bytes received (headers + file content)
        long totalBytesRec=headers_size+totalbytes;
        printf("\nTotal received response bytes: %ld\n",totalBytesRec);
    }
    else{
        char request[MAX_REQUEST_LENGTH];
        char filepath_part2[MAX_URL_LENGTH];

        snprintf(request,sizeof(request),"GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",filepath,hostname);
        printf("HTTP request =\n%s\nLEN = %zu\n",request,strlen(request));
        strcpy(filepath_part2,filepath_part);

        // create a socket
        int clientSocket=socket(AF_INET,SOCK_STREAM,0);
        if (clientSocket==-1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        // set the server address 
        struct sockaddr_in serverAddress;
        serverAddress.sin_family=AF_INET;
        serverAddress.sin_port=htons(port);

        // convert hostname to IP address
        struct hostent *server=gethostbyname(hostname);
        if (server==NULL) {
            herror("gethostbyname");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }
        memcpy(&serverAddress.sin_addr.s_addr,server->h_addr,server->h_length);

        // connect to the server
        if (connect(clientSocket,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) == -1) {
            perror("connect");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // send  request to the server
        if (send(clientSocket,request,strlen(request), 0)==-1) {
            perror("send");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }     
        ssize_t totalbytes=0;
        ssize_t bytesRead;
        char buffer[MAX_RESPONSE_LENGTH]; // read data from the socket
        int find_header_end= 0;

        FILE *newFile=NULL;
        while (1) {
            bzero(buffer,sizeof(buffer));
            bytesRead = read(clientSocket,buffer,sizeof(buffer));
            if (bytesRead<0) { //failed receiving data
                perror("Receiving response failed");
                exit(EXIT_FAILURE);
            } 
            else if (bytesRead>0) { //received data
                buffer[bytesRead]='\0';
                printf("%s",buffer);
                totalbytes+=bytesRead;
                int statusCode;
                sscanf(buffer,"HTTP/1.%*d %d",&statusCode);
                if (statusCode==200) {
                    if (!find_header_end) {
                        // search for the "\r\n\r\n" 
                        char *endOfHeader=strstr(buffer, "\r\n\r\n");
                        if (endOfHeader !=NULL) {
                            // calculate the offset of the end of header 
                            size_t offset =endOfHeader-buffer+4; // add 4 to include "\r\n\r\n"
                                    createDirectories(filepath_part2);

                            newFile=fopen(filepath_part,"wb");  // "ab" mode for  binary
                            if (newFile==NULL) {
                                perror("fopen");
                                close(clientSocket);
                                exit(EXIT_FAILURE);
                            }
                            size_t remaining=fwrite(endOfHeader+4,1,bytesRead-offset,newFile);
                            if (remaining!=bytesRead-offset) {
                                perror("fwrite");
                                fclose(newFile);
                                close(clientSocket);
                                exit(EXIT_FAILURE);
                            }
                            find_header_end=1; // end of header has been found so set the flag 
                        }
                    } 
                    else {
                        size_t bytes_Write = fwrite(buffer,1,bytesRead,newFile); // write the  buffer to the file 
                        if (bytes_Write!=bytesRead) {
                            perror("fwrite");
                            fclose(newFile);
                            close(clientSocket);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            } 
            else { // finish to read
                break;
            }
        }
        if (newFile!=NULL) {
            fclose(newFile); // Close the file 
        }
        printf("\nTotal received response bytes: %ld\n",totalbytes);
        close(clientSocket);
    }
    return 0;
}
