#include <stdio.h>
#include <sys/socket.h>

#include "simple_telnet_server.h"

/**
 Authentication
*/
int authentication(int newsockfd, char buffer[256])
{
    int username;
    int password;
    char userbuffer[256];
    char passbuffer[256];

    write(newsockfd, WELCOME_MESSAGE, sizeof(WELCOME_MESSAGE));
    write(newsockfd, PROMPT_USERNAME, sizeof(PROMPT_USERNAME));
    username = read(newsockfd, buffer, 256);
    strcpy(userbuffer, buffer);
    write(newsockfd, PROMPT_PASSWORD, sizeof(PROMPT_PASSWORD));
    password = read(newsockfd, buffer, 256);
    strcpy(passbuffer, buffer);

    if ((strncmp(USERNAME, userbuffer, username - 2) == 0) && (strncmp(PASSWORD, passbuffer, password - 2) == 0)){
        return 1;
    }
    return 0;
}

/**
 * Method used to print the welcome screen of our shell
 */
void welcomeScreen(int new_socket)
{
  //sendHelpScreen(new_socket);
  send(new_socket, DEFAULTPROMPT, sizeof(DEFAULTPROMPT), 0);
}

/**
 * Method to send help docs
 */
void sendHelpScreen(int newsockfd)
{
    char str1[16] = "Commands list\n\n";
    char str2[26] = "clear       Clear screen\n";
    char str3[28] = "exit        Exit session\n\r\n";
    write(newsockfd, str1, sizeof(str1));
    write(newsockfd, str2, sizeof(str2));
    write(newsockfd, str3, sizeof(str3));
}

int main(int argc, char *argv[])
{
    int i;
    int sd;
    int ret;
    int max_sd;
    int activity;
    int addrlen;
    fd_set readfds;
    int opt = TRUE;
    int new_socket;
    int master_socket;
    int login_errors = 1;
    int authenticated = FALSE;
    struct sockaddr_in address;
    int max_clients = 30;
    int client_socket[30];
    char buffer[256];

    // initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    // 1、create a master socket
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket == 0) {
        printf("master socket create failed!\n");
        exit(EXIT_FAILURE);
    }

    // 2、set master socket to allow multiple connections
    ret = setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if (ret < 0) {
        printf("setsockopt failed!");
        exit(EXIT_FAILURE);
    }

    // 3、type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(LISTEN_IP);
    address.sin_port = htons(PORT);

    // 4、bind the socket
    ret = bind(master_socket, (struct sockaddr *)&address, sizeof(address));
    if (ret < 0) {
        printf("bind failed!");
        exit(EXIT_FAILURE);
    }
    printf("Listener at %s on port %d \n", LISTEN_IP, PORT);

    // 5、try to specify maximum of 3 pending connections for the master socket
    ret = listen(master_socket, 3);
    if (ret < 0) {
        printf("listen failed!\n");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);

    while (TRUE) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // add child sockets to set
        for (i = 0 ; i < max_clients ; i++) {
            //socket descriptor
            sd = client_socket[i];
            //if valid socket descriptor then add to read list
            if(sd > 0) {
                FD_SET( sd , &readfds);
            }
            //highest file descriptor number, need it for the select function
            if(sd > max_sd) {
                max_sd = sd;
            }
        }
        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("select activity error");
        }

        // If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                printf("accept error");
                exit(EXIT_FAILURE);
            }
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket ,inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                //if position is empty
                if(client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        }
        // Do Auth
        while (authenticated == 0) {
            if (login_errors > 3) {
                close(new_socket);
                client_socket[i] = 0;
                printf("Fatal Auth, ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                break;
            } else {
                authenticated = authentication(new_socket, buffer);
                login_errors++;
            }
        }
        login_errors = 1;
        welcomeScreen(new_socket);

        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];
            if (FD_ISSET( sd , &readfds)) {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read(sd, buffer, 256)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                } else { //Echo back the message that came in
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    // write help
                    if (strncmp("help", buffer, 4) == 0) {
                        sendHelpScreen(sd);
                    } else if (strncmp("clear", buffer, 4) == 0) { // clear
                        for (i = 0; i < 100; i = i +1) {
                            send(sd,"\n",1,0);
                        }
                    } else if (strncmp("exit", buffer, 4) == 0) { // exit
                        send(sd, "Good bye\n", 9, 0);
                        printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                        close(sd);
                        client_socket[i] = 0;
                    } else {
                        send(sd, COMANDO_INVALIDO, sizeof(COMANDO_INVALIDO), 0);
                        send(sd, buffer, strlen(buffer), 0);
                    }
                    //send(sd , buffer , strlen(buffer) , 0 );
                    send(sd, DEFAULTPROMPT, sizeof(DEFAULTPROMPT), 0);
                }
            }
        }
    }
    return 0;
}
