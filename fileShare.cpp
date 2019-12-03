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
 * Looks through the mapping of clients and files
 * that the server has and checks to see if any 
 * of the clients have the file.
 * Return 0 for FILE_NOT_FOUND
 * Return Client ID if file is found
 */
int findFile(std::map<int,clientFileData*> nodeInfo, std::string file){
	int result = 0;
	std::map<int,clientFileData*>::iterator it;
	for( it = nodeInfo.begin(); it != nodeInfo.end(); it++){
		for( int i = 0; i < MAX_FILES; i++){
			std::string cmp = it->second->files[i];
			if( cmp == file )
				result = it->first;	
		}	
	}
	return result;	
}

/*
 * Go through each client known to the server and
 * print all the files associated with that client.
 */
void printClientInfo(std::map<int,clientFileData*> nodeInfo){
	std::map<int,clientFileData*>::iterator it;
	for( it = nodeInfo.begin(); it != nodeInfo.end(); it++){
		std::cout << "Client " << it->first << ":" << std::endl << "\t";
		for( int i = 0; i < MAX_FILES; i++ ){
			std::cout << it->second->files[i] << ", ";
		}
		std::cout << std::endl;
	}
}

/*
 * This function will deal with message received
 * by the server.
 */
void *serverHandler(void* data){
	commData* comms = (commData*)data;
	comms->msg = DEFAULT_MSG;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	
	// create tmp data for map and check printed flag
	serverData* tmp = (serverData*)malloc(sizeof(serverData));
	pthread_mutex_lock( &mutex );
	tmp->nodeInfo = comms->sData->nodeInfo;
	tmp->printed = comms->sData->printed;
	pthread_mutex_unlock( &mutex );

	// setup for new input and read it when it comes in		
	memset( comms->buffer, '\0', sizeof(char)*BUFFER_SIZE );
	read(comms->conn_fd,comms->buffer, BUFFER_SIZE);	
	std::vector<std::string> tokens = readBuffer(comms->buffer);
	clientFileData* cData;
	const char* m;
	
	
	if( tokens.size() > 0 ){	
		// ID Check
		if( tokens.at(0) == ID ){
			int cid = std::stoi(tokens.at(1));
			// Check to confirm the client id is unique if not exit
			if( checkID(tmp->nodeInfo,cid) == false ){
				comms->msg = CHANGE_ID;
			}else{
				comms->msg = GOOD_ID;
				cData = clientInfo(tokens);
				cData->ip = comms->con_client_addr;
				// only add once it is clear no other cids have the id
				tmp->nodeInfo.insert(std::pair<int,clientFileData*>(cid,cData));	
			}
		}else if( tokens.at(0) == WANT_FILE ){ // Leader Duties
			std::cout << "tokens: ";
			for( auto& s: tokens )
				std::cout << s << ", ";
			std::cout << std::endl;
			std::map<int,clientFileData*>::iterator it;
			std::string file = tokens.at(1);
			std::string m;
			int key = findFile(tmp->nodeInfo,file);
			it = tmp->nodeInfo.find(key);
			m = it->second->ip;
			std::cout << "SERVER: found it " << m << std::endl; 
			comms->msg = m.c_str();	
	
		}else if( tokens.at(0) == ASK_LEADER && tmp->nodeInfo.size() > 0 ){
			std::pair<int,clientFileData*> leader = *(tmp->nodeInfo.begin());
			if( comms->my_id == leader.first ){
				comms->leader = YES_LEADER;
			}else
				comms->leader = NOT_LEADER;
				comms->msg = leader.second->ip;
			if( tmp->printed == false && comms->leader == YES_LEADER ){
					printClientInfo(tmp->nodeInfo);
					tmp->printed = true;
			}
		}else if( tokens.at(0) == GIVE_FILE ){ // non leader duties
			std::map<int,clientFileData*>::iterator it;
			clientFileData* c;
			std::string fileName = tokens.at(1);
			it = tmp->nodeInfo.find(comms->my_id);
			c = it->second;
			std::string total;
			for( int i = 0; i < MAX_FILES;i++ ){
				if( c->files[i] == fileName ){
					std::fstream file;
					std::string line;
					file.open(fileName, std::ios::in);
					while( getline(file,line) ){
						total += line;
					}
					file.close();
					comms->msg = fileName + ":" + total;
				}
			}
		}
		// send Return message to client
		m = comms->msg.c_str();
		comms->conn_fd;
		send(comms->conn_fd,m,strlen(m),0);
		// Exit 
		pthread_mutex_lock( &mutex );
		comms->sData->nodeInfo = tmp->nodeInfo;
		comms->sData->printed = tmp->printed;
		pthread_mutex_unlock( &mutex );
		free( tmp );
		close( comms->conn_fd );
	}
}

