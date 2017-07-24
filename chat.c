#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "packets.h"
#include "dbase.h"
#include "sockstruct.h"

void* recvthread(void*);
void* bcthread(void* ptr);	
char username[10],usersurname[10];

int main(int argc, char *argv[])
{	
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);
	
	init();
	pthread_t listener,broadcast;
	pthread_create(&listener,NULL,recvthread,NULL);
	
	printf("Hello and welcome to Layer-II Chat System. Please enter your credentials\n[Name] [Surname]\nCredentials? > ");
	scanf("%s %s",username,usersurname);
	printf("Session opened, For help, hit 'H' or 'h'.\n");
	pthread_create(&broadcast,NULL,bcthread,NULL);
	
	while(1) { // chat client
		char cmd[1024];
		printf("Command? > ");
		scanf("%s",cmd);

		if ( strcmp(cmd,"Q") == 0 || strcmp(cmd,"q") == 0) {
			fill_exiting(&exiting,username,usersurname);
			char bcast_exit[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
			send_exiting(bcast_exit);
			printf("\nHave a good day\n");
			exit(0);
		}
		else if(strcmp(cmd,"C") == 0 || strcmp(cmd,"c") == 0) {
			printf("Enter [Name] [Surname] [Message]\n");		
			char target_name[10],target_surname[10], message[140];
			printf("Compose? > ");
			scanf("%s %s %[^\n]s",target_name,target_surname,message);
			fill_chat(&chat,sizeof(message),0x01,message);
			struct database *db;
			db = findNS(target_name,target_surname);
			send_chat(db->mac);
		}
		else if(strcmp(cmd,"H") == 0 || strcmp(cmd,"h") == 0) {
			printf("\nHelp Page\n==========================\nQ,q : Shuts down the system and broadcasts exiting message.\nC,c : Enters composer mode\
			\nH,h : Displays this help message\nL,l : Lists all clients in database\nX,x : Clears Screen\n");
			continue;
		}
		else if(strcmp(cmd,"L") == 0 || strcmp(cmd,"l") == 0) {
			printDatabase();
		}
		else if(strcmp(cmd,"X") == 0 || strcmp(cmd,"x") == 0) {
			system("clear");
		}
		else {
			printf("\nUnknown Command: %s\n",cmd);
			continue;
		}
	}
	
    return 0;
}

void* recvthread(void* ptr) {
	while(1) {
		numbytes = recvfrom(sockfdr, recvbuf, BUF_SIZ, 0, NULL, NULL);
		
		if(recvbuf[14] == 0x00) {
			// get mac, save name to list, kick back hello response
			uint8_t mac[6];
			char req_name[10],req_surname[10];
			for(int i = 0; i < 6; i++) mac[i] = recvbuf[i+6];
			memcpy(&req_name,recvbuf+15,10);
			memcpy(&req_surname,recvbuf+25,10);
			fill_hello_response(&hello_response,username,usersurname,req_name,req_surname);
			char bcast[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
			send_hello_response(bcast);
			
			struct database *db; 
			db = findNS(req_name,req_surname);
			if(db == NULL) {
				insertFirst(mac,req_name,req_surname);
			}
			db = findNS(req_name,req_surname);

		}
		else if(recvbuf[14] == 0x01) {
			//printf("unicast\n");
			// if someone ask my mac, reply to him
			uint8_t mac[6];
			char req_name[10],req_surname[10];
			for(int i = 0; i < 6; i++) mac[i] = recvbuf[i+6];
			memcpy(&req_name,recvbuf+15,10);
			memcpy(&req_surname,recvbuf+25,10);
			fill_hello_response(&hello_response,username,usersurname,req_name,req_surname);
			//if no records match, insertFirst(mac,req_name,req_surname);
			send_hello_response(mac);
			
		}
		else if(recvbuf[14] == 0x02){
			// printf("hello response\n");
			// save to file mac:name:sname
			
			
		}
		else if(recvbuf[14] == 0x03){
			char message[500];
			memcpy(&message,recvbuf+18,sizeof(recvbuf)-18);
			printf("\nIncoming message:\n%s\n",message);
			
			// print message, send ack, if mac not found, send query_ucast and write to ll
			uint8_t mac[6];
			for(int i = 0; i < 6; i++) mac[i] = recvbuf[i+6];
			fill_chat_ack(&chat_ack,(uint8_t)recvbuf[17]);
			send_chat_ack((char*)mac);
		}
		else if(recvbuf[14] == 0x04){
			printf("\nYour message received by user.\n");
		}
		else {
			char req_name[10],req_surname[10];
			memcpy(&req_name,recvbuf+15,10);
			memcpy(&req_surname,recvbuf+25,10);
			struct database *db;
			db = findNS(req_name,req_surname);
			while(db != NULL) {
				db = deleteNS(req_name,req_surname);
			}
			// delete entry mac:name:surname
		}
	}
}

void* bcthread(void* ptr) 
{
	while(1) {
		init();
		fill_query_bcast(&query_bcast,username,usersurname);
		send_query_bcast();
		sleep(10);
	}
}
