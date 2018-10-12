/**
*************************************************************
* @file servtcp.c
* @brief Breve descripción
* Ejemplo de un cliente TCP usando threads
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version 0.1Beta
* @date 13 ene 2011
* Historial de cambios:
*   20110113 - Versión inicial
*
*
*************************************************************/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

/** Puerto  */
#define PORT       7000

/** Número máximo de hijos */
#define MAX_CHILDS 3

/** Longitud del buffer  */
#define BUFFERSIZE 512


//Cabeceras de funciones

int AtiendeCliente(int socket, struct sockaddr_in addr); //se va a hacer un fork cada vez que llegue un cliente
int DemasiadosClientes(int socket, struct sockaddr_in addr); //si estan completos devuelve a este procedimiento
void error(int code, char *err); //manejo de errores
void reloj(int loop); //

int main(int argv, char** argc){

    int socket_host;
    struct sockaddr_in client_addr;
    struct sockaddr_in my_addr;
    struct timeval tv;    /* Para el timeout del accept */
    socklen_t size_addr = 0;
    int socket_client;
    fd_set rfds;        /* Conjunto de descriptores a vigilar */
    int childcount=0;
    int exitcode;

    int childpid;
    int pidstatus;

    int activated=1;
    int loop=0;
    socket_host = socket(AF_INET, SOCK_STREAM, 0); //AF_INET es el dominio, SOCK_STREAM indica que va a ser un socket TCP/IP
    if(socket_host == -1)
      error(1, "No puedo inicializar el socket");
   
    my_addr.sin_family = AF_INET ; //dominio
    my_addr.sin_port = htons(PORT); //puerto
    my_addr.sin_addr.s_addr = INADDR_ANY ; // INADDR_ANY Enlaza el socket a todas las interfaces
    //my_addr es un "objeto" de datos del socket
   
    if(bind( socket_host, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ) //si me da -1 me da false, significa que no se enlazo
      error(2, "El puerto está en uso"); /* Error al hacer el bind() */

    if(listen(socket_host, 10) == -1 ) //pongo a escuchar el socket. el 10 es el maximo de conexiones que pueden haber en cola
      error(3, "No puedo escuchar en el puerto especificado");

    size_addr = sizeof(struct sockaddr_in); //defino el tamaño de la estructura de los datos de conexion
    /* A modo de documentacion, esta es la estructura de esa variable, aca necesitamos conocer el tamaño
    struct sockaddr_in 
    { 
      short int sin_family;         Familia de la Dirección               
      unsigned short int sin_port;  Puerto                               
      struct in_addr sin_addr;      Dirección de Internet                
      unsigned char sin_zero[8];    Del mismo tamaño que struct sockaddr
    }; */


    while(activated) //aca inicia el bucle del servidor
      {
    reloj(loop); //llama a reloj
    /* select() se carga el valor de rfds */
    FD_ZERO(&rfds); //inicializa en 0 el puntero
    FD_SET(socket_host, &rfds); //Añade un descriptor del socket

    /* select() se carga el valor de tv */
    tv.tv_sec = 0;
    tv.tv_usec = 500000;    /* Tiempo de espera */
   
    if (select(socket_host+1, &rfds, NULL, NULL, &tv)) //empieza a monitorear un socket. Investigar un poco mas el select() y el fd_set
      {
        if((socket_client = accept( socket_host, (struct sockaddr*)&client_addr, &size_addr))!= -1) //llega una conexion entrante
          {
        loop=-1;        /* Para reiniciar el mensaje de Esperando conexión... */
        printf("\nSe ha conectado %s por su puerto %d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port); //inet_ntoa traduce a valores entendibles el puerto y la direccion de ip 
        switch ( childpid=fork() ) //crea el proceso hijo, y depende del valor devuelto verifica si hubo error
          {
          case -1:  /* Error */
            error(4, "No se puede crear el proceso hijo");
            break;
          case 0:   /* Somos proceso hijo */
            if (childcount<MAX_CHILDS)
              exitcode=AtiendeCliente(socket_client, client_addr); //si tengo espacio atiendo al cliente
            else
              exitcode=DemasiadosClientes(socket_client, client_addr); //no atiendo al cliente

            exit(exitcode); /* Código de salida */
          default:  /* Somos proceso padre */
            childcount++; /* Acabamos de tener un hijo */
            close(socket_client); /* Nuestro hijo se las apaña con el cliente que
                         entró, para nosotros ya no existe. */
            break;
          }
          }
        else
          fprintf(stderr, "ERROR AL ACEPTAR LA CONEXIÓN\n");
      }
    /* Miramos si se ha cerrado algún hijo últimamente */
    childpid=waitpid(0, &pidstatus, WNOHANG);
    if (childpid>0)
      {
        childcount--;   /* Se acaba de morir un hijo */

        /* Muchas veces nos dará 0 si no se ha muerto ningún hijo, o -1 si no tenemos hijos
         con errno=10 (No child process). Así nos quitamos esos mensajes*/

        if (WIFEXITED(pidstatus))
          {

        /* Tal vez querremos mirar algo cuando se ha cerrado un hijo correctamente */
        if (WEXITSTATUS(pidstatus)==99)
          {
            printf("\nSe ha pedido el cierre del programa\n");
            activated=0;
          }
          }
      }
    loop++;
    }

    close(socket_host);

    return 0;
}

    /* No usamos addr, pero lo dejamos para el futuro */
int DemasiadosClientes(int socket, struct sockaddr_in addr)
{
    char buffer[BUFFERSIZE];
    int bytecount;

    memset(buffer, 0, BUFFERSIZE);
   
    sprintf(buffer, "Demasiados clientes conectados. Por favor, espere unos minutos\n");

    if((bytecount = send(socket, buffer, strlen(buffer), 0))== -1) //Contesto al cliente
      error(6, "No puedo enviar información");
   
    close(socket);

    return 0;
}

int AtiendeCliente(int socket, struct sockaddr_in addr)
{

    char buffer[BUFFERSIZE];
    char aux[BUFFERSIZE];
    int bytecount;
    int fin=0;
    int code=0;         /* Código de salida por defecto */
    time_t t;
    struct tm *tmp;

    while (!fin)
      {

    memset(buffer, 0, BUFFERSIZE);
    if((bytecount = recv(socket, buffer, BUFFERSIZE, 0))== -1)
      error(5, "No puedo recibir información");

    /* Evaluamos los comandos */
    /* El sistema de gestión de comandos es muy rudimentario, pero nos vale */
    /* Comando TIME - Da la hora */
    if (strncmp(buffer, "TIME", 4)==0) //compara strings
      {
        memset(buffer, 0, BUFFERSIZE); //setea un espacio de memoria para almacenar el mensaje

        t = time(NULL); //iguala el tiempo a la variable 
        tmp = localtime(&t); // asigna una estructura al la variable t

        strftime(buffer, BUFFERSIZE, "Son las %H:%M:%S\n", tmp);//formateo el tiempo de acuerdo con la estructura especificada, y lo copio en el buffer
      }
    /* Comando DATE - Da la fecha */
    else if (strncmp(buffer, "DATE", 4)==0)
      {
        memset(buffer, 0, BUFFERSIZE);

        t = time(NULL);
        tmp = localtime(&t);

        strftime(buffer, BUFFERSIZE, "Hoy es %d/%m/%Y\n", tmp);
      }
    /* Comando HOLA - Saluda y dice la IP */
    else if (strncmp(buffer, "HOLA", 4)==0)
      {
        memset(buffer, 0, BUFFERSIZE);
        sprintf(buffer, "Hola %s, ¿cómo estás?\n", inet_ntoa(addr.sin_addr)); //copio el mensaje en el buffer
      }
    /* Comando EXIT - Cierra la conexión actual */
    else if (strncmp(buffer, "EXIT", 4)==0)
      {
        memset(buffer, 0, BUFFERSIZE);
        sprintf(buffer, "Hasta luego. Vuelve pronto %s\n", inet_ntoa(addr.sin_addr));
        fin=1;
      }
    /* Comando CERRAR - Cierra el servidor */
    else if (strncmp(buffer, "CERRAR", 6)==0)
      {
        memset(buffer, 0, BUFFERSIZE);
        sprintf(buffer, "Adiós. Cierro el servidor\n");
        fin=1;
        code=99;        /* Salir del programa */
      }
    else //si no escribi ningun comando en particular, viene a esta parte
      {    
        sprintf(aux, "ECHO: %s\n", buffer);//vuelvo a copiar los datos que trajo en el buffer, nuevamente
        strcpy(buffer, aux);//Notese que en esta opcion no uso memset porque no me interesa formatear la info, solo le concateno un echo
      }

    if((bytecount = send(socket, buffer, strlen(buffer), 0))== -1) //envio el mensaje
      error(6, "No puedo enviar información"); //en caso de error, muestro este mensaje
      }//aca cierra el bucle del cliente

    close(socket); //cierro la conexion
    return code;
}

void reloj(int loop)
{
  if (loop==0) //si loop es 0, es que el servidor acaba de iniciarse
    printf("[SERVIDOR] Esperando conexión  ");

  printf("\033[1D");        /* Introducimos código ANSI para retroceder 2 caracteres */
  switch (loop%4) //cuando el servidor esta en espera, va a oscilar entre estos 3 caracteres, de ahi que cambie el simbolo en la pantalla
    {
    case 0: printf("|"); break;
    case 1: printf("/"); break;
    case 2: printf("-"); break;
    default:            /* No debemos estar aquí */
      break;
    }

  fflush(stdout);       /* Actualizamos la pantalla */
}

void error(int code, char *err)
{
  char *msg=(char*)malloc(strlen(err)+14); //malloc reserva espacio de memoria y devuelve un puntero.
  sprintf(msg, "Error %d: %s\n", code, err); //devuelve el correspondiente mensaje de error
  exit(1);
}