/*
 * Go through each ID,address pair and confirm the given
 * ID is unique amongst all other nodes.
 */
bool checkID(std::map<int,clientFileData*> nodeInfo, int cid){
	bool result = true;
	std::map<int,clientFileData*>::iterator it;
	for( it = nodeInfo.begin(); it != nodeInfo.end(); it++){
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
	struct sockaddr_in serv_addr;
	bool cont = true;
	info->printed = false;
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
	if(listen(server_fd,10) < 0){
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
		comms->leader = UNDECIDED;
		// Include pointer to serverData
		comms->sData = info;
		// lock to check global leader flag, and node id
		pthread_mutex_lock( &mutex );		
		comms->leader = am_leader;
		comms->my_id = node_id;
		pthread_mutex_unlock( &mutex );
		// initialize and have server communicate to individual clients
		pthread_create(&(info->commThreads[clientNum]),NULL, serverHandler,(void*)comms);
		pthread_detach(info->commThreads[clientNum]);
		clientNum++;
	}
	close(server_fd);
	pthread_exit(NULL);
}

/**
 * Open terminal with the client and retrieve the name
 * of the file they would like to download.
 */
std::string clientInterface(){
	std::string input,fileName;
	srand(time(NULL));
	int inc = rand()%(MAX_FILES-1);
	std::cout << FILE_NAME_OPTIONS << std::endl << INPUT_STRING << std::endl;
	std::cin >> input;
	std::cout << "you picked " << input << std::endl;
	input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());
	std::cout << "you picked " << input  << std::endl;	
	fileName = input+std::to_string(inc);
	return fileName;
}

/**
 * Handles communication of information between client
 * and leader node.
 */
void clientHandler(threadData* cData, std::string LIP){
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1, f_threads = 0;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* msg = DEFAULT_MSG;
	const char* ip = LIP.c_str();
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		std::cerr << "Socket Creation Error" << std::endl;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(COMM_PORT);
	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<0){
		std::cerr << "Invalid address/ Address not supported" << std::endl;
	}
	// continue trying to connect to ip
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<=0){}
	while(1){
		fileTransData* trans = (fileTransData*)malloc(sizeof(fileTransData));
//		trans->fileName = clientInterface();	
		trans->fileName = "power1";
		cData->message = "WANT:" + trans->fileName;	
		msg = cData->message.c_str();
		std::cout << "Client tells leader " << cData->message <<std::endl;
		send(sock,msg,strlen(msg),0);
		valread = read(sock,cData->buffer,BUFFER_SIZE);
		std::vector<std::string> tokens = readBuffer(cData->buffer);
		std::cout << "RECV: " << tokens.at(0) << std::endl;
		trans->ip = tokens.at(0);
		
		pthread_mutex_lock( &mutex );
		pthread_create(&(cData->fileThreads[f_threads]),NULL, peerToPeer, (void*)trans);
		pthread_mutex_unlock( &mutex );
		f_threads++;
		for( int i = 0; i < f_threads; i++)
			pthread_join(cData->fileThreads[i],NULL);
	}
	pthread_exit(NULL);	
}

/*
 * Opens a socket to the node which has the desired file
 * and recieves the file.
 */
