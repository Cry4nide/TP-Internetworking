#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "protocolo.h"


// Estructura que contiene los ususarios que se van conectando
typedef struct Nodo {
    int dato_sd;
    struct Nodo* ant;
    struct Nodo* ult;
    char ip[16];
    char nombre[32];
    
} ListaCliente;

ListaCliente *newNode(int sd, char* ip) {
    ListaCliente *nuevo = (ListaCliente *)malloc(sizeof(ListaCliente)); //reservo memoria en forma dinamica (es el tamaño del nodo)
    nuevo->dato_sd = sd;
    nuevo->ant = NULL;
    nuevo->ult = NULL;
    strncpy (nuevo->ip, ip, 16) ; //como son dos punteros se usa esta funcion para asignar el contenido de un puntero a otro
    strncpy (nuevo->nombre, "NULL", 5);
    return nuevo;
}
/////////////////////////////

// variables globales
ListaCliente *root;
ListaCliente *actual;
int cliente = 0;
int servidor = 0;



//enviar mensaje a todos los clientes

void enviar_a_todos (ListaCliente *np, char tmp_buffer[]) {
	ListaCliente *tmp = root->ult;
	while (tmp != NULL) {
		if (np->dato_sd != tmp->dato_sd) {			//se lo envio a todos menos a él mismo
			printf ("Enviado de %d:  \"%s\" \n", tmp->dato_sd, tmp_buffer);
			send(tmp->dato_sd, tmp_buffer, LENGTH_MSG, 0);
		}
		tmp = tmp->ult;
	}
}


//inicio de charla
void inicio_charla(void *pcliente) { 
	int flag = 0;
	char nickname[LENGTH_NAME] = {};
	char recv_buffer[LENGTH_MSG] = {};
	char send_buffer[LENGTH_MSG] = {};
	ListaCliente *np = (ListaCliente *)pcliente;

	// nombre (para probar desde el sevidor las distintas conexiones)
	if (recv(np->dato_sd, nickname, LENGTH_NAME,0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME -1) {
		printf("NO ingreso un nombre, o ingreso un nombre de mas de 30 caracteres\n");
		flag = 1;
	} else {	
		strncpy(np->nombre, nickname, LENGTH_NAME);
		printf("<< %s(%s) ingreso al chat >>\n", np->nombre, np->ip);
		sprintf(send_buffer, "*** %s ingreso al chat***\n", np->nombre); //imprime lo asignado en send_buffer, y se envia esa variable 
		enviar_a_todos(np, send_buffer);								//es para mandar a los usuarios la persona q ingreso al chat	
	}

	//conversacion
	while (1) {
		if (flag) {
			break;
		}
		
		int recibe = recv(np->dato_sd, recv_buffer, LENGTH_MSG, 0);
		if (recibe > 0){
			if(strlen(recv_buffer) == 0) {
				continue;
			}
			sprintf(send_buffer, "%s: %s", np->nombre, recv_buffer);
		} else if (recibe == 0 || strcmp(recv_buffer,"salir") == 0) { //modificar el salir (posiblemente se salga con control+c)
			printf("<< %s(%s) SALIO del chat >>\n", np->nombre, np->ip);
			sprintf(send_buffer, ">>> %s  SALIO del chat <<<", np->nombre);
			flag = 1;
		} else {
			//menor a 0
			printf("ERROR");
			flag = 1;

		}
		enviar_a_todos(np, send_buffer);		

	}
	//eliminar nodo (del usuario que salio del chat)
	close(np->dato_sd);
	if (np == actual) {			//elimino del principio
		actual = np->ant;
		actual->ult = NULL;
	} else {  				//elimino del medio
		np->ant->ult=np->ult;
		np->ult->ant= np->ant;
	}
	free(np);
}

//PRINCIPAL
int main() {

	
	struct sockaddr_in direccionServidor;

	direccionServidor.sin_family = PF_INET;  			
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(5000);

	//crear socket: AF_INET implica que estan  conectados mediante un rea tcp/ip y SOCK_STREAM es el tipo de socket (tcp)
	servidor = socket(AF_INET, SOCK_STREAM, 0);  

	//Libera el puerto una vez que se cancelo o aborto la conexion (sino hay que esperar que lo libere el SO luego de un tiempo)
	int activado = 1;
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)); 
	
	//  Asociar el socket con la direccion del servidor (bind)
	if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
		perror("Error en bind");
		return 1;
	}

	printf("Estoy escuchando\n");
	listen(servidor,5);

    //lista inicial para ir enlazando los clientes
	root = newNode(servidor,inet_ntoa(direccionServidor.sin_addr));
	actual = root;

	int lon;
	struct sockaddr_in direccionCliente;
	//Aceptar conexiones          //
	while(1) {
		lon = sizeof(direccionCliente);
		cliente = accept(servidor, (struct sockaddr *) &direccionCliente, &lon);
		printf("recibi una conexion en el socket %d\n", cliente);                 //lo usamos para in probando en el servidor como se iban conectando los clientes
		
		//insertar los clientes en la lista
		ListaCliente *c = newNode(cliente, inet_ntoa(direccionCliente.sin_addr)); //es la direccion del cliente
		c->ant = actual;
		actual->ult = c;
		actual = c;

		//se crea un hijo se envia el cliente y se comienza la charla (cada vez que se conecta un cliente se genera un hilo pa él)
		pthread_t id;
		if (pthread_create(&id, NULL, (void *)inicio_charla, (void *)c) != 0) {
			perror("Error en pthread\n");
			exit(-1); 

		}

	}

	return 0;

}




















