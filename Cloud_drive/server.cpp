/*server*/
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <string.h>
#include <sstream>
using namespace std;
struct client{
	int socket;
	int ID;
	int online;
	vector<string> list;
};
vector<client> v;
const size_t MAX_BUFFER = 1024*4;
fd_set readfds;

int main(int argc, char * const argv[]) {
	if (argc != 2) {
        cerr << "usage: " << argv[0] << " [port number]" << endl;
        exit(EXIT_FAILURE);
    }
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        cerr << "unable to create socket" << endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in server_addr, c_addr;
	socklen_t c_len = sizeof(c_addr);
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));
    if (bind(listenfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "failed to bind" << endl;
        exit(EXIT_FAILURE);
    }
    listen(listenfd, 32);
	while(1) {
		FD_ZERO(&readfds);
		FD_SET(listenfd, &readfds);
		for(int l = 0;l < v.size();l++) {
			if(v[l].online) {
				FD_SET(v[l].socket, &readfds);
			}
		}
		int result = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if(FD_ISSET(listenfd, &readfds)){
			int c_socket;
			c_socket = accept( listenfd, (sockaddr*)&c_addr,(socklen_t*)&c_len);
			client new_c;
			new_c.socket = c_socket;
			new_c.ID = 0;
			new_c.online = 1;
			v.push_back(new_c);
		}
		else {
			for(int i = 0;i < v.size();i++){
				if(!v[i].online){
					continue;
				}
				if(FD_ISSET(v[i].socket, &readfds)) {
					/*cout << "++++++++++++++++++" << endl;
					for(int k = 0;k < v.size();k++){
						cout << "j: "<< k << " " << v[k].ID << " " << v[k].socket << endl;
					}
					cout << "=================" << endl;*/
					char c_buf[MAX_BUFFER];
					memset(c_buf, 0, MAX_BUFFER);
					int msg_len = read( v[i].socket, &c_buf , MAX_BUFFER);  //read command
					//cout << "Buffer:" << c_buf << endl;
					string command, new_name, old_name;
					if(msg_len < 0){
						cout << "Command read error." << endl;
						continue;
					}
					else if(msg_len == 0){
						v[i].online = 0;
						close(v[i].socket);
						v[i].socket = -1;
						cout << v[i].ID << " is offline" << endl;
						i--;
						continue;
					}
					else{
						stringstream comss;
						comss.clear();
						comss << c_buf;
						comss >> command;
					//	cout << "command: " << command << endl;
						if( command == "CHECK") {
							string new_client = "1\r\n";
							if(v[i].ID != 0){
								new_client = "0\r\n";
							}
							write ( v[i].socket, new_client.c_str(), new_client.size());
						}
						else if(command == "ID") {
							string ID_str;
							comss >> ID_str;
							int ID_int = atoi( ID_str.c_str());
							v[i].ID = ID_int;
							string accept;
							if((v[i].ID < 10000) || (v[i].ID > 99999)) {
//								cout << "five digits" << endl;
								accept = "0\r\n";
								v[i].ID = 0;
								write(v[i].socket, accept.c_str(), accept.size());
							}
							else {
								for(int j = 0;j < v.size();j++){
					//				cout << "++++++++++++++++++" << endl;
					//				cout << "i: "<< i << " " << v[i].ID << endl;
					//				cout << "=================" << endl;
									if(j != i && v[i].ID == v[j].ID){
										if(v[j].online == 1){
//											cout << "already online" << endl;
											accept = "0\r\n";
											v[i].ID = 0;
											write(v[i].socket, accept.c_str(), accept.size());
											break;
										}
										else {
//											cout << "old user" << endl;
											v[j].online = 1;
											v[j].socket = v[i].socket;
											v.erase(v.begin()+i);
											i--;
											accept = "1\r\n";
											write(v[i].socket, accept.c_str(), accept.size());
											write(1, accept.c_str(), accept.size());
											break;
										}
									}
									else if((j == v.size()-1 && v[j].ID != v[i].ID) || i == j) {
										accept = "1\r\n";
										write(v[i].socket, accept.c_str(), accept.size());
									}
									else {
										accept = "0\r\n";
										if(j == v.size()-1) {
											write(v[i].socket, accept.c_str(), accept.size());
										}
									}
								}
							}
						}
						else if( command == "LIST"){
							string list_size = to_string(v[i].list.size());
							list_size += "\r\n";
							write( v[i].socket, list_size.c_str(), list_size.size());
					//		cout << "list.size() " << v[i].list.size() << endl;
							for(int f = 0;f < v[i].list.size();f++){
								string temp;
								temp = v[i].list[f] + "\r\n";
								write( v[i].socket, temp.c_str(), temp.size());
								write( 1, temp.c_str(), temp.size());
							}
						}
						else if( command == "PUT"){
							int f_size;
							comss >> old_name >> new_name >> f_size;
			//				cout << old_name << " " << new_name << endl;
			//				cout << "filesize: " << f_size << endl;
							string ack = "ready\r\n";
							write( v[i].socket, ack.c_str(), ack.size());  //allow client to transfer the file
							ack.clear();
							int read_n;  //count for file size
							string p_name = new_name;
							new_name += to_string(v[i].ID);
							int dfd = open( new_name.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
							if(dfd == -1){
								cout << "Destination file open failed" << endl;
								close(dfd);
								continue;
							}
							char f_buf[MAX_BUFFER];
							while(read_n = read( v[i].socket, f_buf, MAX_BUFFER)){
								if(read_n < 0){
									cout << "Read error." << endl;
									break;
								}
								read_n = write( dfd, f_buf, read_n);
								if(read_n < 0){
									cout << "Write error." << endl;
									break;
								}
								struct stat st;
								stat(new_name.c_str(), &st);
							//	cout << "origin size: " << f_size << " new size: " << st.st_size << endl;
								if(st.st_size == f_size){
									string success = "ya\r\n";
									write( v[i].socket, success.c_str(), success.size());  //allow client to show the success message
									v[i].list.push_back(p_name);
									break;		
								}
							}
							close(dfd);
						}
						else if( command == "GET"){
							string new_name, old_name;
							int f_size;
							char f_buf[MAX_BUFFER];
							comss >> old_name >> new_name;
							string ori_name = old_name;
							old_name += to_string(v[i].ID);
							int exist = 0;
							for(int j = 0;j < v[i].list.size();j++){
								if(ori_name == v[i].list[j]){
									exist = 1;
									break;
								}
							}
							if(!exist) {	
								string f_size_str =  "0\r\n";
								write( v[i].socket, f_size_str.c_str(), f_size_str.size());
								continue;
							}
							//cout << "names: " << old_name << " " << new_name << endl;
							int sfd	= open(old_name.c_str(), O_RDONLY, 0644);
							struct stat st;
							stat( old_name.c_str(), &st);
							int read_n;
							if(sfd == -1){
								cout << "Source file open failed." << endl;
								close(sfd);
								continue;
							}		
							string f_size_str = to_string(st.st_size) + "\r\n";
							write( v[i].socket, f_size_str.c_str(), f_size_str.size());
							while(read_n = read( sfd, f_buf, MAX_BUFFER)){
								sleep(0.02);
								if(read_n < 0){
									cout << "Read source file failed." << endl;
									close(sfd);
									break;
								}
								write( v[i].socket, f_buf, read_n);					
	//							cout << "*******rcv_msg********" << endl;
	//							cout << f_buf << endl;
							}
							close(sfd);
						}
						else {
							cout << "Wrong command." << endl;
						}
					}
				}
			}
		}
	}
    return 0;
}
