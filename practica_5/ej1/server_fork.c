/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void socket_read(int newsockfd)
{
    // el buffer es para poder enviar/recibir
    char buffer[256];
    int n;
    // inicializo el buffer en 0
    bzero(buffer, 256);
    // leo del nuevo socket
    n = read(newsockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("Here is the message: %s\n", buffer);
    n = write(newsockfd, "I got your message", 18);
    if (n < 0)
        error("ERROR writing to socket");
    close(newsockfd);
}

int main(int argc, char *argv[])
{
    // filedescrtiptors de los sockets y el puero
    int sockfd, newsockfd, portno;
    // tamaño del addres del cliente, lo usamos despues
    socklen_t clilen;
    int val = 1;
    // direcciones de los sockets?
    struct sockaddr_in serv_addr, cli_addr;
    //  para el fork()
    int child_pid;
    // me fijo que haya mandado el numero del puerto
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    // creo el socket AF_INET=ipv4 SOCK_STREAM=TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    // pongo a cero todo lo que hay en el server addres
    bzero((char *)&serv_addr, sizeof(serv_addr));
    // el numero del pueto es el argumento que le paso al programa
    portno = atoi(argv[1]);
    // inicializo el serv_addr
    serv_addr.sin_family = AF_INET;
    // el socket se conecta a las interfaces locales (INADDR_ANY)
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    // asigo el puerto que va a usar el socket
    serv_addr.sin_port = htons(portno);
    // seteo el socket para el reuso del addres, ¿importante para INADDR_ANY?
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    // uno el socket al serv_addr
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    // marco el socket para pindicar que voy a escuchar de ahi
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (1)
    {
        // esto espera a que alguien se conecte al sockfd y devuelve un nuevo
        // file descriptor que es otro socket con el que se creo la conección
        // de esta forma, puedo seguir escuchando por el sockfd
        newsockfd = accept(sockfd,
                           (struct sockaddr *)&cli_addr,
                           &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        child_pid = fork();
        switch (child_pid)
        {
        case 0:
            socket_read(newsockfd);
            break;
        case -1:
            perror("fork:");
            exit(1);
        default:
            close(newsockfd);
            break;
        }
        if (child_pid == 0)
            break;
    }
    close(sockfd);
    return 0;
}
