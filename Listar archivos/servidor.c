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
#include <sys/stat.h>
#include <dirent.h>

typedef struct Nodo
{
	int dato_sd;
	struct Nodo *ant;
	struct Nodo *ult;
	char ip[16];
	char nombre[32];
} ListaCliente;

ListaCliente *newNode(int sd, char *ip)
{
	ListaCliente *nuevo = (ListaCliente *)malloc(sizeof(ListaCliente)); //reservo memoria en forma dinamica (es el tamaño del nodo)
	nuevo->dato_sd = sd;
	nuevo->ant = NULL;
	nuevo->ult = NULL;
	strncpy(nuevo->ip, ip, 16); //como son dos punteros se usa esta funcion para asignar el contenido de un puntero a otro
	strncpy(nuevo->nombre, "NULL", 5);
	return nuevo;
}

/* Función para devolver un error en caso de que ocurra */
void error(const char *s);

/* Calculamos el tamaño del archivo */
long fileSize(char *fname);

/* Sacamos el tipo de archivo haciendo un stat(), es como el stat de la línea de comandos */
unsigned char statFileType(char *fname);

/* Función que hace algo con un archivo, pero le pasamos el dirent completo, usaremos más datos */
void procesoArchivo(char *ruta, struct dirent *ent, ListaCliente *np);

void listar_archivos(ListaCliente *np);

// Estructura que contiene los ususarios que se van conectando

/////////////////////////////

// variables globales
ListaCliente *root;
ListaCliente *actual;
int cliente = 0;
int servidor = 0;

//enviar mensaje a todos los clientes

void enviar_a_todos(ListaCliente *np, char tmp_buffer[])
{
	ListaCliente *tmp = root->ult;
	while (tmp != NULL)
	{
		//if (np->dato_sd != tmp->dato_sd)
		if (np->dato_sd != tmp->dato_sd)
		{ //se lo envio a todos menos a él mismo
			printf("%s", tmp_buffer);
			send(tmp->dato_sd, tmp_buffer, LENGTH_MSG, 0);
		}
		tmp = tmp->ult;
	}
}
void enviar_a_cliente(ListaCliente *np, char tmp_buffer[])
{
	ListaCliente *tmp = root->ult;
	while (tmp != NULL)
	{
		//if (np->dato_sd != tmp->dato_sd)
		if (np->dato_sd == tmp->dato_sd)
		{ //se lo envio a todos menos a él mismo
			sprintf(tmp_buffer, "ECHO: %s", tmp_buffer);
			send(tmp->dato_sd, tmp_buffer, LENGTH_MSG, 0);
		}
		tmp = tmp->ult;
	}
}

