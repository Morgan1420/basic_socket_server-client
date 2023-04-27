
// Trivial Torrent

// some includes here
#include "file_io.h"
//#include "file_io.c"
#include "logger.h"
#include "stdbool.h"
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <stdlib.h>

#define NUM_POLL_SOCK 54

// TODO: hey!? what is this?

/**
 * This is the magic number (already stored in network byte order).
 * See https://en.wikipedia.org/wiki/Magic_number_(programming)#In_protocols
 */
static const uint32_t MAGIC_NUMBER = 0xde1c3233; // = htonl(0x33321cde);

static const uint8_t MSG_REQUEST = 0;
static const uint8_t MSG_RESPONSE_OK = 1;
static const uint8_t MSG_RESPONSE_NA = 2;

enum { RAW_MESSAGE_SIZE = 13 };

int client(char **argv);
int server(char **argv);
void printBuffer(uint8_t *buffer_recv);

void printBuffer(uint8_t *buffer_recv) {
	printf("message = { MAGIC NUM: %u%u%u%u - MSG: %u - block_number: %u%u%u%u%u%u%u%u}\n", buffer_recv[0], buffer_recv[1], buffer_recv[2], buffer_recv[3],
	buffer_recv[4], buffer_recv[5], buffer_recv[6], buffer_recv[7],buffer_recv[8], buffer_recv[9], buffer_recv[10], buffer_recv[11], buffer_recv[12]);

}

//Main function.
int main(int argc, char **argv) {

	// return variable
	int r;

	set_log_level(LOG_DEBUG);
	log_printf(LOG_INFO, "Trivial Torrent (build %s %s) by %s", __DATE__, __TIME__, "J. DOE and J. DOE");

	
	// ==========================================================================
	// Parse command line
	// ==========================================================================

	
	if(argc > 2){
		r = server(argv);
	}
	else if(argc > 0){
		r = client(argv);
	}else{
		r = 0;
	}
	



	// The following statements most certainly will need to be deleted at some point...
	(void) argc;
	(void) argv;
	(void) MAGIC_NUMBER;
	(void) MSG_REQUEST;
	(void) MSG_RESPONSE_NA;
	(void) MSG_RESPONSE_OK;




	return r;
}


int client(char **argv){

	// creem una estructura torrent
	struct torrent_t torrent;
	const char destFile[1024] = "torrent_samples/client/test_file";

	if(create_torrent_from_metainfo_file(argv[1], &torrent, destFile) < 0)
	{
		perror("[-]create_torrent_from_metainfo_file error");
		return 0;
	}
	
	

	for(uint64_t serverPeerCount = 0; serverPeerCount < torrent.peer_count; serverPeerCount++){

		// create soket
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock == -1){
			perror("[-]socket create error");
			return  0;
		}

		// adreça servidor
		struct sockaddr_in addrServidor;
		addrServidor.sin_family = AF_INET;
		addrServidor.sin_addr.s_addr = INADDR_ANY; 
		addrServidor.sin_port = torrent.peers[serverPeerCount].peer_port;


		// connect to server
		if(connect(sock, (struct sockaddr *) &addrServidor, sizeof(addrServidor)) < 0){
			perror("[-]Error conectant");
			close(sock);
			return 0;
		} 
		
		
		for(uint64_t blockCount = 0; blockCount < torrent.block_count; blockCount++)
		{
			//printf("block_count: %lu\n", blockCount);
			if(torrent.block_map[blockCount] == MSG_RESPONSE_OK){
				printf("message allready recived");
				continue;
			}

			// crear el buffer
			uint8_t send_buffer [RAW_MESSAGE_SIZE], recv_buffer[RAW_MESSAGE_SIZE + MAX_BLOCK_SIZE];
			
			memset(send_buffer, 0, RAW_MESSAGE_SIZE);
			memset(recv_buffer, 0, RAW_MESSAGE_SIZE + MAX_BLOCK_SIZE);

			send_buffer[0] = (uint8_t) (MAGIC_NUMBER>>24);		
			send_buffer[1] = (uint8_t) (MAGIC_NUMBER>>16);
			send_buffer[2] = (uint8_t) (MAGIC_NUMBER>>8);
			send_buffer[3] = (uint8_t) MAGIC_NUMBER>>0;
			send_buffer[4] = MSG_REQUEST;
			send_buffer[12] = (uint8_t) blockCount;


			// enviar petició bloc
			if(send(sock,send_buffer, RAW_MESSAGE_SIZE,0) < 0){
				perror("[-]Send error al client");
				close(sock);
				return 0;
			}
			
					

			// rebre disponibilitat de block
			if((recv(sock,recv_buffer, RAW_MESSAGE_SIZE + MAX_BLOCK_SIZE,0)) < 0){
				perror("[-]Recv error");
				close(sock);
				return 0;
			}
			
			// comprovem que la request sigui correcte
			if(torrent.block_map[blockCount] == MSG_RESPONSE_OK){
				continue;
			}
			else if(recv_buffer[3] == (uint8_t) MAGIC_NUMBER>>0 && recv_buffer[2] == (uint8_t) (MAGIC_NUMBER>>8) && recv_buffer[1] == (uint8_t) (MAGIC_NUMBER>>16) && recv_buffer[0] == (uint8_t) (MAGIC_NUMBER>>24) && recv_buffer[4] == MSG_RESPONSE_OK)
			{
				
				printBuffer(recv_buffer);
				// creem el block que guardarem al fitxer
				struct block_t block;
				block.size = get_block_size(&torrent, blockCount);
				memset(block.data, 0, block.size);
				
				for(int i = 0; i < MAX_BLOCK_SIZE; i++){
					block.data[i] = recv_buffer[RAW_MESSAGE_SIZE + i];
				}
				
				if(store_block(&torrent, (uint64_t)blockCount, &block) < 0){
					printf("[-]block stored incorrectly\n");
					blockCount--;
					continue;
				}else
					printf("[+]block stored correctly\n");
				torrent.block_map[(int)blockCount] = MSG_RESPONSE_OK;
				
				
			}
			else{
				blockCount--;
			}
				
			

		}
		
		

		close(sock);
		printf("Socked closed\n");

		//break; // surto del bucle per nomes fer 1 iteració

	}

	return 0;

}

