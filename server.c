/*
 *  FTP Server
 *  Compile & link  :   gcc -o server server.c -pthread
 *  Ejecutar         :   ./server
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>


#define SERVER_PORT 8080

void* ManejadorDeConexion(void* socket_client);

int main(int argc, char **argv)
{
	int socket_desc, 
		socket_client, 
		*new_sock, 
		c = sizeof(struct sockaddr_in);

	struct 	sockaddr_in server;
	struct  sockaddr_in	client;

	// Creacion del socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		perror("No se pudo crear el socket");
		return 1;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(SERVER_PORT);

	if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Fallo el bind");
		return 1;
	}

	listen(socket_desc , 3);
	
	while (socket_client = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))
	{
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = socket_client;        
		pthread_create(&sniffer_thread, NULL,  ManejadorDeConexion, (void*) new_sock);
		pthread_join(sniffer_thread, NULL);
	}
	 
	if (socket_client<0)
	{
		perror("Error al aceptar la conexion");
		return 1;
	}

	return 0;
}

void *ManejadorDeConexion(void *socket_client)
{
	int i, size, len, c;
  	int filehandle;
	struct stat obj;
	char buf[100], command[5], filename[20];
	i = 1;
  while(1)
    {
      recv(socket_client, buf, 100, 0);
      sscanf(buf, "%s", command);
      if(!strcmp(command, "ls"))
	{
	  system("ls >temps.txt");
	  i = 0;
	  stat("temps.txt",&obj);
	  size = obj.st_size;
	  send(socket_client, &size, sizeof(int),0);
	  filehandle = open("temps.txt", O_RDONLY);
	  sendfile(socket_client,filehandle,NULL,size);
	}
      else if(!strcmp(command,"get"))
	{
	  sscanf(buf, "%s%s", filename, filename);
	  stat(filename, &obj);
	  filehandle = open(filename, O_RDONLY);
	  size = obj.st_size;
	  if(filehandle == -1)
	      size = 0;
	  send(socket_client, &size, sizeof(int), 0);
	  if(size)
	  sendfile(socket_client, filehandle, NULL, size);
      
	}
      else if(!strcmp(command, "put"))
        {
	  int c = 0, len;
	  char *f;
	  sscanf(buf+strlen(command), "%s", filename);
	  recv(socket_client, &size, sizeof(int), 0);
	  i = 1;
	  while(1)
	    {
	      filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
	      if(filehandle == -1)
		{
		  sprintf(filename + strlen(filename), "%d", i);
		}
	      else
		break;
	    }
	  f = malloc(size);
	  recv(socket_client, f, size, 0);
	  c = write(filehandle, f, size);
	  close(filehandle);
	  send(socket_client, &c, sizeof(int), 0);
        }
      else if(!strcmp(command, "pwd"))
	{
	  system("pwd>temp.txt");
	  i = 0;
          FILE*f = fopen("temp.txt","r");
          while(!feof(f))
            buf[i++] = fgetc(f);
          buf[i-1] = '\0';
	  fclose(f);
          send(socket_client, buf, 100, 0);
	}
      else if(!strcmp(command, "cd"))
        {
          if(chdir(buf+3) == 0)
	    c = 1;
	  else
	    c = 0;
          send(socket_client, &c, sizeof(int), 0);
        }
 
 
      else if(!strcmp(command, "salir"))
	{
	  printf("Saliendo..\n");
	  i = 1;
	  send(socket_client, &i, sizeof(int), 0);
	  exit(0);
	}
    }
  return 0;
}