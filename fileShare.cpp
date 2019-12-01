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
 * Read the tokens which contain the ID and the file names
 * that the node contains.
 */
clientFileData* clientInfo(std::vector<std::string> tokens){
	clientFileData* info = (clientFileData*)malloc(sizeof(clientFileData));
	for( int i = 0; i<MAX_FILES; i++){
		info->files[i] = tokens.at(i+2);
	}
	return info;
}
/*
 * Given a thread_id generate pseudorandom number
 * that is different than the given thread_id.
 * Change the given threadData id to the newly
 * generated id.
 */
void generateID(){
	int result,check;
	check = node_id;
	srand(time(NULL));
	while((result = ((rand()%MAX_NUM_NODES)+1)) == check){}	
	node_id = result;
}

/*
 * This function will deal with message received
 * by the server.
 */
void *serverHandler(void* data){
	commData* comms = (commData*)data;
	comms->msg = DEFAULT_MSG;
	read(comms->conn_fd,comms->buffer, BUFFER_SIZE);	
	std::vector<std::string> tokens = readBuffer(comms->buffer);
	clientFileData* cData;
	const char* m;
	std::cout << "SERVER RECVEIVED" << std::endl;
	// ID Check
	if( tokens.size() > 1 && am_leader == UNDECIDED ){
		int cid = std::stoi(tokens.at(1));
		// Check to confirm the client id is unique if not exit
		if( checkID(comms->sData,cid) == false ){
			comms->msg = CHANGE_ID;
		}else{
			comms->msg = GOOD_ID;
			cData = clientInfo(tokens);
			cData->ip = comms->con_client_addr;
			// only add once it is clear no other cids have the id
			comms->sData->nodeInfo.insert(std::pair<int,clientFileData*>(cid,cData));	
			std::pair<int,clientFileData*> leader = *(comms->sData->nodeInfo.begin());
			if(comms->sData->nodeInfo.size() == comms->totalClients){
				leaderIP = leader.second->ip;
				if( node_id == leader.first ){ // This is the leader node
					am_leader = YES_LEADER;
				}else{
					am_leader = NOT_LEADER;
				}
			}
		}
	}else if( am_leader == YES_LEADER ){ // Leader Duties

	}else if( am_leader == NOT_LEADER ){ // non leader duties

	}
	
	// send Return message to client
	m = comms->msg.c_str();
	comms->conn_fd;
	send(comms->conn_fd,m,strlen(m),0);
}

/*
 * Go through each ID,address pair and confirm the given
 * ID is unique amongst all other nodes.
 */
bool checkID(serverData* sData, int cid){
	bool result = true;
	std::map<int,clientFileData*>::iterator it;
	for( it = sData->nodeInfo.begin(); it != sData->nodeInfo.end(); it++){
		if( it->first == cid )
			result = false;
	}
	return result;
}

/**
 * Set up server socket on the containg node and
 * wait for a response from a client node.
 */
void* leaderProcess(void* data){
	serverData* info = (serverData*)data;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	
	struct sockaddr_in address;
	int sock = 0, clientNum = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	bool cont = true;
	struct sockaddr_in serv_addr;
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
	while( cont ){
		if((new_socket = accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))<0){
			std::cerr << "accept" << std::endl;
		}
		// Communication information	
		commData* comms = (commData*)malloc(sizeof(commData));
		// Client IP address
		inet_ntop(AF_INET, &(address.sin_addr), comms->con_client_addr, INET_ADDRSTRLEN);
		comms->totalClients = info->totalClients;
		comms->conn_fd = new_socket;
		comms->sData = info;
		// initialize and have server communicate to individual clients
		pthread_mutex_lock( &mutex );
		pthread_create(&(info->commThreads[clientNum]),NULL, serverHandler,(void*)comms);
		pthread_detach(info->commThreads[clientNum]);	
		pthread_mutex_unlock( &mutex );
		clientNum++;
	}
	close(server_fd);
	pthread_exit(NULL);
}

void clientHandler(threadData* cData){
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* msg = DEFAULT_MSG;
	const char* ip = leaderIP.c_str();
//	while(1){
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		std::cerr << "Socket Creation Error" << std::endl;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(COMM_PORT);
	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<0){
		std::cerr << "Invalid address/ Address not supported" << std::endl;
	}
	// continue trying to connect to ip
	while(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<=0){}
	//TODO: insert function to talk with user
	cData->message = "WANT:power";
	msg = cData->message.c_str();
	send(sock,msg,strlen(msg),0);
	valread = read(sock,cData->buffer,BUFFER_SIZE);
	std::vector<std::string> tokens = readBuffer(cData->buffer);

//	}
	
}
/*
 * Setup, create, and write data to each file.
 */
void setupFiles(threadData* data,std::string names[]){
	srand(time(NULL));
	int loc = rand()%9;
	std::string file_name = names[loc];
	std::string text;
	for( int i = 0; i < MAX_FILES; i++){
		std::fstream file;
		file_name += std::to_string(i);
		text = "text written to " + file_name;
		data->fileNames[i] = file_name;
		file.open(file_name, std::ios::out);
		file << text << std::endl;
		file.close();
		file_name = names[loc];
	}	
}

void* clientProcess(void* data){
	threadData* info = (threadData*)data;
	generateID();
	info->message = "ID: ";
	info->message += std::to_string(node_id);
	for( int i = 0; i < MAX_FILES; i++){
		info->message += ":" + info->fileNames[i];
	}
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	const char* ip;
	for( int i = 0; i < (info->ips->node_ips.size()); i++){
		info->good_id = false;
		ip = info->ips->node_ips.at(i).c_str();	
		// -------- Client Process --------- //
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			std::cerr << "Socket Creation Error" << std::endl;
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(COMM_PORT);
		if(inet_pton(AF_INET, ip,&serv_addr.sin_addr)<=0){
			std::cerr << "Invalid address/ Address not support" << std::endl;
		}
		while( info->good_id == false ){
			// wait and try to connect with node	
			while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){}
			send(sock,m,strlen(m),0);
			valread = read(sock,info->buffer,BUFFER_SIZE);
			std::vector<std::string> tokens = readBuffer(info->buffer);
			if( tokens.at(0) == GOOD_ID )
				info->good_id = true;
			else
				generateID();
		}
	}
	std::cout << "Notified Everyone" << std::endl;
	//TODO: function which will communicate with leader node
	clientHandler(info);
	pthread_exit(NULL);
}

/**
 * Each node will have to behave as a server initailly
 * by receiving messages from all other nodes containg
 * the nodes id and electing a leader.
 */
void decideLeader(addrInfo* ips){
	pthread_t threads[NUM_THREADS];
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_attr_t attr;
	threadData data;
	serverData sData;
	
	int rc,i(0);
	void* status;
	// give the server and client threads list of ip addresses
	data.ips = ips;	
	sData.totalClients = ips->node_ips.size();
	// create client's files
	std::string names[MAX_NAMES] = {"witch","wizard","elf","dwarf","human","ring","power","magic","evil","good"};	
	setupFiles(&data,names);
	// Initialize and set thread joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_lock( &mutex );
	// create server process to listen
	rc = pthread_create(&threads[i], &attr, leaderProcess, (void*)&sData);
	pthread_mutex_unlock( &mutex );
	pthread_mutex_lock( &mutex );
	// create client process to send out messages and requests
	i = 1;
	rc = pthread_create(&threads[i], &attr, clientProcess, (void*)&data);
	pthread_mutex_unlock( &mutex );

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