//inicio de charla
void inicio_charla(void *pcliente)
{
	int flag_command = 0;
	int flag = 0;
	char nickname[LENGTH_NAME] = {};
	char recv_buffer[LENGTH_MSG] = {};
	char send_buffer[LENGTH_MSG] = {};
	ListaCliente *np = (ListaCliente *)pcliente;

	// nombre (para probar desde el sevidor las distintas conexiones)
	if (recv(np->dato_sd, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME - 1)
	{
		printf("NO ingreso un nombre, o ingreso un nombre de mas de 30 caracteres\n");
		flag = 1;
	}
	else
	{
		//esto se cambiaria por x usuario se ha conectado
		strncpy(np->nombre, nickname, LENGTH_NAME);
		printf("<< %s(%s) ingreso al chat >>\n", np->nombre, np->ip);
	}

	//conversacion
	while (1)
	{
		flag_command = 0;
		if (flag)
		{
			break;
		}
		// habria que cheackear que el comando estuviera correcto, y si pudo hacer la operacion, que le mande a todos lo que hizo
		// si el comando es incorrecto hay que responderle al cliente que el comando es incorrecto.
		int recibe = recv(np->dato_sd, recv_buffer, LENGTH_MSG, 0);
		//si hay datos en el buffer, los manda.
		if (recibe > 0)
		{
			if (strlen(recv_buffer) == 0)
			{
				continue;
			}
			if (strncmp(recv_buffer, "PRUEBA", 6) == 0)
			{
				sprintf(send_buffer, "%s ingreso el comando %s", np->nombre, recv_buffer);
				flag_command = 1;
			}
			if (strncmp(recv_buffer, "LISTAR", 6) == 0)
			{
				listar_archivos(np);
				flag_command = 1;
			}
			//transforma los datos recibidos y los prepara para enviarlos
		}
		else if (recibe == 0 || strcmp(recv_buffer, "salir") == 0)
		{ //modificar el salir (posiblemente se salga con control+c)
			printf("<< %s(%s) SALIO del chat >>\n", np->nombre, np->ip);
			sprintf(send_buffer, ">>> %s  SALIO del chat <<<", np->nombre);
			flag = 1;
		}
		else
		{
			//menor a 0
			printf("ERROR");
			flag = 1;
		}
		if (flag_command == 1)
		{
			enviar_a_todos(np, send_buffer);
			enviar_a_cliente(np, send_buffer);
		}
	}
	//eliminar nodo (del usuario que salio del chat)
	close(np->dato_sd);
	if (np == actual)
	{ //elimino del principio
		actual = np->ant;
		actual->ult = NULL;
	}
	else
	{ //elimino del medio
		np->ant->ult = np->ult;
		np->ult->ant = np->ant;
	}
	free(np);
}

//PRINCIPAL
int main()
{

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
	if (bind(servidor, (void *)&direccionServidor, sizeof(direccionServidor)) != 0)
	{
		perror("Error en bind");
		return 1;
	}

	printf("Estoy escuchando\n");
	listen(servidor, 5);

	//lista inicial para ir enlazando los clientes
	root = newNode(servidor, inet_ntoa(direccionServidor.sin_addr));
	actual = root;

	int lon;
	struct sockaddr_in direccionCliente;
	//Aceptar conexiones          //
	while (1)
	{
		lon = sizeof(direccionCliente);
		cliente = accept(servidor, (struct sockaddr *)&direccionCliente, &lon);
		printf("recibi una conexion en el socket %d\n", cliente); //lo usamos para in probando en el servidor como se iban conectando los clientes

		//Aca iria la validacion del usuario y la contraseña, contra una lista pre cargada en el sistema

		//------------------------------------------------------------------------------------------

		//insertar los clientes en la lista
		ListaCliente *c = newNode(cliente, inet_ntoa(direccionCliente.sin_addr)); //es la direccion del cliente
		c->ant = actual;
		actual->ult = c;
		actual = c;

		//se crea un hijo se envia el cliente y se comienza la charla (cada vez que se conecta un cliente se genera un hilo pa él)
		pthread_t id;
		if (pthread_create(&id, NULL, (void *)inicio_charla, (void *)c) != 0)
		{
			perror("Error en pthread\n");
			exit(-1);
		}
	}

	return 0;
}

void listar_archivos(ListaCliente *np)
{
	/* Con un puntero a DIR abriremos el directorio */
	DIR *dir;
	/* en *ent habrá información sobre el archivo que se está "sacando" a cada momento */
	struct dirent *ent;
	/* Empezaremos a leer en el directorio actual */
	dir = opendir(".");

	/* Miramos que no haya error */
	if (dir == NULL)
		error("No puedo abrir el directorio");

	/* Una vez nos aseguramos de que no hay error, ¡vamos a jugar! */
	/* Leyendo uno a uno todos los archivos que hay */
	while ((ent = readdir(dir)) != NULL)
	{
		/* Nos devolverá el directorio actual (.) y el anterior (..), como hace ls */
		if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
		{
			/* Una vez tenemos el archivo, lo pasamos a una función para procesarlo. */
			procesoArchivo(".", ent, np);
		}
	}
	closedir(dir);

	return EXIT_SUCCESS;
}

void error(const char *s)
{
	/* perror() devuelve la cadena S y el error (en cadena de caracteres) que tenga errno */
	perror(s);
	exit(EXIT_FAILURE);
}

long fileSize(char *fname)
{
	FILE *fich;
	long ftam = -1;

	fich = fopen(fname, "r");
	if (fich)
	{
		fseek(fich, 0L, SEEK_END);
		ftam = ftell(fich);
		fclose(fich);
	}
	else
		printf("ERRNO: %d - %s\n", errno, strerror(errno));
	return ftam;
}

void procesoArchivo(char *ruta, struct dirent *ent, ListaCliente *np)
{
	long ftam;
	char *nombrecompleto;
	char strtam[20];
	char strtipo[30] = "";
	char buffer[LENGTH_MSG] = {};
	/* Tiene que ser del mismo tipo de dirent.d_type en nuestro sistema */
	static unsigned char tipoID[7] = {DT_BLK, DT_CHR, DT_DIR, DT_FIFO, DT_LNK, DT_REG, DT_SOCK};
	static char *tipoSTRs[7] = {"Dispositivo de bloques", "Dispositivo de caracteres", "Directorio", "FIFO", "Enlace", "Archivo regular", "Socket Unix"};

	int i;
	int tmp;
	unsigned char tipo;

	/* Sacamos el nombre completo con la ruta del archivo */
	tmp = strlen(ruta);
	nombrecompleto = malloc(tmp + strlen(ent->d_name) + 2); /* Sumamos 2, por el \0 y la barra de directorios (/) no sabemos si falta */
	if (ruta[tmp - 1] == '/')
		sprintf(nombrecompleto, "%s%s", ruta, ent->d_name);
	else
		sprintf(nombrecompleto, "%s/%s", ruta, ent->d_name);

	/* Calcula el tamaño */
	ftam = fileSize(nombrecompleto);
	if (ftam >= 0)
		sprintf(strtam, "%ld bytes", ftam);
	else
		strcpy(strtam, "No info");

	/* A veces ent->d_type no nos dice nada, eso depende del sistema de archivos que estemos */
	/* mirando, por ejemplo ext*, brtfs, sí nos dan esta información. Por el contrario, nfs */
	/* no nos la da (directamente, una vez que hacemos stat sí lo hace), y es en estos casos donde probamos con stat() */
	tipo = ent->d_type;
	if (tipo == DT_UNKNOWN)
		tipo = statFileType(nombrecompleto);

	if (tipo != DT_UNKNOWN)
	{
		/* Podíamos haber hecho un switch con los tipos y devolver la cadena,
         pero me da la impresión de que así es menos costoso de escribir. */
		i = 0;
		while ((i < 7) && (tipo != tipoID[i]))
			++i;

		if (i < 7)
			strcpy(strtipo, tipoSTRs[i]);
	}

	/* Si no hemos encontrado el tipo, éste será desconocido */
	if (strtipo[0] == '\0')
		strcpy(strtipo, "Tipo desconocido");

	sprintf(buffer, "%30s (%s)\t%s \n", ent->d_name, strtam, strtipo);
	enviar_a_cliente(np, buffer);

	free(nombrecompleto);
}

/* stat() vale para mucho más, pero sólo queremos el tipo ahora */
unsigned char statFileType(char *fname)
{
	struct stat sdata;

	/* Intentamos el stat() si no funciona, devolvemos tipo desconocido */
	if (stat(fname, &sdata) == -1)
	{
		return DT_UNKNOWN;
	}

	switch (sdata.st_mode & S_IFMT)
	{
	case S_IFBLK:
		return DT_BLK;
	case S_IFCHR:
		return DT_CHR;
	case S_IFDIR:
		return DT_DIR;
	case S_IFIFO:
		return DT_FIFO;
	case S_IFLNK:
		return DT_LNK;
	case S_IFREG:
		return DT_REG;
	case S_IFSOCK:
		return DT_SOCK;
	default:
		return DT_UNKNOWN;
	}
}