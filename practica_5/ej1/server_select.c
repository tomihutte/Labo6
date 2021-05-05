/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_CLIENT_LIST 1

// uso una struct para guardar los sockets, se puede hacer con un array
typedef struct
{
    int sockets[MAX_CLIENT_LIST];
} Sockets_to_Clients_t;

// me devuelve el indice de un lugar libre, si es que hay, si estan todos ocupados devuelve 0
int Socket_Free(Sockets_to_Clients_t *pS)
{
    int index = 0;
    while (pS->sockets[index] != -1)
    {
        index++;
        if (index == MAX_CLIENT_LIST)
            return -1;
    }
    return index;
}

// me agrega un socket al index que le paso
void Socket_Add(Sockets_to_Clients_t *pS, int socket_fd, int index)
{
    pS->sockets[index] = socket_fd;
}

// remueve el socket del array
void Socket_Remove(Sockets_to_Clients_t *pS, int socket_fd)
{
    int index = 0;
    while (pS->sockets[index] != socket_fd)
    {
        index++;
        if (index == MAX_CLIENT_LIST)
            return;
    }
    pS->sockets[index] = -1;
}

// inicializa los sockets en -1
void Sockets_Init(Sockets_to_Clients_t *pS)
{
    int i;
    for (i = 0; i < MAX_CLIENT_LIST; i++)
    {
        pS->sockets[i] = -1;
    }
}

// esto me devuelve el fd mas grande entre la lista de sockets y el original
int Max_Fd(Sockets_to_Clients_t *pS, int socket_fd)
{
    int max = socket_fd;
    int i;
    for (i = 0; i < MAX_CLIENT_LIST; i++)
    {
        if (pS->sockets[i] > max)
            max = pS->sockets[i];
    }
    return max;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
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

    // el buffer es para poder enviar/recibir, n es el tamaño que leo
    char buffer[256];
    int n;

    //creo los sets para el select y la variable que retorna el select
    fd_set read_set;
    // variables para manejar
    int n_fd, max_fd, i, index;
    // incializo la lista de sockets para los clientes
    Sockets_to_Clients_t *pSC = malloc(sizeof(Sockets_to_Clients_t));
    Sockets_Init(pSC);

    while (1)
    {
        // inicializo en cero el set de fd
        FD_ZERO(&read_set);
        // agrego el socket para nuevos clientes solo si hay lugar para nuevos clientes
        // si no hay lugar, no lo voy a monitorear
        if ((index = Socket_Free(pSC)) >= 0)
            FD_SET(sockfd, &read_set);
        // agrego los sockets de clientes ya vinculados
        for (i = 0; i < MAX_CLIENT_LIST + 1; i++)
        {
            // me fijo de no agregar el -1
            if (pSC->sockets[i] != -1)
                FD_SET(pSC->sockets[i], &read_set);
        }
        // obtengo el valor de fd mas alto de todo el set
        max_fd = Max_Fd(pSC, sockfd);
        // hago el select sobre todos los sockets ya creados y el que escucha a nuevos clientes
        n_fd = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if (n_fd == -1)
        {
            perror("select: ");
            exit(1);
        }
        // me fijo si hay un nuevo cliente que quiere ser atendido
        else if (FD_ISSET(sockfd, &read_set))
        {
            // aca reviso de nuevo que el index sea >= 0, por las dudas
            if (index >= 0)
            {
                // esto espera a que alguien se conecte al sockfd y devuelve un nuevo
                // file descriptor que es otro socket con el que se creo la conección
                // de esta forma, puedo seguir escuchando por el sockfd
                newsockfd = accept(sockfd,
                                   (struct sockaddr *)&cli_addr,
                                   &clilen);
                if (newsockfd < 0)
                    error("ERROR on accept");
                else
                    // agregoe el nuevo socket a la lista para monitorear
                    Socket_Add(pSC, newsockfd, index);
            }
        }
        else
        {
            for (i = 0; i < MAX_CLIENT_LIST; i++)
            {
                //  aca reviso todos los sockets con comunicación ya establecida
                newsockfd = pSC->sockets[i];
                if (FD_ISSET(newsockfd, &read_set))
                {
                    // si estan listos, me preparo para leerlos

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
                    Socket_Remove(pSC, newsockfd);
                    close(newsockfd);
                }
            }
        }
    }
    close(sockfd);
    return 0;
}
