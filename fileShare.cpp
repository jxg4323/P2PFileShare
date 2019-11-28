#include "fileShare.h"

void usage(){
	std::cerr << "usage: ./fileShare <inputFileName>" << std::endl;
	exit(-1);
}
/**
 * Set up server socket on the containg node and
 * wait for a response from a client node.
 */
void* leaderProcess(void* data){
	threadData* info;
	info = (threadData*)data;
	//TODO: randomize thread id to reduce collisions
	info->thread_id = 1;
	info->message = "ID: ";
	info->message += std::to_string(info->thread_id);
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	char buffer[1024] = {0};
	// -------- Server Process --------- //
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		std::cerr << "socket failed" << std::endl;
		exit(-1);
	}
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		std::cerr << "setsockopt" << std::endl;
		exit(-1);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(COMM_PORT);

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
		std::cerr << "Bind Failed" << std::endl;
		exit(-1);
	}

	if(listen(server_fd,3) < 0){
		std::cerr << "Listen" << std::endl;
		exit(-1);
	}

	if((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))<0){
		std::cerr << "accept" << std::endl;
		exit(-1);
	}
	valread = read(new_socket, buffer, 1024);
	std::cerr << buffer << std::endl;
	send(new_socket,m,strlen(m),0);
	std::cerr << "hello message sent" << std::endl;
	
}
void* clientProcess(void* data){
	threadData* info;
	info = (threadData*)data;
	//TODO: randomize thread id to reduce collisions
	info->thread_id = 1;
	info->message = "ID: " + std::to_string(info->thread_id);
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	char buffer[1024] = {0};

	// -------- Client Process --------- //
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		std::cerr << "Socket Creation Error" << std::endl;
		exit(-1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(COMM_PORT);
	
	if(inet_pton(AF_INET, info->node_ips[0].c_str(),&serv_addr.sin_addr)<=0){
		std::cerr << "Invalid address/ Address not support" << std::endl;
		exit(-1);
	}
	
	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
		std::cerr << "Connection Failed" << std::endl;
		exit(-1);
	}
	send(sock,m,strlen(m),0);
	std::cout << "Message sent" << std::endl;
	valread = read(sock,buffer,1024);
	std::cout << buffer << std::endl;
}

/**
 * Each node will have to behave as a server initailly
 * by receiving messages from all other nodes containg
 * the nodes id and electing a leader.
 */
void decideLeader(){
	pthread_t threads[NUM_THREADS];
	struct threadData data[NUM_THREADS];
	int rc;
	void* status;
	// create server process to listen
	rc = pthread_create(&threads[0], NULL, leaderProcess, (void*)&data[0]);
	// create client process to send out messages and requests
	rc = pthread_create(&threads[1], NULL, clientProcess, (void*)&data[1]);

	for(int i = 0; i< NUM_THREADS;i++){
		rc = pthread_join(threads[i], &status);
	}
	pthread_exit(NULL);
}

threadData* commandArgs(int argc, char** argv){
	threadData* node = (threadData*)malloc(sizeof(threadData));	
	std::string line;
	std::fstream in;
	if( argc != 2)
		usage();
	else{
		std::string filename = argv[1];
		in.open(filename, std::ios::in);
		while(std::getline(in, line)){
			node->node_ips.push_back(line);	
		}
	}
	in.close();
	return node;
}

void printIPs(threadData* info){
	for(auto& i: info->node_ips){
		std::cout << i << std::endl;
	}
}

int main(int argc,char** argv) {
	threadData* info = commandArgs(argc,argv);
	//printIPs(info);
	//decideLeader();
	leaderProcess((void*)info);
}