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
#include <string.h>
#include <ctype.h>

#define N_PLAYERS 2

typedef struct
{
    char board[10];
    int player_turn[2];
    pthread_cond_t player_cond[2];
    pthread_mutex_t game_access;
} Tateti_t;

typedef struct
{
    Tateti_t *pT;
    int player_socket;
    int player_id;
} Player_t;

void Tateti_Init(Tateti_t *pT)
{
    int i;
    for (i = 0; i < 10; i++)
    {
        pT->board[i] = ' ';
    }
    pT->board[i] = '\0';
    pT->player_turn[0] = 0;
    pT->player_turn[1] = 0;
    //inicializo mutex y demas cosas
    if (pthread_mutex_init(&pT->game_access, NULL) == -1)
        perror("game_access mutex init");
    if (pthread_cond_init(&pT->player_cond[0], NULL) == -1)
        perror("player_turn[0] cond init");
    if (pthread_cond_init(&pT->player_cond[1], NULL) == -1)
        perror("player_turn[1] cond init");
}

void Player_Init(Player_t *pP, Tateti_t *pTateti, int p_socket, int id)
{
    pP->pT = pTateti;
    pP->player_socket = p_socket;
    pP->player_id = id;
}

void Game_Start(Tateti_t *pT)
{
    if (pthread_mutex_lock(&pT->game_access) != 0)
        perror("game_start locking game_acces mutex");
    pT->player_turn[0] = 1;
    if (pthread_cond_signal(&pT->player_cond[0]) != 0)
        perror("game start signal player_cond[0] cond");
    if (pthread_mutex_unlock(&pT->game_access) != 0)
        perror("game start unlocking game_acces mutex");
}

void client_manage(Player_t *pP)
{
    // esto es para saber que player soy y quien es el rival
    int rival_id, int_read;
    char player_mark, char_read; //char_id = pP->player_id + '0';
    if (pP->player_id == 0)
    {
        rival_id = 1;
        player_mark = 'X';
    }
    else
    {
        rival_id = 0;
        player_mark = 'O';
    }
    // if (write(pP->player_socket, &char_id, 1) == -1)
    //     perror("client write");
    // cargo el juego a una variable para que no sea tedioso usarlo
    Tateti_t *pT = pP->pT;
    while (1)
    {
        // pido acceso al juego
        if (pthread_mutex_lock(&pT->game_access) != 0)
            perror("player mutex lock:");
        // espero a que sea mi turno, el otro jugador deberia mandar una signal cuando termine
        while (pT->player_turn[pP->player_id] == 0)
            pthread_cond_wait(&pT->player_cond[pP->player_id], &pT->game_access);
        /// ZONA SEGURA
        // le mando al cliente el tablero
        if (write(pP->player_socket, pT->board, 10) == -1)
            perror("client write");
        // leo donde quiere el ciente poner su marca
        if (read(pP->player_socket, &char_read, 1) == -1)
            perror("client read");
        // convierto la marca de char a int
        int_read = (int)char_read - '0';
        // pongo la marca en el tablero
        pT->board[int_read] = player_mark;
        // le reenvio el tablero al cliente
        if (write(pP->player_socket, pT->board, 10) == -1)
            perror("client write");
        // ya no es mas mi turno
        pT->player_turn[pP->player_id] = 0;
        // el el turno del otro
        pT->player_turn[rival_id] = 1;
        // le aviso al otro jugador que es su turno
        if (pthread_cond_signal(&pT->player_cond[rival_id]) != 0)
            perror("end of turn signaling");
        if (pthread_mutex_unlock(&pT->game_access) != 0)
            perror("player mutex unlock:");
        // FIN ZONA SEGURA
    }
    char *over = "Game is over";
    if (write(pP->player_socket, over, strlen(over)) == -1)
        perror("client write over");
    close(pP->player_socket);
}

int main(int argc, char *argv[])
{
    printf("Starting game_server\n");
    // filedescrtiptors de los sockets y el puerto
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
        perror("ERROR opening socket");
    // pongo a cero todo lo que hay en el server addres
    bzero((char *)&serv_addr, sizeof(serv_addr));
    // el numero del pueto es el argumento que le paso al programa
    portno = atoi(argv[1]);
    // inicializo el serv_addr
    serv_addr.sin_family = AF_INET;
    // el socket se conecta a todas las interfaces locales (INADDR_ANY)
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    // asigo el puerto que va a usar el socket
    serv_addr.sin_port = htons(portno);
    // seteo el socket para el reuso del addres, ¿importante para INADDR_ANY?
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    // uno el socket al serv_addr
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        perror("ERROR on binding");
    // marco el socket para indicar que voy a escuchar de ahi
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    // creo los attr para los threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    // el id del thread
    pthread_t thr_id[N_PLAYERS];

    // creo la estructura del juego
    Tateti_t *pT = malloc(sizeof(Tateti_t));
    Tateti_Init(pT);

    // voy a conectarme con los dos clientes
    int i = 0;

    while (i < 2)
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
            perror("ERROR on accept");
        // Creo a la estructura player, que va a manejar el thread
        Player_t *pP = malloc(sizeof(Player_t));
        Player_Init(pP, pT, *newsockfd, i);
        // disparo un thread con el player, para atender a ese cliente
        thr_id[i] = (pthread_t)i;
        printf("Accepted player %d\n", i);
        if (pthread_create(&thr_id[i], &attr, (void *)client_manage, (void *)pP) != 0)
        {
            perror("pthread_create:");
            exit(1);
        }
        i++;
    }

    Game_Start(pT);

    for (i = 0; i < N_PLAYERS; i++)
    {
        pthread_join(thr_id[i], NULL);
        printf("Thread %ld joined\n", thr_id[i]);
    }

    close(sockfd);
    return 0;
}
