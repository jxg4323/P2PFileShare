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
#define COMM_PORT 12000
#define MAX_CLIENTS 1024
#define NUM_THREADS 2
#define CHANGE_ID "CHANGE ID"
#define GOOD_ID "GOOD_ID"
#define DEFAULT_MSG "SERVER MSG"
#define MAX_NUM_NODES 4096
#define FILE_NAME_ST "witch"
#define MAX_FILES 5
#define BUFFER_SIZE 1024
typedef struct addrInfo{
	std::vector<std::string> node_ips;	
}addrInfo;

typedef struct serverData{
	std::string serv_msg;
	int totalClients;
	pthread_t commThreads[MAX_CLIENTS];
	std::map<int,char*> nodeInfo;
}serverData;

typedef struct threadData{
	std::string fileNames[MAX_FILES];
	std::string message;
	char buffer[BUFFER_SIZE];
	addrInfo* ips;
}threadData;

typedef struct commData{
	std::string msg;
	int totalClients;
	int conn_fd;
	bool terminate;
	char con_client_addr[INET_ADDRSTRLEN];
	char buffer[BUFFER_SIZE];
	serverData* sData;
}commData;
int node_id;

bool checkID(serverData*,int);
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
 * node and assign unique IDs to each node and the
 * node with the lowest ID will be dubbed the leader.
 */
void decideLeader(addrInfo*);
void *leaderProcess(void*);
void *clientProcess(void*);
addrInfo* commandArgs(int argc,char** argv);
void printIPs(threadData*);

#endif // FILESHARE_H