int server(char **argv){
	// declarem les variables generals
	struct torrent_t torrent;
	uint8_t buffers[NUM_POLL_SOCK][RAW_MESSAGE_SIZE];

	// creem una estructura torrent
	const char destFile[1024] = "torrent_samples/server/test_file_server";
	if(create_torrent_from_metainfo_file(argv[3], &torrent, destFile) < 0)
	{
		perror("[-]create_torrent_from_metainfo_file error");
		return 0;
	}

	// create soket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("[-]Socket create error");
		return  0;
	}
	// make socket nonblock
	int flags = fcntl(sock, F_GETFD, 0);
	if(fcntl(sock, F_SETFD, flags | O_NONBLOCK) < 0){ 
		perror("[-]Socket making non-block error");
		close(sock);
		return  0;
	}
	printf("[+]Server socket created\n");

	
	// addreça client
	int addr_len;
	struct sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_port = htons(((u_int16_t)atoi(argv[2]))); // potser no agafem el port correcte 


	// bind
	if(bind(sock, (struct sockaddr*) &addrServer, sizeof(addrServer)) < 0){
		perror("[-]Binding error");
		close(sock);
		return 0;
	}

	//listen
	if(listen(sock, 1) < 0){
		perror("[-]Listen error");
		close(sock);
		return 0;
	}
	
	int listener = sock;
	
	
	// creem un array de sockets i li afegim el primer 
	struct pollfd fds[NUM_POLL_SOCK];
	fds[0].fd = sock;
	fds[0].events = POLLIN;
	int fd_count = 1;



	// bucle del servidor
	while(true){

		// poll
		printf("... polling ...\n");
		
		int poll_count = poll(fds, (nfds_t)fd_count, -1);
		if(poll_count  < 0){
			perror("poll error");
			break;
		} else if (poll_count == 0){
			printf("poll() timed-out exiting program");
			break;
		}


		// buscant per dintre de les conexions obertes quines puc llegir o escriure
		for(int i = 0; i < fd_count; i++){ 
			
			// si el camp de revents es null continuem amb el bucle
			if(fds[i].revents == 0){ 
				printf("... continue ...\n");			
				continue;
			} 
			
			// preguntem si hem rebut dades i les podem tractar
			if(fds[i].revents & POLLIN){ 
			
				
				// si el servidor que ha de rebre dades es el listener significa que hi ha una nova conexió i la hem d'acceptar
				if(fds[i].fd == listener){

					// acceptem la conexió
					addr_len = sizeof(addrServer);

					int newfd = accept(listener, (struct sockaddr*) &addrServer, (socklen_t *__restrict__) &addr_len);
					if(newfd < 0){
						close(newfd);
						continue;
					} else {
						printf("[+]connecting new socket\n");

						fds[fd_count].fd = newfd;
												
						// make socket nonblock
						flags = fcntl(fds[fd_count].fd, F_GETFD, 0);
						if(fcntl(fds[fd_count].fd, F_SETFD, flags | O_NONBLOCK) < 0)
						{
							perror("[-]Socket making non-block error");
							return 0;
						}

						fds[fd_count].events = POLLIN;
												
						fd_count++;
					}

				} // per qualsevol altre rebem la info que pertoqui
				else {
					printf("[+]Descriptor %d is readable\n", fds[i].fd);

					// creem buffer per rebre dades i asignem la memoria del buffer 0
					uint8_t buffer_recv[13];
					memset(buffer_recv, 0, sizeof(buffer_recv));
					
					// recv la informació
					long int nbytes = recv(fds[i].fd, buffer_recv, sizeof(buffer_recv), 0);

					// si el recv() ha donat errors o s'ha acabat la conexió
					if(nbytes <= 0)
					{
						// got connection error
						if(nbytes < 0){
							perror("[-]recv() error");
						}
						else{
							printf("[+]connection closed\n");
						}
						close(fds[i].fd);
						// remove fd from list
						for(int j = i; j < fd_count - 1; j++){
							fds[j].fd = fds[j + 1].fd;
							fds[j].events = fds[j + 1].events;
							fds[j].revents = fds[j + 1].revents;
						}
						

						// tirem els buffers enrrera
						//for(int j = i; j < fd_count - 1; j++){
						//	*buffers[i] = buffers[i + 1];
						//}
						
						fd_count--;
					} 
					else 
					{
						printf("Bytes recived: %ld\n", nbytes);

						
						printBuffer(buffer_recv);

						if(buffer_recv[3] == (uint8_t) (MAGIC_NUMBER>>0) && buffer_recv[2] == (uint8_t) (MAGIC_NUMBER>>8) && buffer_recv[1] == (uint8_t) (MAGIC_NUMBER>>16) && 
						   buffer_recv[0] == (uint8_t) (MAGIC_NUMBER>>24) && buffer_recv[4] == MSG_REQUEST && buffer_recv[12] < (uint8_t)3
						)
						{
							printf("[+]request recived correctly\n");
							
												
							// copiem les dades del buffer temporal al bloc que pertoqui i les actualitzem
							for(int k = 0; k < 13; k++){
								buffers[i][k] = buffer_recv[k];
							}
							buffers[i][4] = MSG_RESPONSE_OK;


							// preparem el socket per a enviar dades
							fds[i].events = POLLOUT;
							
						} 
					}
					
					
				}
			}
			
			// Si el socket està llest per enviar les dades
			if(fds[i].revents & POLLOUT)
			{
				printf("...preparing for respone...\n");

				// creem el send_buffer  i el buidem
				uint8_t send_buffer[RAW_MESSAGE_SIZE + MAX_BLOCK_SIZE];
				memset(send_buffer, 0, sizeof(send_buffer));
				
				// Omplim els primers bytes amb el Metadata que pertoqui
				for(int k = 0; k < RAW_MESSAGE_SIZE; k++)
					send_buffer[k] = buffers[i][k];

				// carreguem en el block[i] les dades del block
				struct block_t block_temp;
				if(load_block(&torrent, (uint64_t)buffers[i][12], &block_temp) < 0){
					perror("load_block() error");

					// we close the socket
					close(fds[i].fd);

					// remove fd from list
					for(int j = i; j < fd_count - 1; j++){
						fds[j].fd = fds[j + 1].fd;
						fds[j].events = fds[j + 1].events;
						fds[j].revents = fds[j + 1].revents;
					}

					fd_count--;
				}	
				
				// copiem les dades del block al send_buffer
				for(int k = 0; k < MAX_BLOCK_SIZE; k++)
					send_buffer[RAW_MESSAGE_SIZE + k] = block_temp.data[k];

				
				// enviem les dades
				printf("...Sending ");
				printBuffer(send_buffer);

				long int s = send(fds[i].fd, send_buffer, RAW_MESSAGE_SIZE + MAX_BLOCK_SIZE, 0);
				if(s < 0){
					perror("[-]send error");


					// we close the socket
					close(fds[i].fd);
							
					// remove fd from list
					for(int j = i; j < fd_count - 1; j++){
						fds[j].fd = fds[j + 1].fd;
						fds[j].events = fds[j + 1].events;
						fds[j].revents = fds[j + 1].revents;
					}
							
					fd_count--;
				}
				else if(s == 0){
					perror("send recive = 0 ");

				}else
					printf("[+]Data sent correctly\n");

				// tornem a fer el fd POLLIN 
				fds[i].events = POLLIN; 
							
			}
		}
	}
	
	printf("[+]Closing connection");
	close(sock);

	return 0;
}



/*
 * Stress test:
 *  for i in {0..500}; do rm torrent_samples/client/test_file; reference_binary/ttorrent torrent_samples/client/test_file.ttorrent; done;
 * 
 * Make and run my server:
 *  make && bin/ttorrent -l 8080 torrent_samples/server/test_file_server.ttorrent
 * 
 * Remove torrent_file and run my client:
 *  rm torrent_samples/client/test_file; bin/ttorrent torrent_samples/client/test_file.ttorrent; 
 * 
 * Remove torrent_file and run teacher's client:
 *  rm torrent_samples/client/test_file; reference_binary/ttorrent torrent_samples/client/test_file.ttorrent; 
 * 
 * Comand per comparar els files
 *  cmp torrent_samples/client/test_file torrent_samples/server/test_file_server
 */
