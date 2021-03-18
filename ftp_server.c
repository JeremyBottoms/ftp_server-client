/*****************************************
 * Names: Kendra Marckini & Jeremy Bottoms
 * Course: CIS457-30
 * Program: Building a FTP Server - Server
 *****************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_BUFF 1000

void checkError(int type, int num);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen, n, x, i;
    char buffer[MAX_BUFF], myFile[MAX_BUFF], data[MAX_BUFF];
    char commands[5][10] = {"CONNECT","LIST","RETRIEVE","STORE","QUIT"};
    struct sockaddr_in serv_addr, cli_addr;

    if(argc < 2){
        perror("No port provided");
        exit(1);
    }

    sockfd = socket(AF_INET,SOCK_STREAM,0);                                 //Create socket
    if(sockfd < 0)
        perror("Could not open socket");
    bzero((char *) &serv_addr,sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if(bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)   //Bind the socket to the server address
        perror("Could not bind");

    listen(sockfd,5);                                                       //Listen for connections with the system
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);       //Accept a connection
    if(newsockfd < 0)
        perror("Could not accept socket");

    bzero(buffer,MAX_BUFF);                                                 //Verify connection is established
    n = read(newsockfd,buffer,MAX_BUFF-1);
    checkError(0,n);
    printf("\n\n\n\n%s\n\n",buffer);

    for(;;)
    {
        bzero(buffer,MAX_BUFF);                                             //Read data from the socket
        n = read(newsockfd,buffer,MAX_BUFF-1);
        checkError(0,n);

        if(strncmp(commands[1],buffer,strlen(commands[1])) == 0)            //LIST
        {
            printf("\nLIST\n\n");

            DIR *dir;                                                       //Open directory
            struct dirent *d;
            dir = opendir(".");
            bzero(buffer, MAX_BUFF);

            while((d = readdir(dir)) != NULL){                              //Send file names to socket
                if(!strcmp(d->d_name,".") || !strcmp(d->d_name,".."))
                {}
                else{
                    bzero(buffer,MAX_BUFF);                                 //Write files to socket
                    n = write(newsockfd,d->d_name,MAX_BUFF);
                    checkError(1,n);
                    sleep(1);

                    n = read(newsockfd,buffer,MAX_BUFF-1);                  //Verify text sent correctly
                    checkError(0,n);
                    if(strncmp("GOT",buffer,3) != 0){
                        perror("Verification Failed");
                    }
                    else
                        bzero(buffer,MAX_BUFF);
                }
            }
            closedir(dir);                                                  //Close directory

            bzero(buffer,MAX_BUFF);                                         //Let client know all files were sent
            n = write(newsockfd,"END",3);
            checkError(1,n);
        }
        else if(strncmp(commands[2],buffer,strlen(commands[2])) == 0)
        {
            printf("\nRETRIEVE\n");                                         //RETRIEVE

            n = write(newsockfd,"WORKING",7);
            checkError(1,n);

            bzero(buffer,MAX_BUFF);                                         //Read file name from socket
            n = read(newsockfd,buffer,MAX_BUFF-1);
            checkError(0,n);
            myFile[0] = '\0';
            strcat(myFile,buffer);
            bzero(buffer,MAX_BUFF);
            printf("File name: %s\n",myFile);

            DIR *dir;                                                       //Open directory
            struct dirent *d;
            dir = opendir(".");
            x = 0;

            while((d = readdir(dir)) != NULL){                              //Search directory for file name
                if(!strcmp(d->d_name,".") || !strcmp(d->d_name,".."))
                {}
                else{
                    if(strcmp(myFile,d->d_name) == 0){
                        x = 1;

                        n = write(newsockfd,"EXSITS",6);                    //Let client know file was found
                        checkError(0,n);
                        printf("File found\n\n");
                        sleep(1);
                        data[MAX_BUFF];

                        FILE *fp;                                           //Open file for reading
                        fp = fopen(myFile,"r");
                        if(fp == NULL){
                            perror("File cannot be opened");
                            exit(1);
                        }

                        for(;;){                                            //Send contents of file to socket
                            bzero(data,MAX_BUFF);
                            if(fgets(data,MAX_BUFF,fp) == NULL){
                                printf("Done sending file data\n");
                                fclose(fp);
                                break;
                            }
                            n = write(newsockfd,data,sizeof(data));
                            checkError(1,n);
                            printf("Sending file data\n");
                        }
                        break;
                    }
                }
            }
            if(x == 0){
                n = write(newsockfd,"NOT",3);
                checkError(1,n);
            }
            else{
                n = write(newsockfd,"END",3);                               //Let client know server finished sending
                checkError(1,n);
                sleep(1);

                bzero(buffer,MAX_BUFF);                                     //Verify file was stored
                n = read(newsockfd,buffer,MAX_BUFF-1);
                checkError(0,n);
                if(strncmp(buffer,"GOT",3) == 0)
                    printf("\nFile retrieved\n\n");
                else
                    perror("File not retrieved");
            }
        }
        else if(strncmp(commands[3],buffer,strlen(commands[3])) == 0)
        {
            printf("\nSTORE\n");                                            //STORE

            bzero(buffer,MAX_BUFF);                                         //Read file name from socket
            n = read(newsockfd,buffer,MAX_BUFF-1);
            checkError(0,n);
            myFile[0] = '\0';
            strcat(myFile,buffer);
            printf("File name: %s\n",myFile);

            n = read(newsockfd,buffer,MAX_BUFF-1);                          //Verify file was created
            checkError(0,n);

            if(strncmp(buffer,"EXISTS",6) == 0){
                FILE *fp;                                                   //Create file
                fp = fopen(myFile,"w+");
                if(fp == NULL){
                    perror("File could not be created");
                    exit(1);
                }
                printf("\nFile Contents Begin------------------\n");

                for(;;){                                                    //Get file contents and store in file
                    bzero(buffer,MAX_BUFF);
                    n = read(newsockfd,buffer,MAX_BUFF);
                    checkError(0,n);
                    if(strncmp(buffer,"END",3) == 0)
                        break;
                    fprintf(fp,"%s",buffer);
                    printf("%s",buffer);
                }
                printf("File Contents End--------------------\n\n");
                fclose(fp);
                printf("File closed\n\n");

                n = write(newsockfd,"GOT",3);                               //Let client know file obtaining is complete
                checkError(1,n);
            }
        }
        else if(strncmp(commands[4],buffer,strlen(commands[4])) == 0)
        {
            printf("\nQUIT\n\nServer Exit...\n\n");                         //QUIT
            break;
        }
    }
    return 0;
}

void checkError(int type, int num)
{
    if(num < 0){
        if(type == 1){
            perror("Error writing to socket");
            exit(0);
        }
        else if(type == 0){
            perror("Error reading from socket");
            exit(0);
        }
    }
}
