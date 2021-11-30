// Iordache Ionut-Iulian 323CB

#include <stdbool.h>
#include "helpers.h"

// comanda subscribe primita la stdin si transmitere info. la server
bool send_subscribe_msg(char buffer[BUFLEN], int sockfd) {
	int n;
	bool check = false;
	int topic_sz = strlen(buffer) - 13; // nr. de caractere ptr. topic
    char sf_as_ch = buffer[strlen(buffer) - 2];
	tcp_msg msg; // creez mesaj pentru a il trimite la server
	memset(&msg, 0, sizeof(msg));
	msg.type = 1; // mesaj de subscribe
	memcpy(msg.topic, &buffer[10], topic_sz * sizeof(char)); // copiere din buffer
	msg.topic[topic_sz] = '\0'; // adaug \0 la final de topic
	msg.SF = (int) (sf_as_ch - 48); // char to int using ASCII code
	n = send(sockfd, &msg, sizeof(msg), 0);
	DIE(n == -1, "send() subscribe msg failed in subscriber!");
	printf("Subscribed to topic.\n");	
	check = true;

	return check;
}

// comanda unsubscribe primita la stdin si transmitere info. la server
bool send_unsubscribe_msg(char buffer[BUFLEN], int sockfd) {
	int n;
	bool check = false;
    int topic_sz = strlen(buffer) - 13;
	tcp_msg msg; //creare mesaj tcp pentru trimitere catre server
	memset(&msg, 0, sizeof(msg));
	msg.type = 0; // mesaj de unsubscribe
	memcpy(msg.topic, &buffer[12], topic_sz * sizeof(char));
	msg.topic[topic_sz] ='\0'; // adaug \0 la final de topic
	msg.SF = 0;
	n = send(sockfd, &msg, sizeof(msg), 0);
	DIE(n == -1, "send() unsubscribe msg failed in subscriber!");
	printf("Unsubscribed from topic.\n");
    check = true;
    
    return check;
}