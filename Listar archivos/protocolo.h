#ifndef PROTO
#define PROTO

#define LENGTH_NAME 31 // en realidad es 30 y 200 pero C agrega un lugar mas para \0 (que indica el fin de cadena)
#define LENGTH_MSG 2000

#endif // PROTO

void recv_mensaje(int cliente);
void send_mensaje(int cliente);
