#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "./simpleSocketAPI.h"

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "0"
#define LISTEN_QUEUE_LENGTH 1
#define BUFFER_SIZE 1024
#define HOSTNAME_LENGTH 64
#define PORT_LENGTH 64

void handle_client(int clientSock, int rendezvousSock);

int main()
{
    int status;
    char serverAddress[HOSTNAME_LENGTH];
    char serverPort[PORT_LENGTH];
    int rendezvousSock;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_storage serverInfo;
    struct sockaddr_storage clientInfo;
    socklen_t addrLen;

    rendezvousSock = socket(AF_INET, SOCK_STREAM, 0);
    if (rendezvousSock == -1)
    {
        perror("Error creating rendezvous socket");
        exit(2);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    status = getaddrinfo(SERVER_ADDRESS, SERVER_PORT, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    status = bind(rendezvousSock, res->ai_addr, res->ai_addrlen);
    if (status == -1)
    {
        perror("Error binding rendezvous socket");
        exit(3);
    }

    freeaddrinfo(res);

    addrLen = sizeof(struct sockaddr_storage);
    status = getsockname(rendezvousSock, (struct sockaddr *)&serverInfo, &addrLen);
    if (status == -1)
    {
        perror("SERVER: getsockname");
        exit(4);
    }

    status = getnameinfo((struct sockaddr *)&serverInfo, sizeof(serverInfo), serverAddress, HOSTNAME_LENGTH,
                         serverPort, PORT_LENGTH, NI_NUMERICHOST | NI_NUMERICSERV);
    if (status != 0)
    {
        fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(status));
        exit(4);
    }
    printf("Listening address: %s\n", serverAddress);
    printf("Listening port: %s\n", serverPort);

    status = listen(rendezvousSock, LISTEN_QUEUE_LENGTH);
    if (status == -1)
    {
        perror("Error initializing listen buffer");
        exit(5);
    }

    while (1)
    {
        addrLen = sizeof(struct sockaddr_storage);
        int clientSock = accept(rendezvousSock, (struct sockaddr *)&clientInfo, &addrLen);
        if (clientSock == -1)
        {
            perror("Error accepting connection");
            exit(6);
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("Error forking");
            close(clientSock);
            continue;
        }
        else if (pid == 0)
        {
            close(rendezvousSock);
            handle_client(clientSock, rendezvousSock);
            exit(0);
        }
        else
        {
            close(clientSock);
        }
    }

    close(rendezvousSock);
    return 0;
}

void handle_client(int clientSock, int rendezvousSock)
{
    int status;
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *username = (char *)malloc(HOSTNAME_LENGTH * sizeof(char));
    char *hostname = (char *)malloc(PORT_LENGTH * sizeof(char));
    int serverSock;
    int clientPassiveSock;
    int serverPassiveSock;

    strcpy(buffer, "220 Connection to FTP server! :D\n");
    write(clientSock, buffer, strlen(buffer));

    status = read(clientSock, buffer, BUFFER_SIZE);
    if (status <= 0)
    {
        perror("Error reading USER command");
        close(clientSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(1);
    }

    char *user_token = strtok(buffer, " ");
    char *user_info = strtok(NULL, "@");
    char *host_info = strtok(NULL, "\r\n");

    if (user_token == NULL || user_info == NULL || host_info == NULL || strcmp(user_token, "USER") != 0)
    {
        perror("Error parsing USER command");
        close(clientSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(2);
    }

    strcpy(username, user_info);
    strcpy(hostname, host_info);

    printf("Hostname: %s\n", hostname);

    status = connect2Server(hostname, "21", &serverSock);
    if (status == -1)
    {
        perror("Error connecting to FTP server");
        close(clientSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(3);
    }

    memset(buffer, 0, BUFFER_SIZE);
    status = read(serverSock, buffer, BUFFER_SIZE);
    if (status <= 0)
    {
        perror("Error reading welcome message from FTP server");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(4);
    }
    buffer[status] = '\0';

    snprintf(buffer, BUFFER_SIZE, "USER %s\r\n", username);
    status = write(serverSock, buffer, strlen(buffer));
    if (status == -1)
    {
        perror("Error sending login to server");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(5);
    }

    memset(buffer, 0, BUFFER_SIZE);
    status = read(serverSock, buffer, BUFFER_SIZE);
    if (status == -1)
    {
        perror("Error receiving login response");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(5);
    }

    status = write(clientSock, buffer, strlen(buffer));
    if (status == -1)
    {
        perror("Error sending login response to client");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(5);
    }

    memset(buffer, 0, BUFFER_SIZE);
    status = read(clientSock, buffer, BUFFER_SIZE);
    if (status <= 0)
    {
        perror("Error reading PASS command");
        close(clientSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(6);
    }

    snprintf(buffer, BUFFER_SIZE, "PASS %s\r\n", buffer);
    status = write(serverSock, buffer, strlen(buffer));
    if (status == -1)
    {
        perror("Error sending PASS command to FTP server");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(7);
    }

    memset(buffer, 0, BUFFER_SIZE);
    status = read(serverSock, buffer, BUFFER_SIZE);
    if (status <= 0)
    {
        perror("Error reading response from FTP server after PASS");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(8);
    }

    buffer[status] = '\0';

    status = write(clientSock, buffer, strlen(buffer));
    if (status == -1)
    {
        perror("Error sending FTP server response to client");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(9);
    }

    snprintf(buffer, BUFFER_SIZE, "SYST\r\n");
    status = write(serverSock, buffer, strlen(buffer));
    if (status == -1)
    {
        perror("Error sending SYST command to FTP server");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(7);
    }

    memset(buffer, 0, BUFFER_SIZE);
    status = read(serverSock, buffer, BUFFER_SIZE);
    if (status <= 0)
    {
        perror("Error reading response from FTP server after SYST");
        close(clientSock);
        close(serverSock);
        close(rendezvousSock);
        free(buffer);
        free(username);
        free(hostname);
        exit(8);
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        status = read(clientSock, buffer, BUFFER_SIZE);
        if (status <= 0)
        {
            break;
        }

        if (strncmp(buffer, "PORT", 4) == 0)
        {
            int ip1, ip2, ip3, ip4, port1, port2;
            char *token = strtok(buffer, " ");
            token = strtok(NULL, ",");
            ip1 = atoi(token);
            token = strtok(NULL, ",");
            ip2 = atoi(token);
            token = strtok(NULL, ",");
            ip3 = atoi(token);
            token = strtok(NULL, ",");
            ip4 = atoi(token);
            token = strtok(NULL, ",");
            port1 = atoi(token);
            token = strtok(NULL, "\r\n");
            port2 = atoi(token);

            if (token == NULL)
            {
                perror("Error parsing PORT command");
                close(clientSock);
                close(serverSock);
                close(rendezvousSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(10);
            }

            char clientIP[INET_ADDRSTRLEN];
            sprintf(clientIP, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            char clientPort[6];
            sprintf(clientPort, "%d", port1 * 256 + port2);

            memset(buffer, 0, BUFFER_SIZE);
            strcpy(buffer, "PASV\r\n");
            status = write(serverSock, buffer, strlen(buffer));
            if (status == -1)
            {
                perror("Error sending PASV command to FTP server");
                close(clientSock);
                close(serverSock);
                close(rendezvousSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(10);
            }

            memset(buffer, 0, BUFFER_SIZE);
            status = read(serverSock, buffer, BUFFER_SIZE);
            if (status <= 0)
            {
                perror("Error reading PASV response from FTP server");
                close(clientSock);
                close(serverSock);
                close(rendezvousSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(11);
            }

            char *token2 = strtok(buffer, "(");
            token2 = strtok(NULL, ",");
            ip1 = atoi(token2);
            token2 = strtok(NULL, ",");
            ip2 = atoi(token2);
            token2 = strtok(NULL, ",");
            ip3 = atoi(token2);
            token2 = strtok(NULL, ",");
            ip4 = atoi(token2);
            token2 = strtok(NULL, ",");
            port1 = atoi(token2);
            token2 = strtok(NULL, ")");
            port2 = atoi(token2);

            char serverIP[16];
            sprintf(serverIP, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            char serverPort[6];
            sprintf(serverPort, "%d", port1 * 256 + port2);

            if (connect2Server(clientIP, clientPort, &clientPassiveSock) == -1)
            {
                perror("Error connecting to client FTP");
                close(clientSock);
                close(serverSock);
                close(rendezvousSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(3);
            }

            if (connect2Server(serverIP, serverPort, &serverPassiveSock) == -1)
            {
                perror("Error connecting to FTP server");
                close(clientSock);
                close(serverSock);
                close(rendezvousSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(3);
            }

            snprintf(buffer, BUFFER_SIZE, "200 PORT command successful.\n");
            write(clientSock, buffer, strlen(buffer));
        }
        else if (strncmp(buffer, "LIST", 4) == 0)
        {
            write(serverSock, buffer, strlen(buffer));

            memset(buffer, 0, BUFFER_SIZE);
            status = read(serverSock, buffer, BUFFER_SIZE);
            if (status <= 0)
            {
                perror("Error reading LIST response from FTP server");
                close(clientSock);
                close(serverSock);
                close(clientPassiveSock);
                close(serverPassiveSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(12);
            }
            write(clientSock, buffer, strlen(buffer));

            while ((status = read(serverPassiveSock, buffer, BUFFER_SIZE)) > 0)
            {
                write(clientPassiveSock, buffer, status);
            }

            close(clientPassiveSock);
            close(serverPassiveSock);

            memset(buffer, 0, BUFFER_SIZE);
            status = read(serverSock, buffer, BUFFER_SIZE);
            if (status <= 0)
            {
                perror("Error reading final LIST response from FTP server");
                close(clientSock);
                close(serverSock);
                free(buffer);
                free(username);
                free(hostname);
                exit(13);
            }
            write(clientSock, buffer, strlen(buffer));
        }
        else
        {
            buffer[status] = '\0';

            status = write(serverSock, buffer, strlen(buffer));
            if (status == -1)
            {
                perror("Error sending to FTP server");
                break;
            }

            memset(buffer, 0, BUFFER_SIZE);
            status = read(serverSock, buffer, BUFFER_SIZE);
            if (status <= 0)
            {
                break;
            }
            buffer[status] = '\0';

            status = write(clientSock, buffer, strlen(buffer));
            if (status == -1)
            {
                perror("Error sending to client");
                break;
            }
        }
    }

    close(clientSock);
    close(serverSock);
    free(buffer);
    free(username);
    free(hostname);
}