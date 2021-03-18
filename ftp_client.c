/*****************************************
 * Names: Kendra Marckini & Jeremy Bottoms
 * Course: CIS457-30
 * Program: Building a FTP Server - Client
 *****************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_BUFF 1000

void checkError(int type, int num);

int main()
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int ip[MAX_BUFF], portno, sockfd, n, i = 0, j = 0;
    char buffer[MAX_BUFF], myFile[MAX_BUFF], *args[MAX_BUFF], *ipAddress[MAX_BUFF], data[MAX_BUFF], *tok;
    char commands[5][10] = {"CONNECT","LIST","RETRIEVE","STORE","QUIT"};

    for(;;)
    {
        printf("\n\nCommand Line: ");
        bzero(buffer,MAX_BUFF);                                                                 //Get and store user input
        fgets(buffer,MAX_BUFF-1,stdin);

        if(strncmp(commands[0],buffer,strlen(commands[0])) == 0)                                //CONNECT
        {
            printf("CONNECT\n");
            i = 0;
            tok = strtok(buffer," \n");
            while(tok != NULL){
                args[i] = tok;
                if(i == 1)
                    ipAddress[0] = tok;
                if(i == 2)
                    portno = atoi(tok);
                tok = strtok(NULL," \n");
                i++;
            }
            args[i] = NULL;
            if(i != 3)
                printf("Missing arguments for connect\n\n");

            server = gethostbyname(args[1]);                                                    //Validate server
            if(server == NULL){
                perror("No such server");
                exit(0);
            }

            tok = strtok(ipAddress[0],".\n");                                                   //Validate IP address
            while(tok != NULL){
                ip[j] = atoi(tok);
                tok = strtok(NULL,".\n");
                j++;
            }
            ip[j] = 0;
            if((ip[0] != 127) || (j > 4)){
                perror("Invalid IP Address");
                exit(0);
            }
            for(i = 1; i < j; i++){
                if((ip[i] > 255) || (ip[i] < 0)){
                    perror("Invalid IP Address");
                    exit(0);
                }
            }

            if((portno > 65535) || (portno < 0)){                                               //Validate port number
                perror("Invalid Port Number");
                exit(0);
            }

            sockfd = socket(AF_INET,SOCK_STREAM,0);                                             //Connect socket to client
            if(sockfd < 0)
                perror("Could not open socket");
            bzero((char *) &serv_addr,sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
            serv_addr.sin_port = htons(portno);
            if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
                perror("Could not connect");

            n = write(sockfd,"Connection Established",strlen("Connection Established"));
            checkError(1,n);
        }
        else if(strncmp(commands[1],buffer,strlen(commands[1])) == 0)                           //LIST
        {
            printf("LIST\n");
            n = write(sockfd,buffer,strlen(buffer));                                            //Let server know LIST
            checkError(1,n);

            while(strncmp("END",buffer,3) != 0){
                bzero(buffer,MAX_BUFF);
                n = read(sockfd,buffer,MAX_BUFF-1);                                             //Read and print list from socket
                printf("%s\n",buffer);
                checkError(0,n);

                n = write(sockfd,"GOT",3);                                                      //Let server know the file name from the list was received
                checkError(1,n);
            }
        }
        else if(strncmp(commands[2],buffer,strlen(commands[2])) == 0)                           //RETRIEVE
        {
            printf("RETRIEVE\n");

            n = write(sockfd,commands[2],strlen(commands[2]));                                  //Let server know RETRIEVE
            checkError(1,n);

            tok = strtok(buffer," \n");                                                         //Get file name
            i = 0;
            while(tok != NULL){
                args[i] = tok;
                tok = strtok(NULL," \n");
                i++;
            }
            myFile[0] = '\0';
            strcpy(myFile,args[1]);
            printf("File name: %s\n",myFile);

            while(strncmp("WORKING",buffer,7) != 0){
                n = read(sockfd,buffer,MAX_BUFF-1);
                checkError(0,n);
            }

            n = write(sockfd,myFile,strlen(myFile));                                            //Write file name to socket
            checkError(1,n);

            while((strncmp("EXSITS",buffer,6) != 0)){                                           //Wait for client to catch up
                n = read(sockfd,buffer,MAX_BUFF-1);
                checkError(0,n);
                if(strncmp(buffer,"NOT",3) == 0)
                    break;
            }

            if(strncmp(buffer,"EXSITS",6) == 0){

                printf("Retrieved file name: %s\n",myFile);
                FILE *fp;                                                                       //Create file
                fp = fopen(myFile,"w+");
                if(fp == NULL){
                    perror("File could not be created");
                    exit(0);
                }
                printf("Copying File \n\nFile Contents Begin------------------\n");

                for(;;){                                                                        //Get file contents and store in file
                    bzero(buffer,MAX_BUFF);
                    n = read(sockfd,buffer,MAX_BUFF);
                    checkError(0,n);
                    if(strncmp(buffer,"END",3) == 0)
                        break;
                    fprintf(fp,"%s",buffer);
                    printf("%s",buffer);
                }
                printf("File Contents End--------------------\n\n");
                fclose(fp);
                printf("File Retrieved\n\n");

                n = write(sockfd,"GOT",3);                                                      //Let server know the file was retrieved
                checkError(1,n);
            }
            else
                printf("File does not exist");
        }
        else if(strncmp(commands[3],buffer,strlen(commands[3])) == 0)                           //STORE
        {
            printf("STORE\n");

            n = write(sockfd,commands[3],strlen(commands[3]));                                  //Let server know STORE
            checkError(1,n);

            sleep(1);

            tok = strtok(buffer," \n");                                                         //Get file Name
            i = 0;
            while(tok != NULL){
                args[i] = tok;
                tok = strtok(NULL," \n");
                i++;
            }
            args[i] = 0;
            myFile[0] = '\0';
            strcat(myFile,args[1]);
            printf("File name: %s\n",myFile);

            n = write(sockfd,myFile,strlen(myFile));                                            //Send file name to server
            checkError(1,n);
            sleep(1);

            FILE *fp;                                                                           //Open file for reading
            fp = fopen(myFile,"r");
            if(fp == NULL){
                printf("File cannot be opened");
                n = write(sockfd,"NOT",3);
                checkError(1,n);
            }
            else{
                n = write(sockfd,"EXISTS",6);
                checkError(1,n);

                for(;;){                                                                        //Send contents of file to server
                    bzero(data,MAX_BUFF);
                    if(fgets(data,MAX_BUFF,fp) == NULL)
                        break;

                    n = write(sockfd,data,sizeof(data));
                    checkError(1,n);
                    printf("Sending file data\n");
                }
                printf("Done sending file data\n");
                fclose(fp);
                printf("File closed\n\n");

                n = write(sockfd,"END",3);                                                      //Let serve know client finished sending
                checkError(1,n);
                sleep(1);

                bzero(buffer,MAX_BUFF);
                n = read(sockfd,buffer,MAX_BUFF-1);                                             //Verify file was stored
                checkError(0,n);
                if(strncmp(buffer,"GOT",3) == 0)
                    printf("\nFile %s stored\n\n",myFile);
                else
                    perror("File not stored");
            }
        }
        else if(strncmp(commands[4],buffer,strlen(commands[4])) == 0)                           //QUIT
        {
            printf("QUIT\n\nClient Exit...\n\n");

            n = write(sockfd,commands[4],strlen(commands[4]));                                  //Let server know QUIT
            checkError(1,n);
            break;
        }
        else
            printf("\nCommand Unknown\n");
    }
    sleep(1);
    close(sockfd);
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
