/* A simple client
   The hostname and port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void print_board(char board[])
{
    for (int i = 0; i < 9; i++)
    {
        if (board[i] == ' ')
            putchar('_');
        else
            putchar(board[i]);
        if (i % 3 == 2)
            putchar('\n');
    }
}

void play_game(int sockfd)
{
    char board[10] = "         ", send;
    // if (read(sockfd, &send, 1) == -1)
    //     printf("client read board error");
    // int id = send - '0';
    while (1)
    {
        // system("clear");
        printf("\x1b[H\x1b[J");
        print_board(board);
        if (read(sockfd, board, 10) == -1)
            printf("client read board error");
        printf("\x1b[H\x1b[J");
        print_board(board);
        printf("Choose position to mark: ");
        send = getchar();
        if (write(sockfd, &send, 1) == -1)
            perror("writing to server:");
        if (read(sockfd, board, 10) == -1)
            printf("client read board error");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    play_game(sockfd);
    close(sockfd);
    return 0;
}
