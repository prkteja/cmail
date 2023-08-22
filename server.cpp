#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
using namespace std;

struct auth{
	string uname;
	string passwd;
};
// format - “User: user-name Pass: passwd”
bool parse_auth(string s, struct auth& a){// parses s and if valid, adds the credentials to a
	if(s.size() <= 13) return false;
	else if (s.substr(0, 6) != "User: ") return false;
	else{
		string s1 = s.substr(6);// 'User; ' part removed
		int pos = s1.find(' ');
		if(pos == -1)return false;
		string uname;
		if(s1.size() < pos+2){
			return false;
		} else uname = s1.substr(0, pos);
		string s2 = s1.substr(pos+1);// 'Pass: ...'
		if(s2.size() < 7) return false;
		else if(s2.substr(0, 6) != "Pass: ") return false;
		else {
			string passwd = s2.substr(6);
			a.uname = uname;
			a.passwd = passwd;
			return true;
		};
	};
};

void reset_buffer(char buffer[], int n){
	for (int i = 0; i < n; i++) buffer[i] = 0;
};

int sendFile(FILE* fp, char* buf, int s) 
{ 
    int i, len; 
    if (fp == NULL) { 
        cout << "file null\n";
    } 
  
    char ch; 
    for (i = 0; i < s; i++) { 
        ch = fgetc(fp);  
        buf[i] = ch; 
        if (ch == EOF) 
            return 1; 
    } 
    return 0; 
};

