#pragma once
#ifndef FILESHARE_H
#define FILESHARE_H
#include <iostream>
#include <cstring>
#include <ctime>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <sys/socket.h>
#include <sys/file.h>

#define COMM_PORT 12000
#define MAX_CLIENTS 1024
#define NUM_THREADS 2
#define ID "ID"
#define ASK_LEADER "ASK_LEADER"
#define CHANGE_ID "CHANGE ID"
#define GOOD_ID "GOOD_ID"
#define WANT_FILE "WANT"
#define GIVE_FILE "GIVE"
#define FILE_NOT_FOUND "FILE NOT FOUND"
#define FILE_DOWNLOADED "File has been downloaded"
#define DEFAULT_MSG "SERVER MSG"
#define MAX_NUM_NODES 4096
#define FILE_NAME_OPTIONS "[witch,wizard,elf,dwarf,human,ring,power,magic,evil,good]"
#define WELCOME "Welcome to the SimP2P File Share System!"
#define INPUT_STRING "What file would you like?(type a name from above)"
#define MAX_FILES 5
#define MAX_NAMES 10
#define BUFFER_SIZE 1024
#define NOT_LEADER 0
#define YES_LEADER 1
#define UNDECIDED 2
typedef struct addrInfo{
	std::vector<std::string> node_ips;	
}addrInfo;

typedef struct clientFileData{
	char* ip;
	std::string files[MAX_FILES];
}clientFileData;

typedef struct serverData{
	std::string serv_msg;
	int totalClients;
	bool printed;
	pthread_t commThreads[MAX_CLIENTS];
	std::map<int,clientFileData*> nodeInfo;
}serverData;

typedef struct threadData{
	std::string fileNames[MAX_FILES];
	std::string message;
	bool good_id;
	char buffer[BUFFER_SIZE];
	pthread_t fileThreads[MAX_CLIENTS];
	addrInfo* ips;
}threadData;

typedef struct commData{
	std::string msg;
	int totalClients,leader,my_id;
	int conn_fd;
	char con_client_addr[INET_ADDRSTRLEN];
	char buffer[BUFFER_SIZE];
	serverData* sData;
}commData;
typedef struct fileTransData{
	std::string ip;
	std::string fileName;
}fileTransData;
int node_id,am_leader;

void *peerToPeer(void*);
void printClientInfo(std::map<int,clientFileData*>);
int findFile(std::map<int,clientFileData*>,std::string);
void setupFiles(threadData*,std::string[]);
bool checkID(std::map<int,clientFileData*>,int);
/*
 * Function to handle communication with server and client.
 */
void *serverHandler(void*);
void clientHandler(threadData*);
void generateID();
std::vector<std::string> readBuffer(char*);
void usage();
/*
 * Should go through and contact each participating
 * node and begin the fileShare program.
 */
void run(addrInfo*);
void *leaderProcess(void*);
void *clientProcess(void*);
addrInfo* commandArgs(int argc,char** argv);
void printIPs(threadData*);

#endif // FILESHARE_H
