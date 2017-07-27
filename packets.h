#ifndef _PACKETS_H
#define _PACKETS_H

#define MAX_NAME_SIZE 10
#define MAX_FILE_NAME_SIZE 32


enum {
   QUERY_BROADCAST,
   QUERY_UNICAST,
   HELLO_RESPONSE,
   CHAT,
   CHAT_ACK,
   EXITING,
   FILE_BROADCAST,
   FILE_MD5_ACK,
   FILE_QUERY_UCAST,
   FILE_STATUS
};

struct query_bcast {
	//struct ether_header eth;
    uint8_t type;
    char name[MAX_NAME_SIZE];
    char surname[MAX_NAME_SIZE];
} __attribute__((packed));

struct query_ucast {
	//struct ether_header eth;
    uint8_t type;
    char name[MAX_NAME_SIZE];
    char surname[MAX_NAME_SIZE];
    char target_name[MAX_NAME_SIZE];
    char target_surname[MAX_NAME_SIZE]; 
} __attribute__((packed));

struct hello_response { 
	//struct ether_header eth;
    uint8_t type;
    char responder_name[10];
    char responder_surname[10];
    char requester_name[10];
    char requester_surname[10];
} __attribute__((packed));

struct chat {
	//struct ether_header eth;
	uint8_t type;
	uint16_t length;
	uint8_t packet_id;
	char message[500];
} __attribute__((packed));

struct chat_ack{
	//struct ether_header eth;
    uint8_t type;
    uint8_t packet_id;
} __attribute__((packed));

struct exiting{
	//struct ether_header eth;
    uint8_t type;
    char name[10];
    char surname[10];
} __attribute__((packed));

struct database {
	struct ether_header eth;
   char name[10];
   char surname[10];
   uint8_t mac[6];
   struct database *next;
} __attribute__((packed));

struct file_header {
    struct ether_header eth;
	uint8_t type;
    char name[MAX_NAME_SIZE];
    char surname[MAX_NAME_SIZE];
    char filename[MAX_FILE_NAME_SIZE];
    uint32_t filesize;
    uint16_t pkt_count;
    uint16_t pkt_index;
    uint32_t pkt_size;
    char payload[0];
} __attribute__((packed));

struct file_query_ucast {
	struct ether_header eth;
	uint8_t type;
	char filename[MAX_FILE_NAME_SIZE];
	uint8_t packet_id;
} __attribute__((packed));

struct file_status {
	struct ether_header eth;
	uint8_t type;
	uint8_t packet_id;
	uint8_t status; // 0 : NACK, 1: ACK
} __attribute__((packed));

#endif
