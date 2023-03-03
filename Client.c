// includes
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>



#define SERVER_PORT 80      // Aquí trobem el port que farem servir
#define MAXLINE 4096        // Mida màxima del buffer
#define SA struct sockaddr  // predefinim una comanda per a estalviar linees de codi

int main(int argc, char **argv)
{
    // declaració de variables
    int sock, n;
    int sendbytes;
    struct sockaddr_in servaddr;
    char sendLine[MAXLINE];
    char recvLine[MAXLINE];
    
    // creem el socket (un socket es un fitxer del qual llegirem o escriurem la informació que vulguem rebre/enviar)
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("!Error al crear el socket! ");
        return 0;
    }
    // creem la adreça del client
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    // agafem l'adreça passada per parèmetre a la terminal
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
        printf("inet_pton error for: %s", argv[1]);
        close(sock);
        return 0;
    }

    // s'inicia la conexió
    if(connect(sock, (SA*) &servaddr, sizeof(servaddr))){
        printf("error de conexió");
        close(sock);
        return 0;
    }

    sprintf(sendLine, "GET / HTTP/1.1\r\n\r\n");
    sendbytes = strlen(sendLine);
    
    if(write(sock, sendLine, sendbytes) != sendbytes){
        printf("");
        close(sock);
        return 0;
    }

    
    return 0;
}