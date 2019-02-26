/* 
 *  FTP Client
 *  Envia el comando get filename para recuperar el archivo.
 *
 *  Compile & link  :   gcc -o client client.c
 *  Ejecutar       :   ./client
 */

#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_IP 	"127.0.0.1"
#define SERVER_PORT 	8080

int main(int argc , char **argv)
{
	int 	socket_desc;
	struct 	sockaddr_in server;

	// Variables para el archivo que se va a recuperar
	struct stat obj;
	int choice;
	char buf[100], command[5], filename[20], *f;
	int i, size, status;
	int filehandle;
		
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		perror("No se pudo crear el socket");
		return 1;
	}

	server.sin_addr.s_addr = inet_addr(SERVER_IP);
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);

	// Conectar al servidor
	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Fallo la conexion");
		return 1;
	}

	i = 1;
	while(1)
	{
		printf("Ingresar opcion:\n1- get\n2- put\n3- pwd\n4- ls\n5- cd\n6- salir\n");
		scanf("%d", &choice);
		switch(choice)
		{
		case 1:
		printf("Ingrese el archivo a obtener: ");
		scanf("%s", filename);
		strcpy(buf, "get ");
		strcat(buf, filename);
		send(socket_desc, buf, 100, 0);
		recv(socket_desc, &size, sizeof(int), 0);
		if(!size)
			{
			printf("No existe el archivo en el directorio\n\n");
			break;
			}
		f = malloc(size);
		recv(socket_desc, f, size, 0);
		while(1)
			{
			filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
			if(filehandle == -1)
			{
			sprintf(filename + strlen(filename), "%d", i);//se necesita solamente si el cliente y el servidor estan en el mismo directorio
			}
			else break;
			}
		write(filehandle, f, size, 0);
		close(filehandle);
		strcpy(buf, "cat ");
		strcat(buf, filename);
		system(buf);
		break;
		case 2:
		printf("Ingrese el nombre del archivo para subir al servidor: ");
			scanf("%s", filename);
		filehandle = open(filename, O_RDONLY);
			if(filehandle == -1)
				{
				printf("No existe el archivo en el directorio\n\n");
				break;
				}
			strcpy(buf, "put ");
		strcat(buf, filename);
		send(socket_desc, buf, 100, 0);
		stat(filename, &obj);
		size = obj.st_size;
		send(socket_desc, &size, sizeof(int), 0);
		sendfile(socket_desc, filehandle, NULL, size);
		recv(socket_desc, &status, sizeof(int), 0);
		if(status)
			printf("Archivo almacenado exitosamente\n");
		else
			printf("El archivo no se pudo almacenar\n");
		break;
		case 3:
		strcpy(buf, "pwd");
		send(socket_desc, buf, 100, 0);
		recv(socket_desc, buf, 100, 0);
		printf("La ruta del directorio es: %s\n", buf);
		break;
		case 4:
		strcpy(buf, "ls");
			send(socket_desc, buf, 100, 0);
		recv(socket_desc, &size, sizeof(int), 0);
			f = malloc(size);
			recv(socket_desc, f, size, 0);
		filehandle = creat("temp.txt", O_WRONLY);
		write(filehandle, f, size, 0);
		close(filehandle);
			printf("El listado del directorio es el siguiente:\n");
		system("cat temp.txt");
		break;
		case 5:
		strcpy(buf, "cd ");
		printf("Introduzca la ruta para cambiar el directorio: ");
		scanf("%s", buf + 3);
			send(socket_desc, buf, 100, 0);
		recv(socket_desc, &status, sizeof(int), 0);
			if(status)
				printf("Directorio cambiado exitosamente\n");
			else
				printf("No se pudo cambiar el directorio\n");
			break;
		case 6:
		strcpy(buf, "salir");
			send(socket_desc, buf, 100, 0);
			recv(socket_desc, &status, 100, 0);
		if(status)
			{
			printf("Saliendo..\n");
			exit(0);
			}
			printf("No se pudo cerrar la conexion\n");
		}
    }
}