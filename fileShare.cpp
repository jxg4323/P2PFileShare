#include "fileShare.h"

void usage(){
	std::cerr << "usage: ./fileShare <inputFileName>" << std::endl;
	exit(-1);
}
/*
 * Tokenize the information sent between nodes.
 * Return a list of the tokens.
 */
std::vector<std::string> readBuffer(char* buffer){
	std::string str=buffer;
	str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
	std::stringstream ss(str);
	std::string item;
	std::vector<std::string> tokens;
	while(std::getline(ss,item,':')){
		tokens.push_back(item);
	}
	return tokens;
	
}

/*
 * Given a thread_id generate pseudorandom number
 * that is different than the given thread_id.
 * Change the given threadData id to the newly
 * generated id.
 */
void generateID(threadData* info){
	int result,check;
	check = info->thread_id;
	srand(time(NULL));
	while((result = ((rand()%MAX_NUM_NODES)+1)) == check){}	
	info->thread_id = result;
}

/**
 * Set up server socket on the containg node and
 * wait for a response from a client node.
 */
void* leaderProcess(void* data){
	threadData* info;
	info = (threadData*)data;
	info->message = "SERVER THREAD";
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	char buffer[1024] = {0};
	// -------- Server Process --------- //
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		std::cerr << "socket failed" << std::endl;
	}
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		std::cerr << "setsockopt" << std::endl;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(COMM_PORT);

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){
		std::cerr << "Bind Failed" << std::endl;
	}

	if(listen(server_fd,3) < 0){
		std::cerr << "Listen" << std::endl;
	}

	if((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))<0){
		std::cerr << "accept" << std::endl;
	}
	//TODO: function here that will do the leaders duties
	valread = read(new_socket, buffer, 1024);
	std::vector<std::string> tokens = readBuffer(buffer);	
	
	send(new_socket,m,strlen(m),0);
	std::cerr << "hello message sent" << std::endl;
	
	pthread_exit(NULL);
}
void* clientProcess(void* data){
	threadData* info;
	info = (threadData*)data;
	generateID(info);
	info->message = "ID: ";
	info->message += std::to_string(info->thread_id);
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	//TODO: loop through each ip address and form connection
	const char* ip = info->ips->node_ips[0].c_str();
	char buffer[1024] = {0};

	// -------- Client Process --------- //
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		std::cerr << "Socket Creation Error" << std::endl;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(COMM_PORT);
	
	
	if(inet_pton(AF_INET, ip,&serv_addr.sin_addr)<=0){
		std::cerr << "Invalid address/ Address not support" << std::endl;
	}
	// wait and try to connect with node	
	while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){
//		std::cerr << "Connection Failed" << std::endl;	
	}

	//TODO: Create function that will perform client actions
	send(sock,m,strlen(m),0);
	std::cout << "Message sent" << std::endl;
	valread = read(sock,buffer,1024);
	std::vector<std::string> tokens = readBuffer(buffer);
	std::cout << buffer << std::endl;
	
	pthread_exit(NULL);
}

/**
 * Each node will have to behave as a server initailly
 * by receiving messages from all other nodes containg
 * the nodes id and electing a leader.
 */
void decideLeader(addrInfo* ips){
	pthread_t threads[NUM_THREADS];
	pthread_attr_t attr;
	threadData data[NUM_THREADS];
	
	int rc,i(0);
	void* status;
	std::cout << i << std::endl;
	// give the server and client threads list of ip addresses
	data[i].ips = ips;

	// Initialize and set thread joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	// create server process to listen
	rc = pthread_create(&threads[i], &attr, leaderProcess, (void*)&data[i]);
	// create client process to send out messages and requests
	i = 1;
	rc = pthread_create(&threads[i], &attr, clientProcess, (void*)&data[i]);

	pthread_attr_destroy(&attr);
	for(i = 0; i< NUM_THREADS;i++){
		rc = pthread_join(threads[i], &status);
	}
	pthread_exit(NULL);
}

addrInfo* commandArgs(int argc, char** argv){
	addrInfo* node = (addrInfo*)malloc(sizeof(threadData));	
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

void printIPs(addrInfo* info){
	for(auto& i: info->node_ips){
		std::cout << i << std::endl;
	}
}

int main(int argc,char** argv) {
	addrInfo* info = commandArgs(argc,argv);
	decideLeader(info);
}
