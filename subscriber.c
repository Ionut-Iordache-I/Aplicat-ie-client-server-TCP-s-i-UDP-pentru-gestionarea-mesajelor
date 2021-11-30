// Iordache Ionut-Iulian 323CB
// Tema 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include "helpers.h"
#include "subscriber_functions.c"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, fdmax;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	bool check = false;

	fd_set temp_fds; // multime temporara de descriptori de fisiere
	fd_set read_fds; // multimea cu descriptori de citire

    // comanda pentru dezactivarea buffering la afisare
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// verificare numar de parametrii pentru subscriber
    // si afisare mesaj in caz de neconcordanta
	if (argc < 4) {
		usage(argv[0]);
	}

	// eliberez multimile cu descriptori temp_fds si read_fds
	FD_ZERO(&temp_fds);
	FD_ZERO(&read_fds);

	// crearea socket-ului pentru a putea realiza comunicarea
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd == -1, "Can't create subscriber socket!"); // tratare eroare

	FD_SET(0, &read_fds); // adaug 0(standard input) si sockfd in multimea
                          // descriptorilor de citire
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd; // actualizez val. lui fdmax cu val. lui sockfd

	// completez in serv_addr adresa serverului, familia de adrese si portul pentru conectare
	serv_addr.sin_family = AF_INET;
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	serv_addr.sin_port = htons(atoi(argv[3]));
	DIE(ret == 0, "Error inet_aton() in subscriber!"); // tratare eroare

	// realizarea conexiunii cu serverul 
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret == -1, "Error connect() in subscriber!");

	// trimit id-ul clientului catre server(pentru a verifica ca nu mai am
    // alt client cu acelasi nume)
    size_t len_id_client = strlen(argv[1]);
	n = send(sockfd, argv[1],len_id_client, 0);
	DIE(n == -1, "send() id failed in subscriber!"); // tratare eroare

	while (1) {
		temp_fds = read_fds; // copiez read_fds in temp_fds deoarece dupa
                             // apelul de select() multimile de descriptori
                             // vor contine numai descriptorii pe care s-au primit date
        // apelul de select
		ret = select(fdmax + 1, &temp_fds, NULL, NULL, NULL);
		DIE(ret == -1, "select() failed in subscriber!");

        // verific daca am citit date de la standard input prin cautarea 
        // descriptorului 0 in temp_fds cu FD_ISSET()
		check = false;
		if (FD_ISSET(0, &temp_fds)) {
			memset(buffer, 0, BUFLEN);
            // preluarea a BUFLEN octeti de date de la stdin
			fgets(buffer, BUFLEN, stdin);

            // verificare comanda subscribe primita la stdin si transmitere info. la server
			if (strncmp(buffer, "subscribe", 9) == 0 && buffer[10] && buffer[strlen(buffer) - 2]) {
				check = send_subscribe_msg(buffer, sockfd);
			}

            // verificare comanda unsubscribe primita la stdin si transmitere info. la server
			else if (strncmp(buffer, "unsubscribe", 11) == 0 && buffer[12]) {
				check = send_unsubscribe_msg(buffer, sockfd);
			}

            // verificare comanda exit 
			else if (strncmp(buffer, "exit", 4) == 0) {
				break;
			} 
            else if (check == false) {
				// printf ("Command not found! Try:\nsubscribe <TOPIC> <SF>\nunsubscribe <TOPIC>\nnext\n");
                continue;
			}
		}

        // verific daca sockfd se afla in temp_fds 
        // (in cazul in care am primit date pe socket)
		if (FD_ISSET(sockfd, &temp_fds)) {
  			udp_msg msg ; // mesaj udp ce vine prin server
  			memset(&msg, 0, sizeof(msg));
			ret = recv(sockfd, &msg, sizeof(msg), 0); // primesc mesaj de la server
			DIE(ret == -1, "recv() msg failed in subscriber!");
			// daca primesc exit inchid subscriber-ul
            if (strncmp(msg.topic, "exit", 4) == 0) {
				break;
			} else {
				//cazul de interpretare a mesajelor udp
				// verificare pentru cazul INT
				if (msg.tip_date == 0) {
					uint32_t int_nr = ntohl(*((uint32_t*)(msg.payload + 1)));
					// daca primul octet din payload este 1 atunci int_nr este un nr. negativ
                    if(msg.payload[0] == 1) {
						int_nr = int_nr * (-1);
					}
					printf("%s:%d - %s - INT - %d\n",inet_ntoa(msg.addr_ip),
						    ntohs(msg.port), msg.topic, int_nr);
				}

				// verificare pentru cazul SHORT_REAL
				if (msg.tip_date == 1) {
				   	double real_nr = ntohs(*(uint16_t*)(msg.payload));
            		real_nr = real_nr / 100;
            		printf("%s:%d - %s - SHORT_REAL - %.2f\n",inet_ntoa(msg.addr_ip),
            				ntohs(msg.port), msg.topic, real_nr);
            	}
            	// verificare pentru cazul FLOAT
				if(msg.tip_date == 2) {
			        double real_nr = ntohl(*(uint32_t*)(msg.payload + 1));
			        //unsigned p = ntohl(*(uint8_t*)(msg.payload + sizeof(uint32_t*)));
			        real_nr /= pow(10,msg.payload[5]);
			            if (msg.payload[0] == 1) {
			                real_nr *= -1;
			            }
			        printf("%s:%d - %s - FLOAT - %f\n",inet_ntoa(msg.addr_ip),
			        		ntohs(msg.port), msg.topic, real_nr);
            	}
            	// verificare pentru caz STRING
            	if(msg.tip_date == 3) {
            		printf("%s:%d - %s - STRING - %s\n", inet_ntoa(msg.addr_ip),
            		        ntohs(msg.port), msg.topic, msg.payload);
            	}
            }
  		}
	}

    // inchidere socket
	close(sockfd);

	return 0;
}