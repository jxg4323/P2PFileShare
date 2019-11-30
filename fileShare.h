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
#define NUM_THREADS 2
#define LEADER_MSG "YOUR LEADER"
#define CHANGE_ID "CHANGE ID"
#define GOOD_ID "GOOD_ID"
#define MAX_NUM_NODES 4096
#define BUFFER_SIZE 1024
typedef struct addrInfo{
	std::vector<std::string> node_ips;	
}addrInfo;

typedef struct serverData{
	std::string serv_msg;
	int totalClients;
	int conn_fd;
	char con_client_addr[INET_ADDRSTRLEN];
	char buffer[BUFFER_SIZE];
	std::map<int,char*> nodeInfo;
}serverData;

typedef struct threadData{
	int thread_id;
	std::string message;
	char buffer[BUFFER_SIZE];
	addrInfo* ips;
}threadData;

void informLeader(serverData*);
bool checkID(serverData*,int);
void serverHandler(serverData*);
void clientHandler(threadData*);
void generateID(threadData*);
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