int main(int argc, char *argv[])
{
	if(argc != 4) {
		cerr << "Usage: 3 command line arguments: portNum, passwdfile, user-database\n";
		exit(1);
	};
	int portNum = atoi(argv[1]);
	int socket_connected = 0;
	string user_db = argv[3];
	struct stat buf;
	if(stat(user_db.c_str(), &buf) != -1){ // check if the directory exists
		if(!S_ISDIR(buf.st_mode)){
			cerr << "user_db not found\n";
			exit(4);
		};
	} else{
		cerr << "user_db not found\n";
		exit(4);
	};

	int server_fd;
	struct sockaddr_in address;
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		cerr << "socket failed\n";
		exit(2);
	};
	int new_socket;
	int addrlen = sizeof(address);
	char buffer[1024] = {0};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(portNum);
	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
		cerr << "bind failed\n";
		exit(2);
	} else cout << "BindDone: " << portNum << '\n';
	string filepath = argv[2];
	ifstream passwdfile(filepath);
	if(!passwdfile){
		cerr << "passwdfile not found\n";
		exit(3);
	};
	if(listen(server_fd, 3) < 0)
    {
        cerr << "unable to listen\n";
        exit(0);
    } else cout << "ListenDone: " << portNum << '\n';
	while(1){
		if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			cerr << "!accept\n";
			exit(0);
		} else {
			char client_addr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(address.sin_addr), client_addr, INET_ADDRSTRLEN);
			int client_port = ntohs(address.sin_port);
			socket_connected = 1;
			cout << "Client: " << client_addr << ":" << client_port << '\n';
		};
		int valread = read(new_socket, buffer, 1024);
		string data(buffer);
		// cout << data << endl;
		struct auth a;
		if (parse_auth(data, a) == false) {
			cout << "Unknown Command\n";
			close(new_socket);
			socket_connected = 0;
		};
		ifstream f(argv[2]);
		string line;
		int pswd_mismatch = 0;
		if(f.is_open()){
			bool user_found = false;
			while(getline(f, line)){
				if(line.size() > 1){
					int pos = line.find(' ');
					if(a.uname == line.substr(0, pos)){
						if(a.passwd == line.substr(pos+1)){
							string msg = "Welcome " + a.uname +"\n\0";
							cout << msg;
							send(new_socket, msg.c_str(), msg.size(), 0);
						} else {
							// cout << a.passwd << " " << line.substr(pos+1) << endl;
							cout << "Wrong Passwd\n";
							close(new_socket);
							socket_connected = 0;
							pswd_mismatch = 1;
							break;
						};
						user_found = true;
						break;
					};
				};
			};
			if(pswd_mismatch == 1) continue;
			if(user_found == false){
				cout << "Invalid User\n";
				close(new_socket);
				socket_connected = 0;
				continue;
			};
			f.close();
		};
		reset_buffer(buffer, 1024);
		while(socket_connected = 1){
			valread = read(new_socket, buffer, 1024);
			if(valread != 0){
				string str_buffer(buffer);
				reset_buffer(buffer, 1024);
				if(str_buffer == "quit\0"){
					cout << "Bye " << a.uname << '\n';
					close(new_socket);
					socket_connected = 0;
					break;
				} else if (str_buffer == "LIST\0"){
					struct stat stat_buf;
					string path_to_user_dir(argv[3]);
					path_to_user_dir += a.uname;
					// find no. of files in given directory
					int no_of_msgs = 0;
					DIR *dp;
					struct dirent *ep;
					dp = opendir (path_to_user_dir.c_str());
					if (dp != NULL){
						while (ep = readdir(dp)){
							if(ep -> d_type == DT_REG) no_of_msgs++;
						};
					} else {
						cout << a.uname << ": Folder Read Fail\n";
						close(new_socket);
						socket_connected = 0;
						break;
					};
					string msg = a.uname + ": No of messages " + to_string(no_of_msgs) + "\n\0"; 
					cout << msg;
					send(new_socket, msg.c_str(), msg.size(), 0);
				} else if(str_buffer.substr(0, 6) == "RETRV ") {
					string m = str_buffer.substr(6);
					struct stat stat_buf;
					string path_to_user_dir(argv[3]);
					if(path_to_user_dir[path_to_user_dir.size()-1] == '/') path_to_user_dir += a.uname;
					else path_to_user_dir = path_to_user_dir + '/' + a.uname;
					// find a file with name m
					int no_of_msgs = 0;
					DIR *dp;
					struct dirent *ep;
					dp = opendir (path_to_user_dir.c_str());
					bool msg_rd_fail = false;
					bool file_found = false;
					if (dp != NULL){
						while (ep = readdir(dp)){
							if(ep -> d_type == DT_REG){
								string str_d_name(ep -> d_name);
								if(str_d_name.substr(0, m.size()) == m){
									file_found = true;
									//send the name of the file
									send(new_socket, ep -> d_name, strlen(ep -> d_name), 0);
									// send this file
									FILE *fp;
									string path = user_db + "/" + a.uname + "/" + (ep->d_name);
									fp = fopen(path.c_str(), "rb");
									if(fp == NULL){
										cout << "Message read Fail\n";
										msg_rd_fail = true;
										close(new_socket);
										break;
									};
									struct stat stat_buff;
									int rc = stat(path.c_str(), &stat_buff);
									int size = stat_buff.st_size;
									string str_size = to_string(size);
									send(new_socket, str_size.c_str(), 1024, 0);// send the file size to the client
									//read data from fp
									bool transfer_fail = false;
									cout << a.uname << " : Transferring Message " << m <<"\n";
									char buff[1024] = {0};
									int nread;
									while(nread = fread(buff, 1, 1024, fp) != 0){//send the file
										int s = send(new_socket, buff, 1024, 0);
										reset_buffer(buff, 1024);
									};
									if(transfer_fail) break;
								};
							};
						};
						if(msg_rd_fail) break;
						if(!file_found){
							cout << "Message Read Fail\n";
							close(new_socket);
							socket_connected = 0;
							break;
						};
					} else {
						cout << a.uname << ": Folder Read Fail\n";
						close(new_socket);
						socket_connected = 0;
						break;
					};
				} else{
					cout << "Unknown Command\n";
					close(new_socket);
					socket_connected = 0;
					break;
				};
			};
		};
	};
}
