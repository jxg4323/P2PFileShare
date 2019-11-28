#pragma once
#ifndef FILESHARE_H
#define FILESHARE_H
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <string>
#include <vector>
#include <sys/socket.h>
#define COMM_PORT 12000
#define NUM_THREADS 2

typedef void * (*THREADFUNCPTR)(void *);

typedef struct threadData{
	int thread_id;
	std::vector<std::string> node_ips;
	std::string message;
}threadData;
void usage();
/*
 * Should go through and contact each participating
 * node and assign unique IDs to each node and the
 * node with the lowest ID will be dubbed the leader.
 */
void decideLeader();
void *leaderProcess(void* threadData);
void *clientProcess(void* threadData);
threadData* commandArgs(int argc,char** argv);
void printIPs(threadData*);

#endif // FILESHARE_H