void *peerToPeer(void* data){
	fileTransData* trans = (fileTransData*)data;
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE];
	const char* m = DEFAULT_MSG;
	const char* ip = trans->ip.c_str();
	std::string msg = GIVE_FILE;
	msg += ":" + trans->fileName;
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		std::cerr << "Socket Creation Error" << std::endl;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(COMM_PORT);
	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<0){
		std::cerr << "Invalid address/ Address not supported" << std::endl;
	}
	// continue trying to connect to ip
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<=0){}
	m = msg.c_str();
	std::cout << "Clients reachs out to another client with: " << msg << std::endl;
	send(sock,m,strlen(m),0);
	valread = read(sock,buffer,BUFFER_SIZE);
	std::vector<std::string> tokens = readBuffer(buffer);
	std::fstream file;
	file.open(trans->fileName, std::ios::out);
	file << tokens.at(1) << "\n";
	file.close();
	std::cout << FILE_DOWNLOADED << std::endl;
	close(sock);
	pthread_exit(NULL);
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
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock( &mutex );
	generateID();
	info->message = "ID: ";
	info->message += std::to_string(node_id);
	pthread_mutex_unlock( &mutex );
	for( int i = 0; i < MAX_FILES; i++){
		info->message += ":" + info->fileNames[i];
	}
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	std::string leaderIP;
	const char* ip;
	// Broadcast ID and file information
	for( int i = 0; i < (info->ips->node_ips.size()); i++){
		info->good_id = false;
		ip = info->ips->node_ips.at(i).c_str();	
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			std::cerr << "Socket Creation Error" << std::endl;
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(COMM_PORT);

		std::cout << "client trying to reach: " << ip << std::endl;

		if(inet_pton(AF_INET, ip,&serv_addr.sin_addr)<=0){
			std::cerr << "Invalid address/ Address not support" << std::endl;
		}
		// continuing broadcasting until a unique id is achieved
		while( info->good_id == false ){
			// wait and try to connect with node	
			while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){}
			send(sock,m,strlen(m),0);
			valread = read(sock,info->buffer,BUFFER_SIZE);
			std::vector<std::string> tokens = readBuffer(info->buffer);
			if( tokens.at(0) == GOOD_ID )
				info->good_id = true;
			else{
				info->message = "ID:";
				pthread_mutex_lock( &mutex );
				generateID();
				info->message += std::to_string(node_id);
				pthread_mutex_unlock( &mutex );
				for( int i = 0; i < MAX_FILES; i++){
					info->message += ":" + info->fileNames[i];
				}
			}
		}
		close(server_fd);
	}
	// Discover Leader
	for( int i = 0; i < (info->ips->node_ips.size()); i++){
		info->good_id = false;
		ip = info->ips->node_ips.at(i).c_str();	
		 
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			std::cerr << "Socket Creation Error" << std::endl;
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(COMM_PORT);

		if(inet_pton(AF_INET, ip,&serv_addr.sin_addr)<=0){
			std::cerr << "Invalid address/ Address not support" << std::endl;
		}
		while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){}
		std::cout << "Asking for leader ip" << std::endl;
		memset( info->buffer, '\0', sizeof(char)*BUFFER_SIZE);
		// get leader info
		m = ASK_LEADER;
		send(sock,m,strlen(m),0);
		valread = read(sock,info->buffer,BUFFER_SIZE);
		std::vector<std::string> tokens = readBuffer(info->buffer);
		leaderIP = tokens.at(0);
		std::cout << info->buffer << std::endl;
		close(server_fd);
	}
	
	clientHandler(info, leaderIP);

	pthread_exit(NULL);
}

/**
 * Each node will have to behave as a server initailly
 * by receiving messages from all other nodes containg
 * the nodes id and electing a leader.
 */
void run(addrInfo* ips){
	pthread_t threads[NUM_THREADS];
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_attr_t attr;
	threadData* data;
	serverData* sData;
	
	int rc,i(0);
	void* status;
	// allocate server and client structs
	data = (threadData*)malloc(sizeof(threadData));
	sData = (serverData*)malloc(sizeof(serverData));	

	// give the server and client threads list of ip addresses
	data->ips = ips;	
	sData->totalClients = ips->node_ips.size();
	// create client's files
	std::string names[MAX_NAMES] = {"witch","wizard","elf","dwarf","human","ring","power","magic","evil","good"};	
	setupFiles( data,names );
	// Initialize and set thread joinable
	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
	// create server process to listen
	rc = pthread_create( &threads[i], &attr, leaderProcess, (void*)sData );
	// have server thread act seperately
	pthread_detach( threads[i] );	

	// create client process to send out messages and requests
	pthread_mutex_lock( &mutex );
	i = 1;
	rc = pthread_create( &threads[i], &attr, clientProcess, (void*)data );
	pthread_mutex_unlock( &mutex );

	pthread_attr_destroy( &attr );
	// wait for client thread to finish
	rc = pthread_join( threads[i], &status );
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
	run(info);
}
