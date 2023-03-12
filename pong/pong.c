 #include "logger.h"
	#include<stdio.h>
	#include<sys/types.h>
	#include<sys/socket.h>
	#include<sys/un.h>
	#include<string.h>
	#include<netdb.h>
	#include<netinet/in.h>
	#include<arpa/inet.h>
	#include<string.h>
	#include<unistd.h>
	#include <stdlib.h>
	#include <stdbool.h>

	#define PORT 8080


	int main(int argc, char **argv) {

		(void) argc; // This is how an unused parameter warning is silenced.
		(void) argv;
		
		
		struct sockaddr_in addrServer, addrClient;
		char buffer[1025];
		unsigned int length = sizeof(struct sockaddr_in);

		
		// crear sockets
		int sock = socket(AF_INET, SOCK_DGRAM, 0);
		if(sock == -1){
			perror("Socket not created correctly");
			return  1;
		}
		
		// adreça servidor
		addrServer.sin_family = AF_INET;
		addrServer.sin_addr.s_addr = INADDR_ANY;
		addrServer.sin_port = htons(PORT);

		// bind
		int b = bind(sock, (struct sockaddr *) &addrServer, sizeof(addrServer));
		if (b == -1){
			perror("Unable to bind");
			close(sock);
			return 0;
		}

		// bucle
		for (int i = 0; i < 3; i++)
		{
			memset(buffer, '\0', sizeof(buffer));

			// recive message
			long int mida_mssg = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &addrClient, &length) ;
			if(mida_mssg < 0){
				perror("recivefrom: ");
			}			
			
			printf("%s", buffer);

			if(sendto(sock, buffer, (long unsigned int)mida_mssg, 0, (struct sockaddr*)&addrClient, sizeof(addrClient)) < 0){
				perror("Sendto error: ");
			}
		}
		
		close(sock);
		// TODO: some socket stuff here

		return 0;
	}
