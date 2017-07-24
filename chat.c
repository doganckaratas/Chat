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

#define DEFAULT_IF "eth0"
#define BUF_SIZ 1024

void* recvthread(void*);
void* bcthread(void* ptr);	
void init(void);
static void fill_query_bcast(struct query_bcast *q,char s_name[10], char s_surname[10]);
static void fill_query_ucast(struct query_ucast *q,char s_name[10], char s_surname[10], char r_name[10], char r_surname[10]);
static void fill_hello_response(struct hello_response *h, char s_name[10], char s_surname[10], char r_name[10], char r_surname[10]);
static void fill_chat(struct chat *c,uint16_t length,uint8_t packet_id,char message[500]);
static void fill_chat_ack(struct chat_ack *ca,uint8_t packet_id);
static void fill_exiting(struct exiting *e,char name[10], char surname[10]);
void send_query_bcast(void);
void send_query_ucast(const char[6]);
void send_chat(const char[6]);
void send_hello_response();
void send_chat_ack(const char[6]);
void send_exiting(const char[6]);

void insertFirst(uint8_t mac[6], char name[10], char surname[10]);
struct database* deleteFirst();
bool isDatabaseEmpty();
int databaseLength();
struct database* findMac(uint8_t mac[6]);
struct database* findNS(char name[10], char surname[10]);
struct database* deleteNS(char name[10], char surname[10]);

int sockfds;
int sockfdr;
int sockopt;
ssize_t numbytes;
struct ifreq if_idx;
struct ifreq ifopts;
struct ifreq if_mac;
int tx_len = 0;
char sendbuf[BUF_SIZ];
uint8_t recvbuf[BUF_SIZ];
struct ether_header *ehs = (struct ether_header *) sendbuf;
struct ether_header *ehr = (struct ether_header *) recvbuf;
struct sockaddr_ll socket_address;
char ifName[IFNAMSIZ];

struct query_bcast query_bcast;
struct query_ucast query_ucast;
struct hello_response hello_response;
struct chat chat;
struct chat_ack chat_ack;
struct exiting exiting;

struct database *head = NULL;
struct database *current = NULL;

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
//	printf("Session opened, Hit Q for disconnect, Hit C for compose message\n");
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

