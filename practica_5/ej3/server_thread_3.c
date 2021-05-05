/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#define TIMEOUT_SEC 5

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// -2 en caso de timeout, -1 en caso de error, si no, numero de bytes leidos
int recvtimeout(int sock, char *buf, int len, int timeoutSecs)
{
    // inicializo un set de lectura donde va a estar el socket
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(sock, &read_set);

    // creo la estructura tiempo
    struct timeval tv;
    tv.tv_sec = timeoutSecs;

    int n_fd, n_bytes;
    n_fd = select(sock + 1, &read_set, NULL, NULL, &tv);
    if (n_fd == -1)
    {
        perror("socket select");
        return -1;
    }
    else if (n_fd == 0)
    {
        return -2;
    }
    else if (FD_ISSET(sock, &read_set))
    {
        n_bytes = read(sock, buf, len);
        if (n_bytes == -1)
        {
            perror("socket read:");
            return -1;
        }
        return n_bytes;
    }
    return -2;
}

void socket_read(int *newsockfd)
{
    // el buffer es para poder enviar/recibir
    char buffer[256];
    int n;
    // inicializo el buffer en 0
    bzero(buffer, 256);
    // leo del nuevo socket
    n = recvtimeout(*newsockfd, buffer, 256, TIMEOUT_SEC);
    if (n == -1)
        exit(1);
    else if (n == -2)
    {
        char *msg = "TIMEOUT, SOCKET CLOSED";
        n = write(*newsockfd, msg, strlen(msg));
        printf("Closing socket %d, TIMEOUT", *newsockfd);
        if (n < 0)
            error("timeout warn client");
        close(*newsockfd);
    }
    else
    {
        printf("Here is the message: %s\n", buffer);
        int i;
        for (i = 0; i < n; i++)
        {
            buffer[i] = toupper(buffer[i]);
        }
        n = write(*newsockfd, buffer, n);
        if (n < 0)
            error("ERROR writing to socket");
        close(*newsockfd);
    }
}

int main(int argc, char *argv[])
{
    // filedescrtiptors de los sockets y el puero
    int sockfd, portno;
    // tamaño del addres del cliente, lo usamos despues
    socklen_t clilen;
    int val = 1;
    // direcciones de los sockets?
    struct sockaddr_in serv_addr, cli_addr;
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
    // marco el socket para indicar que voy a escuchar de ahi
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // creo los attr para los threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    // el id del thread
    pthread_t thr_id = 0;

    while (1)
    {
        // tengo que crear un nuevo newsockfd por cada thread porque si lo cierran
        // cagan a los demas
        int *newsockfd = malloc(sizeof(int));
        // esto espera a que alguien se conecte al sockfd y devuelve un nuevo
        // file descriptor que es otro socket con el que se creo la conección
        // de esta forma, puedo seguir escuchando por el sockfd
        *newsockfd = accept(sockfd,
                            (struct sockaddr *)&cli_addr,
                            &clilen);
        if (*newsockfd < 0)
            error("ERROR on accept");

        if (pthread_create(&thr_id, &attr, (void *)socket_read, (void *)newsockfd) != 0)
        {
            perror("pthread_create:");
            exit(1);
        }
        thr_id++;
    }
    close(sockfd);
    return 0;
}
