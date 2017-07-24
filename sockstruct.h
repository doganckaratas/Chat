#ifndef _SOCKSTRUCT_H
#define _SOCKSTRUCT_H

#define DEFAULT_IF "eth0"
#define BUF_SIZ 1024

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

#endif