void init(void) {
	//strcpy(ifName,DEFAULT_IF);	
	
	if ((sockfds = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	    perror("socket");
	}
	
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfds, SIOCGIFINDEX, &if_idx) < 0)
	
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfds, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");
	
	if ((sockfdr = socket(PF_PACKET, SOCK_RAW, htons(0x1234))) == -1) {
		perror("listener: socket");	
		exit(-1);
	}	
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfdr, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfdr, SIOCSIFFLAGS, &ifopts);
	
	if (setsockopt(sockfdr, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(sockfdr);
		exit(EXIT_FAILURE);
	}
	
	if (setsockopt(sockfdr, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		close(sockfdr);
		exit(EXIT_FAILURE);
	}
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

void printDatabase() {
	struct database *db = head;
	int idx = 0;
	printf("\nConnected Clients Table\n===============\n[MAC]\t\t\t[Name]\t\t[Surname]\n");
	while(db != NULL) {
		for(int i = 0; i < 6;i++) printf("%x ",db->mac[i]);
		printf("\t%s\t\t%s\n",db->name,db->surname);
		idx++;
		db = db->next;
	}
	printf("\n");
}
void insertFirst(uint8_t mac[6], char name[10], char surname[10]) {
	struct database *link = (struct database*) malloc(sizeof(struct database));
	memcpy(link->mac,mac,sizeof(uint8_t)*6);
	strcpy(link->name,name);
	strcpy(link->surname,surname);
	link->next = head;
	head = link;
}

struct database* deleteFirst() {
	struct database *tempLink = head;
	head = head->next;
	return tempLink;
}

bool isDatabaseEmpty() {
   return head == NULL;
}

int databaseLength() {
   int length = 0;
   struct database *current;
	
   for(current = head; current != NULL; current = current->next) {
      length++;
   }
	
   return length;
}

struct database* findMac(uint8_t mac[6]) {
	struct database* current = head;

	if(head == NULL) {
		return NULL;
	}

	while(memcmp(current->mac,mac,6) != 0) {
		if(current->next == NULL) {
			return NULL;
		} else {
			current = current->next;
		}
	}
	return current;
}

struct database* findNS(char name[10], char surname[10]) {
	struct database* current = head;

	if(head == NULL) {
		return NULL;
	}

	while((strcmp(current->name,name) != 0) && (strcmp(current->surname,surname) != 0)) {
		if(current->next == NULL) {
			return NULL;
		} else {
			current = current->next;
		}
	}
	return current;
}

struct database* deleteNS(char name[10], char surname[10]) {
	struct database* current = head;
	struct database* previous = NULL;
	
	if(head == NULL) {
		return NULL;
	}

	while((strcmp(current->name,name) != 0) && (strcmp(current->surname,surname) != 0)) {
		if(current->next == NULL) {
			return NULL;
		} else {
			previous = current;
			current = current->next;
		}
	}

	if(current == head) {
		head = head->next;
	} else {
		previous->next = current->next;
	}    
	return current;
}

void send_query_bcast() 
{
	const char dest_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

	memset(sendbuf, 0, BUF_SIZ);
	
	ehs->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0]; // local mac
	ehs->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	ehs->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	ehs->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	ehs->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	ehs->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	ehs->ether_dhost[0] = dest_mac[0];
	ehs->ether_dhost[1] = dest_mac[1];	// remote mac
	ehs->ether_dhost[2] = dest_mac[2];
	ehs->ether_dhost[3] = dest_mac[3];
	ehs->ether_dhost[4] = dest_mac[4];
	ehs->ether_dhost[5] = dest_mac[5];
	
	ehs->ether_type = htons(0X1234);

	tx_len += sizeof(struct ether_header);
	
	memcpy(sendbuf+tx_len,&query_bcast,sizeof(query_bcast));
	tx_len+=sizeof(query_bcast);
	
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	socket_address.sll_halen = ETH_ALEN;
	
	socket_address.sll_addr[0] = dest_mac[0]; //sll_addr = ether_dhost
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];
	
	if (sendto(sockfds, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	 
	tx_len = 0;
    memset(&query_bcast,0,sizeof(struct query_bcast));
}

void send_query_ucast(const char dest_mac[6]) 
{
	memset(sendbuf, 0, BUF_SIZ);
	
	ehs->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0]; // local mac
	ehs->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	ehs->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	ehs->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	ehs->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	ehs->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	ehs->ether_dhost[0] = dest_mac[0];
	ehs->ether_dhost[1] = dest_mac[1];	// remote mac
	ehs->ether_dhost[2] = dest_mac[2];
	ehs->ether_dhost[3] = dest_mac[3];
	ehs->ether_dhost[4] = dest_mac[4];
	ehs->ether_dhost[5] = dest_mac[5];
	
	ehs->ether_type = htons(0X1234);

	tx_len += sizeof(struct ether_header);
	
	memcpy(sendbuf+tx_len,&query_ucast,sizeof(query_ucast));
	tx_len+=sizeof(query_ucast);
	
	
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	socket_address.sll_halen = ETH_ALEN;
	
	socket_address.sll_addr[0] = dest_mac[0]; //sll_addr = ether_dhost
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];
	
	if (sendto(sockfds, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	    
	tx_len = 0;
    memset(&query_ucast, 0, sizeof(struct query_ucast));
}

void send_hello_response() 
{
	const char dest_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	memset(sendbuf, 0, BUF_SIZ);
	
	ehs->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0]; // local mac
	ehs->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	ehs->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	ehs->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	ehs->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	ehs->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	ehs->ether_dhost[0] = dest_mac[0];
	ehs->ether_dhost[1] = dest_mac[1];	// remote mac
	ehs->ether_dhost[2] = dest_mac[2];
	ehs->ether_dhost[3] = dest_mac[3];
	ehs->ether_dhost[4] = dest_mac[4];
	ehs->ether_dhost[5] = dest_mac[5];
	
	ehs->ether_type = htons(0X1234);

	tx_len += sizeof(struct ether_header);
	
	memcpy(sendbuf+tx_len,&hello_response,sizeof(hello_response));
	tx_len+=sizeof(hello_response);
	
	
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	socket_address.sll_halen = ETH_ALEN;
	
	socket_address.sll_addr[0] = dest_mac[0]; //sll_addr = ether_dhost
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];
	
	if (sendto(sockfds, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	    
	tx_len = 0;
    memset(&hello_response, 0, sizeof(struct hello_response));
}

void send_chat(const char dest_mac[6]) 
{
	memset(sendbuf, 0, BUF_SIZ);
	
	ehs->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0]; // local mac
	ehs->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	ehs->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	ehs->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	ehs->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	ehs->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	ehs->ether_dhost[0] = dest_mac[0];
	ehs->ether_dhost[1] = dest_mac[1];	// remote mac
	ehs->ether_dhost[2] = dest_mac[2];
	ehs->ether_dhost[3] = dest_mac[3];
	ehs->ether_dhost[4] = dest_mac[4];
	ehs->ether_dhost[5] = dest_mac[5];
	
	ehs->ether_type = htons(0X1234);

	tx_len += sizeof(struct ether_header);
	
	memcpy(sendbuf+tx_len,&chat,sizeof(chat));
	tx_len+=sizeof(chat);
	
	
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	socket_address.sll_halen = ETH_ALEN;
	
	socket_address.sll_addr[0] = dest_mac[0]; //sll_addr = ether_dhost
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];
	
	if (sendto(sockfds, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	    
	tx_len = 0;
    memset(&chat, 0, sizeof(struct chat));
}

void send_chat_ack(const char dest_mac[6]) 
{
	memset(sendbuf, 0, BUF_SIZ);
	
	ehs->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0]; // local mac
	ehs->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	ehs->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	ehs->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	ehs->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	ehs->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	ehs->ether_dhost[0] = dest_mac[0];
	ehs->ether_dhost[1] = dest_mac[1];	// remote mac
	ehs->ether_dhost[2] = dest_mac[2];
	ehs->ether_dhost[3] = dest_mac[3];
	ehs->ether_dhost[4] = dest_mac[4];
	ehs->ether_dhost[5] = dest_mac[5];
	
	ehs->ether_type = htons(0X1234);

	tx_len += sizeof(struct ether_header);
	memcpy(sendbuf+tx_len,&chat_ack,sizeof(chat_ack));
	tx_len+=sizeof(chat_ack);
	
	
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	socket_address.sll_halen = ETH_ALEN;
	
	socket_address.sll_addr[0] = dest_mac[0]; //sll_addr = ether_dhost
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];
	
	if (sendto(sockfds, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	    
	tx_len = 0;
    memset(&chat_ack, 0, sizeof(struct chat_ack));
}

void send_exiting(const char dest_mac[6]) 
{
	memset(sendbuf, 0, BUF_SIZ);
	
	ehs->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0]; // local mac
	ehs->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	ehs->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	ehs->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	ehs->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	ehs->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	ehs->ether_dhost[0] = dest_mac[0];
	ehs->ether_dhost[1] = dest_mac[1];	// remote mac
	ehs->ether_dhost[2] = dest_mac[2];
	ehs->ether_dhost[3] = dest_mac[3];
	ehs->ether_dhost[4] = dest_mac[4];
	ehs->ether_dhost[5] = dest_mac[5];
	
	ehs->ether_type = htons(0X1234);

	tx_len += sizeof(struct ether_header);

	memcpy(sendbuf+tx_len,&exiting,sizeof(exiting));
	tx_len+=sizeof(exiting);
	
	
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	socket_address.sll_halen = ETH_ALEN;
	
	socket_address.sll_addr[0] = dest_mac[0]; //sll_addr = ether_dhost
	socket_address.sll_addr[1] = dest_mac[1];
	socket_address.sll_addr[2] = dest_mac[2];
	socket_address.sll_addr[3] = dest_mac[3];
	socket_address.sll_addr[4] = dest_mac[4];
	socket_address.sll_addr[5] = dest_mac[5];
	
	if (sendto(sockfds, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");
	    
	tx_len = 0;
    memset(&exiting, 0, sizeof(struct exiting));
}


static void fill_query_bcast(struct query_bcast *q,char s_name[10], char s_surname[10])
{
    q->type = QUERY_BROADCAST;
    snprintf(q->name, MAX_NAME_SIZE, "%s", s_name);
    snprintf(q->surname, MAX_NAME_SIZE, "%s", s_surname);
}


static void fill_query_ucast(struct query_ucast *q,char s_name[10], char s_surname[10], char r_name[10], char r_surname[10])
{
    q->type = QUERY_UNICAST;
    snprintf(q->name, MAX_NAME_SIZE, "%s", s_name);
    snprintf(q->surname, MAX_NAME_SIZE, "%s", s_surname);
    snprintf(q->target_name, MAX_NAME_SIZE, "%s", r_name);
    snprintf(q->target_surname, MAX_NAME_SIZE, "%s", r_surname);
}

static void fill_hello_response(struct hello_response *h, char s_name[10], char s_surname[10], char r_name[10], char r_surname[10])
{
    h->type = HELLO_RESPONSE;
    snprintf(h->responder_name, MAX_NAME_SIZE, "%s", s_name);
    snprintf(h->responder_surname, MAX_NAME_SIZE, "%s", s_surname);
    snprintf(h->requester_name, MAX_NAME_SIZE, "%s", r_name);
    snprintf(h->requester_surname, MAX_NAME_SIZE, "%s", r_surname);
}


static void fill_chat(struct chat *c, uint16_t length, uint8_t packet_id, char message[500]) 
{
	c->type = CHAT;
	c->length = (uint16_t) htons(length);
	c->packet_id = packet_id;
	snprintf(c->message,length,"%s",message);
}

static void fill_chat_ack(struct chat_ack *ca, uint8_t packet_id) 
{
	ca->type = CHAT_ACK;
	ca->packet_id = packet_id;
}

static void fill_exiting(struct exiting *e, char name[10], char surname[10]) 
{
	e->type = EXITING;
	snprintf(e->name,MAX_NAME_SIZE,"%s",name);
	snprintf(e	->surname,MAX_NAME_SIZE,"%s",surname);
}
