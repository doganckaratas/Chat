#include "sockstruct.h"

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
