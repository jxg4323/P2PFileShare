void clientHandler(threadData* cData){
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* ip;
	const char* msg = DEFAULT_MSG;		
	// Setup Connection to Leader Node
	ip = leaderIP.c_str();
	std::cout << "Leader IP: " << ip << std::endl;
//	while(1){
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			std::cerr << "Socket Creation Error" << std::endl;
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(COMM_PORT);
		if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0){
			std::cerr << "Invalid address/ Address not supported  CH" << ip << std::endl;
		}
		// continue trying to connect to ip
		while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){}
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
void setupFiles(threadData* data, std::string names[]){
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
		info->message += ":"+info->fileNames[i];
	}
	struct sockaddr_in address;
	int sock = 0, valread, server_fd, new_socket, opt = 1;
	int addrlen = sizeof(address);
	struct sockaddr_in serv_addr;
	const char* m = info->message.c_str();
	const char* ip;
	for( int i = 0; i < (info->ips->node_ips.size()); i++){
		ip = info->ips->node_ips.at(i).c_str();	
		info->good_id = false;
		// -------- Client Process --------- //
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			std::cerr << "Socket Creation Error" << std::endl;
		}
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(COMM_PORT);
		if(inet_pton(AF_INET, ip,&serv_addr.sin_addr)<=0){
			std::cerr << "Invalid address/ Address not support"<< std::endl;
		}
		// continue telling server id until a good one is reached.
		while( info->good_id == false ){
			// wait and try to connect with node	
			while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){}
			std::cout << "Message Sent" << std::endl;
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
