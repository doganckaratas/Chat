#include "dbase.h"

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
