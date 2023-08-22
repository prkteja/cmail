#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <vector>
using namespace std;

void reset_buffer(char buffer[], int n){
	for (int i = 0; i < n; i++) buffer[i] = 0;
};

inline bool isInteger(const std::string & s)// true if string is an integer
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

   char * p;
   strtol(s.c_str(), &p, 10);

   return (*p == 0);
};

int recvFile(FILE* fp, char* buf, int s) 
{ 
    int i; 
    char ch; 
    for (i = 0; i < s; i++) { 
        ch = buf[i];  
        if (ch == EOF) 
            return 1; 
        else
            fwrite(buf, 1, s, fp); 
    } 
    return 0; 
} ;

bool is_list_of_nos(char *a){// true if it is a list of numbers
    char *token = strtok(a, ",");
    while(token != NULL){
        string s(token);
        if(!isInteger(s)) return false;
        token = strtok(NULL, ",");
    };
    return true;
};

int main(int argc, char *argv[])
{
    char lst[sizeof(argv[4])];
    for(int i = 0; i < sizeof(argv[4]); i++) lst[i] = argv[4][i];
	if(argc != 6) {
		cerr << "Usage: 5 command line arguments - serverIPAddr:port, user-name, passwd, list-of-messages, local-folder\n";
		exit(1);
	};
    if(! is_list_of_nos(lst)){
        cerr << "Usage: 5 command line arguments - serverIPAddr:port, user-name, passwd, list-of-messages, local-folder\n";
		exit(3);
    };
    string local_folder = argv[5];
	struct stat buf;
	if(stat(local_folder.c_str(), &buf) != -1){ // check if the directory exists
		if(!S_ISDIR(buf.st_mode)){
			// it is not a directory
            string command = "mkdir -p " + local_folder;
            int create_dir = system(command.c_str());
            if(create_dir == -1){
                cerr << "Unable to create/access the local folder\n";
                exit(4);
            };
		} else {
            // the dir exists
            string command = "rm -rf " + local_folder;
            int remove_dir = system(command.c_str());// delete the existing dir
            if(remove_dir == -1){
                cerr << "Unable to create/access the local folder\n";
                exit(4);
            };
            command = "mkdir -p " + local_folder;// make new dir
            int create_dir = system(command.c_str());
            if(create_dir == -1){
                cerr << "Unable to create/access the local folder\n";
                exit(4);
            };
        }
	} else{//dir does not exist
		string command = "mkdir -p " + local_folder;
        int create_dir = system(command.c_str());
        if(create_dir == -1){
            cerr << "Unable to create/access the local folder\n";
            exit(4);
        };
	};
    vector<string> lst_of_msgs;
    char *token = strtok(argv[4], ",");
    while(token != NULL){
        string str_token(token);
        lst_of_msgs.push_back(str_token);
        token = strtok(NULL, ",");
    };
	struct sockaddr_in address;
	int sock = 0; int valread;
	struct sockaddr_in serv_addr;
	char buffer[1024] = {0};
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		cerr << "socket creation error \n";
		exit(-1);
	};
	char *serverIPaddr = strtok(argv[1], ":");
	int port = atoi(strtok(NULL, ":"));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if(inet_pton(AF_INET, serverIPaddr, &serv_addr.sin_addr) <= 0){
		cerr << "Invalid address\n";
		exit(-1);
	};
	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		cerr << "connection failed\n";
		exit(2);
	} else cout << "ConnectDone: " << serverIPaddr << ":" << port << '\n';
	string uname = argv[2];
	string passwd = argv[3];
	string credentials = "User: " +uname + " Pass: " + passwd + "\0";
	send(sock, credentials.c_str(), credentials.size(), 0);
	valread = read(sock, buffer, 1024);
	cout << buffer;
	reset_buffer(buffer, 1024);
	// send(sock, "LIST\0", strlen("LIST\0"), 0);
	// valread = read(sock, buffer, 1024);
	// cout << buffer;
    // reset_buffer(buffer, 1024);
    for(int i = 0; i < lst_of_msgs.size(); i++){
        //retrive each of the messages
        string msg = "RETRV " + lst_of_msgs[i] + "\0";
        send(sock, msg.c_str(), msg.size(), 0);
        int val = read(sock, buffer, 1024);//get the file name
        string file_name = buffer;
        FILE *fp;
        if(local_folder[local_folder.size() - 1] == '/') fp = fopen((local_folder + file_name).c_str(), "ab");
        else fp = fopen((local_folder + "/" + file_name).c_str(), "ab");
        reset_buffer(buffer, 1024);
        val = read(sock, buffer, 1024);//get the file size
        int file_size = atoi(buffer);
        reset_buffer(buffer, 1024);
        int bytesReceived = 0;
        while(1){
            char buff[1024] = {0};
            bytesReceived = recv(sock, buff, 1024, 0);
            if(file_size >= 1024) fwrite(buff, 1, 1024, fp);
            else fwrite(buff, 1, file_size, fp);
            file_size -= 1024;
            if(file_size < 0){
                break;
                cout << "Downloaded Message " << lst_of_msgs[i] << "\n";
            };
        };
    };
	send(sock, "quit\0", strlen("quit\0"), 0);
	return 0;
}
