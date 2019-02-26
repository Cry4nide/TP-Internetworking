#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include "protocolo.h"

//variables globales
int flag = 0;
char nickname[LENGTH_NAME] = {};
int cliente = 0;

void salir()
{
    printf("\n*** Saliste del chat. CHAU ***\n");
    exit(-1);
}

//PRINCIPAL
int main(int argc, char *argv[])
{

    struct hostent *h;

    if (argc < 2)
    {
        printf("Debe ejecutar el cliente con el nombre de host: %s\n", argv[0]);
        exit(-1);
    }

    signal(SIGINT, salir); // cuando el usuario preciona ctrl+c (dado por la señal SIGINT), se llama a la funcion salir, la cual termina el programa (es para salir del chat)

    //Ingreso de nickname
    printf("************************************************************ \n");
    printf("*******              BIENVENIDO AL CHAT              ******* \n");
    printf("************************************************************ \n");
    printf(" \n");
    printf("Para salir presione CTRL+C \n");
    printf(" \n");
    printf("************************************************************ \n");
    int flag_nombre = 1;
    while (flag_nombre)
    {
        printf("Por favor ingrese un nombre: ");
        if (fgets(nickname, LENGTH_NAME, stdin) != NULL)
        { //almacena la cadena introducida por teclado
            nickname[strlen(nickname) - 1] = '\0';

            flag_nombre = 0;
        }
        if (strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME - 1)
        {
            printf("El nombre ingresado debe tener entre 2 y 30 caracteres\n");
            flag_nombre = 1;
        }
    }
    //crea el socket
    struct sockaddr_in direccionCliente;
    direccionCliente.sin_family = PF_INET;
    direccionCliente.sin_port = htons(5000);
    if (h = gethostbyname(argv[1]))
    {
        memcpy(&direccionCliente.sin_addr, h->h_addr, h->h_length);
    }
    else
    {
        perror("error");
        exit(-1);
    }

    cliente = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(cliente, (void *)&direccionCliente, sizeof(direccionCliente)) != 0)
    {
        perror("No se pudo conectar");
        return 1;
    }

    send(cliente, nickname, LENGTH_NAME, 0);

    //hilo para enviar
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_mensaje, cliente) != 0)
    {                                 // para llamar a la funcion send_mensaje el parametro no se puede
        printf("Error en pthread\n"); //para como send_mensaje(cliente), como si fuera un llamado comun,
        exit(-1);                     //por q la funcion phtread_create tiene su propia forma de enviar los parametros, y se hace en el ultimo parametro de ptread
    }
    //otro hilo para recibir
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_mensaje, cliente) != 0)
    { //lo mismo aca cuando se llama a recv_mensaje
        printf("Error en pthread!\n");
        exit(-1);
    }

    while (1)
    {
        if (flag)
        {
            break;
        }
    }

    close(cliente);
    return 0;
}
