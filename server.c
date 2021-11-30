// Iordache Ionut-Iulian 323CB
// Tema 2

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
    
	int sockfd_tcp; // socket pt. client tcp
    int sockfd_udp; // socket pt. client udp
    int new_tcp_cli; // socket pt. client nou adaugat
    int portno; // portul pe care se realizeaza conexiunea
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, client_addr, udp_addr;
	socklen_t udp_len, cli_len;
	fd_set read_fds;	// multimea cu descriptori de citire
	fd_set temp_fds;	// multime temporara de descriptori de fisiere
	int fdmax;			// valoarea maxima a unui descriptor din read_fds
	int n, i, ret, clients_nr = 0, topics_nr = 0, saved_mesgs_nr = 0;
	
	// vector de structuri reprezentand clientii conectati
	subscriber* cli_connected = NULL;
	// corespondenta, asemanatoare unui map, dintre un topic si clientii acestuia
	Map* map = NULL;
	// structura reprezentand si un vector cu mesajele
	// ce vor fii transmise clientilor tcp
	msgs_buff* saved_mesgs = NULL;
	// vector de structuri client(subscriber) folosit la realocare
	subscriber* cli_conected_aux = NULL;
	
    // comanda pentru dezactivarea buffering la afisare
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // alocari memorie
	cli_connected = malloc(sizeof(subscriber));
	map = malloc(sizeof(Map));
	saved_mesgs = malloc(sizeof(msgs_buff));
    
	// verificare numar de parametrii pentru subscriber
    // si afisare mesaj in caz de neconcordanta
	if (argc < 2) {
		usage(argv[0]);
	}

	// salvare port primit ca parametru + verificare eroare
	portno = atoi(argv[1]);
	DIE(portno == 0, "Atoi func. at portno in server faild!");

	// eliberez multimile cu descriptori temp_fds si read_fds
	FD_ZERO(&temp_fds);
	FD_ZERO(&read_fds);

	// deschid socket inactiv pentru comunicare cu tcp
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp == -1, "Error creating passive socket in server!");

	// setare campuri structura (adresa, familia de adrese si portul)
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// asociez o adresa scoketului prin apelul bind()
	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret == -1, "Error bind() function (tcp) in server!");
	
	// ascult pe server cererile de conectare venite de la clienti tcp prin socketul pasiv
	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret == -1, "Error listen function in server!");
	
	FD_SET(0, &read_fds);// adaug 0(standard input) si sockfd in multimea
                         // descriptorilor de citire
	FD_SET(sockfd_tcp, &read_fds); 
	fdmax = sockfd_tcp; // actualizez val. lui fdmax cu val. lui sockfd_tcp
	// ------------------------------------------------------------------------

	//deschid socket pentru comunicare cu client udp
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp == -1, "Error creating udp socket sockfd_udp!");

	// setare campuri structura pentru udp
	memset((char *)&udp_addr, 0, sizeof(udp_addr));
	//completare campuri structura pentru udp
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;
	udp_len = sizeof(udp_addr);

	ret = bind(sockfd_udp, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret == -1, "Error bind() function (udp) in server!");

	FD_SET(sockfd_udp, &read_fds); // adaug sockfd_udp in multimea descriptorilor de citire

	// daca am un sockfd_udp mai mare de fdmax actualizez fdmax
	if (sockfd_udp > fdmax){
		fdmax = sockfd_udp;
	}

	while (1) {
		temp_fds = read_fds; // pastrez in temp_fds multimea descriptorilor
							 // pt. a face modificarile pe acesta
		// apelul de select + tratare eroare
		ret = select(fdmax + 1, &temp_fds, NULL, NULL, NULL);
		DIE(ret == -1, "select() failed in server!");

		// parcurg multimea de descriptori pentru a vedea pe care socket am primit date
		for (i = 0; i <= fdmax; i++) {
		// verific daca am citit date de la stdin
			if (FD_ISSET(i, &temp_fds)) { 
				if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN, stdin);
					// trimitere comanda exit la clientii tcp si oprire server
					if (strncmp(buffer, "exit", 4) == 0) {
						int socket;
						for (socket = 1; socket <= fdmax; socket++) {
							// verific sa nu inchid socketul pasiv de tcp si socketul udp
							if((socket != sockfd_tcp) && (socket != sockfd_udp)) {
								if (FD_ISSET(socket, &read_fds)) {
									memset(buffer, 0, BUFLEN);
									strcpy(buffer, "exit");
									// trimit comanda de exit pe socket catre client tcp
									n = send(socket, buffer, BUFLEN, 0);
									DIE(n == -1, "send() exit failed in server!");
									// inchidere socket
									close(socket);
								}
							}
						}
						// inchidere server
						break;
					} 
				}
				// verific daca am pimit date pe socketul de udp
				if (i == sockfd_udp) {
				 	memset(&buffer, 0, BUFLEN); // golire buffer
				 	// primesc un mesaj udp de la client_udp
					int ret = recvfrom(sockfd_udp, &buffer, BUFLEN, 0, (struct sockaddr*)&udp_addr, &udp_len);
					DIE(ret == -1, "Error receiving from udp socket in server!");

				 	udp_msg message; // declarare structura mesaj udp
				 	memset(&message, 0, sizeof(udp_msg));
					//completare mesaj udp folosind structura creata ce va fii transmis la sochetii de tcp
					memcpy(&message.topic, &buffer, 50); // topic
					message.tip_date = buffer[50]; // 1 octet reprezentand tipul de date
					memcpy(&message.payload, &buffer[51], 1500); // payload
					memcpy(&message.addr_ip, &udp_addr.sin_addr, sizeof(struct in_addr)); // ip
					message.port = udp_addr.sin_port; // port

					// parcurg topicurile si daca se gaseste potrivirea cu cea a mesajului
					// caut in subscriberii abonati la topicul respectiv
					for (int j = 0; j < topics_nr; j++) {
						if (strcmp(map[j].topic, message.topic) == 0) {
							for (int subs = 0; subs < map[j].nr_subscribers; subs++) {
								bool check = false; // verificare trimitere mesaj
								// verific daca printre clientii conectati se gaseste cel curent
								// (subs) pentru a ii trimite mesajul
								for (int cli = 0; cli < clients_nr; cli++) {
									if (strcmp(cli_connected[cli].id, map[j].subscribers[subs].id) == 0) {
										int subs_sockfd_1, n;
										subs_sockfd_1 = map[j].subscribers[subs].sockfd_client;
										n = send(subs_sockfd_1, &message, sizeof(message), 0);
										DIE(n == -1, "Error send() to client tcp from server!");
										check = true; // s-a trimis mesajul catre client
									}
								}
								// daca clientul nu este conectat poate fii totusi in lista de subscriberi
								// pentru un topic
								if (check == false) {
									// cazul in care are S&F = 0 deci subscriberul nu primeste mesajul
									if (map[j].subscribers[subs].SF == 0) {
										continue;
									}
									
									// in cazul in care mai am mesaje salvate adaug si acest mesaj
									bool ok = false;
									for (int curr_msg = 0; curr_msg < saved_mesgs_nr; curr_msg++) {
										if (strcmp(saved_mesgs[curr_msg].id, map[j].subscribers[subs].id)== 0) {
											// mai adaug un mesaj la lista de mesaje, pt. clienti neconectati cu realloc
											udp_msg *new_msg;
											new_msg = realloc(saved_mesgs[curr_msg].msg,
															 (saved_mesgs[curr_msg].nr_msg + 1) * sizeof(udp_msg));
											if (new_msg == NULL) {
						    					printf("Error realloc() function in new_msg!\n");		
											} else {
												int k;
												saved_mesgs[curr_msg].msg = new_msg;
												k = saved_mesgs[curr_msg].nr_msg;
												// adaug mesaj la finalul celorlalte mesaje
												memcpy(&saved_mesgs[curr_msg].msg[k], &message, sizeof(message));
												saved_mesgs[curr_msg].nr_msg++; // incrementare contor din msgs_buff
												ok = true;
											}
										}
									}
									// adaug o noua intrare in msgs_buff dupa un nou id a unui subscr.
									// care nu a mai avut mesaje stocate in lipsa lui pana acum
									if (ok == false) {
										msgs_buff buff_aux;
										memset(&buff_aux, 0, sizeof(msgs_buff));

										buff_aux.msg = malloc(sizeof(udp_msg));
										memcpy(&buff_aux.msg, &message, sizeof(message)); // copiere mesaj
										memcpy(&buff_aux.id, &map[j].subscribers[subs].id,
											   sizeof(map[j].subscribers[subs].id)); // id
										buff_aux.nr_msg = 1;
										saved_mesgs[saved_mesgs_nr] = buff_aux; 
										saved_mesgs_nr++;
									}
								}
							}
							break;
						}
					}
				} else if (i == sockfd_tcp) {
					// primesc o cerere de conexiune pe socketul pasiv
					memset(buffer, 0, BUFLEN);
					cli_len = sizeof(client_addr);
					// accept conexiunea de la un client nou la server
					new_tcp_cli = accept(i, (struct sockaddr*)&client_addr, &cli_len);
					DIE(new_tcp_cli == -1, "Error accepting connection in server!");

					// dezactivare algoirm Neagle pentru micsorarea latentei
					int flag = 1;
					int result = setsockopt(new_tcp_cli, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
 					DIE(result < 0, "setsockopt");

					// primire mesaj cu id-ul clientului tcp
					ret = recv(new_tcp_cli, buffer, sizeof(buffer), 0);
					DIE(ret == -1, "Error receiving message that contain the id of client!");

					// verific daca id-ul primit apartine clientilor deja conectati
					// caz in care afisez mesajul corespunzator si trimit la client comanda exit
					bool check = false;
					for (int j = 0; j< clients_nr; j++) {
						if (strcmp(cli_connected[j].id, buffer) == 0) {
							char err_buff[BUFLEN];
							sprintf(err_buff, "exit");
							printf("Client %s already connected.\n", cli_connected[j].id);
							n = send(new_tcp_cli, err_buff, BUFLEN, 0);
							DIE(n == -1, "Error sending message for connection already established!");
							// inchidere socket
							close(new_tcp_cli);
							check = true;
							break;
						}
					}
					if(check == true) {
						continue;
					}

					// afisare conexiunea cu clientul conf. specs
					printf("New client %s connected from %s:%d.\n", buffer,
							inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

					// declar un nou client pe care il voi adauga in lista de clienti conectati
					subscriber new_cli_conctd; // noul client ce se conecteaza
					memset(&new_cli_conctd, 0, sizeof(subscriber));

					memcpy(new_cli_conctd.id, buffer, strlen(buffer)); //id
					new_cli_conctd.sockfd_client = new_tcp_cli; // skfd
					new_cli_conctd.SF = 0; // S&F

					// realocare vector de clienti conectati la server
					cli_conected_aux = realloc(cli_connected, (clients_nr + 1) * sizeof(subscriber));
					if (cli_conected_aux == NULL) {
    					printf("Error realloc() function in cli_conected_aux!\n");	
					} else {
						cli_connected = cli_conected_aux;
					}
					cli_connected[clients_nr] = new_cli_conctd; // adaugare client in vector de clienti
					clients_nr++;

					// daca clientul a mai fost conectat ii actualizez socketul
					for(int j = 0; j < topics_nr; j++) {
						for(int subs = 0; subs < map[j].nr_subscribers; subs++) {
							if (strcmp(map[j].subscribers[subs].id, new_cli_conctd.id) == 0) {
									map[j].subscribers[subs].sockfd_client = new_tcp_cli;
								}
							}
						}
					// efectuare transmitere mesaje catre client tcp cu id coresp.
					for (int j = 0; j < saved_mesgs_nr; j++) {
						if (strcmp(saved_mesgs[j].id, new_cli_conctd.id) == 0) {
							for (int curr_msg = 0; curr_msg < saved_mesgs[j].nr_msg; curr_msg++) {
								n = send(new_tcp_cli, &saved_mesgs[j].msg[curr_msg],
											 sizeof(saved_mesgs[j].msg[curr_msg]), 0);
								DIE(n == -1, "Error sending stored messages from server!");
							}
							memset(&saved_mesgs[j].msg, 0, sizeof(saved_mesgs[j].msg));
						}
						memmove(&saved_mesgs[j], &saved_mesgs[j+1], 
								(saved_mesgs_nr - j - 1) * sizeof(msgs_buff));
						saved_mesgs_nr--;
					}
					// adaugare socket in multimea descriptorilor de citire
					FD_SET(new_tcp_cli, &read_fds);
					// actualizare valoarea lui fdmax
					if (new_tcp_cli > fdmax) { 
						fdmax = new_tcp_cli;
					}
				} else {
					// am primit date pe un socket de comunicatie intre
					// client tcp si server
					tcp_msg msg;
					memset(&msg, 0, sizeof(msg));
					int ret1 = recv(i, &msg, sizeof(msg), 0);
					// printf("%s\n", msg.topic);
					DIE(ret1 == -1, "Error receiving message from tcp client!");

					// verificare inchidere conexiune si afisare mesaj (recv() intoarce 0)
					if (ret1 == 0) {
						for (int j = 0; j < clients_nr; j++) {
							if (i == cli_connected[j].sockfd_client) {	
								printf("Client %s disconnected.\n", cli_connected[j].id);
								// actualizare vector clienti conectati prin eliminarea
								// celui ce s-a deconectat
								memmove(&cli_connected[j], &cli_connected[j+1], 
											(clients_nr-j-1) * sizeof(subscriber));
								clients_nr--;
								//inchidere socket
								close(i);
							}
						}
						// eliminarea socket-ului care s-a inchis
						FD_CLR(i, &read_fds);	
					} 
					else {
						// in continuare mai trebuiau tratate cateva situatii
						// in functie de tipul de mesaj pe care il primesc
					}
				}
			}
		}
	}

	// la final inchid socketi din read_fds
	for(int i = 0; i<= fdmax; i++) {
		if (FD_ISSET(i, &read_fds)) {
			close(i);
		}
	}
	for(int j = 0; j < topics_nr; j++) {
		free(map[j].subscribers);
	}
	free(cli_connected);
	free(map);
	free(cli_conected_aux);
	return 0;
}