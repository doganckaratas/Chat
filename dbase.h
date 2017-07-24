#ifndef _DBASE_H
#define _DBASE_H

void insertFirst(uint8_t mac[6], char name[10], char surname[10]);
struct database* deleteFirst();
bool isDatabaseEmpty();
int databaseLength();
struct database* findMac(uint8_t mac[6]);
struct database* findNS(char name[10], char surname[10]);
struct database* deleteNS(char name[10], char surname[10]);

struct database *head = NULL;
struct database *current = NULL;

#endif
