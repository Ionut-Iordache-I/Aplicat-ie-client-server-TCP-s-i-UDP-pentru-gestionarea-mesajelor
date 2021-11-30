// Iordache Ionut-Iulian 323CB

#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// modificat - initial era 256
#define BUFLEN		1560	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare
#define LENGTH 50

// structura ce descrie un mesaj tcp transmis catre server cu informatiile de mai jos
typedef struct tcp_mesg {
    unsigned char type; // descrie mesajul transmis serverului (subscribe=1 / unsubscribe=0)
    char topic[LENGTH];  // descrie topicul la care este abonat clientul tcp
    unsigned char SF; // descrie functionalitatea pentru store&forward (poate fii 0 sau 1)
} tcp_msg;

// structura unui mesaj udp transmis catre server conform formatului
// i-am adaugat si ip + port pentru afisarile corespunzatoare
typedef struct udp_mesg {
    char topic[50]; // sir de maxim 50 de caractere
    unsigned int tip_date;
    char payload[1500]; // variabil in functie de tipul de date
   	struct in_addr addr_ip;
	unsigned short int port;
} udp_msg;

// informatii referitoare la un client 
typedef struct s_subscriber {
	char id[10]; // sir de maxim 10 caractere
	int sockfd_client;
	int SF; // val. 0 sau 1
} subscriber;

// structura in care se afla maparea dintre 
// un topic si subscriberii acestuia + nr. subs.
typedef struct s_Map {
    char topic[LENGTH];
    int nr_subscribers;
    subscriber *subscribers;
} Map;

// contine mesajele de la clientii udp, stocate pana la
// reconectarea unui client tcp la socket pentru a fii transmise
typedef struct udp_msgs_buffer {
	char id[10];
	int nr_msg; // counter pentru mesaje udp
	udp_msg *msg; // vector de mesaje udp
} msgs_buff;

#endif