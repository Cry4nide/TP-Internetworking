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
#include "protocolo.h"


void imprime_simbolo() {
    printf("\r%s", "> "); //imprime ">" para el que escribe, es para diferenciar de lo que escribo yo de lo que escribe el otro.\r permite mantener el cursor luego de >
    fflush(stdout);     //vacia el buffer del teclado, a veces no cargaba > o lo carga varias veces
}
// ENVIAR Y RECIBIR MENSAJe

void recv_mensaje(int cliente) {
    char recibe_mensaje[LENGTH_MSG] = {}; //no admite cadenas C por eso el texto se almacena en arrays de tipo char
    while(1) {
        int recibe = recv(cliente, recibe_mensaje, LENGTH_MSG, 0);
        if (recibe > 0) {
           printf("\r%s\n", recibe_mensaje);
           imprime_simbolo();          
        } else if  (recibe == 0) {
            break;          //corta el bucle
        } else {

            //es menor a 0, no hago nada
        }
    } 
}

void send_mensaje(int cliente) {
    char mensaje[LENGTH_MSG] = {};
    while (1) {
        imprime_simbolo();
        while (fgets(mensaje, LENGTH_MSG, stdin) != NULL) { // DE ESTA FORMA SE LIMITA EL NUMERO DE CARACTERES QUE PUEDE LEERSE, stdin indica que se lee desde el teclado
            mensaje[strlen(mensaje)-1] = '\0'; 
            if (strlen(mensaje) == 0) {
                imprime_simbolo();                         
            } else {
                break;
            }
        }
        send(cliente, mensaje, LENGTH_MSG, 0);
        
    
    }


}
