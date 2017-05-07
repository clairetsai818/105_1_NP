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
#include <sstream>
using namespace std;
struct client{
	string name;
	int socket;
	string ip;
	string port;
	sockaddr_in client_addr;
	int client_addr_len;
};
vector<client> v;
fd_set readfds;
int name_anon(string s_name);
int name_dup(string s_name, int in);
int name_len(string s_name);
int tell_rec(string s_rec);
int tell_send(string s_send);
int tell_r_anon(string s_rec);

const size_t MAX_LINE = 256;

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
    sockaddr_in server_addr;//, client_addr;
//    socklen_t client_len = sizeof(client_addr);
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));
    if (bind(listenfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "failed to bind" << endl;
        exit(EXIT_FAILURE);
    }
    listen(listenfd, 32);
	while(1){
		FD_ZERO(&readfds);
		FD_SET(listenfd, &readfds);
		for(int i = 0;i < v.size();i++)
			FD_SET(v[i].socket, &readfds);
		int result = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
		if(FD_ISSET(listenfd, &readfds)){
			string hi = "[Server] Hello, anonymous! From:";
			string hi_ex = "[Server] Someone is coming!\r\n";
			client new_client;
			new_client.name = "anonymous";
			new_client.client_addr_len = sizeof(new_client.client_addr);
			new_client.socket = accept(listenfd, (sockaddr*)&(new_client.client_addr),(socklen_t*)&new_client.client_addr_len);
			char t_ip[32];
			inet_ntop(AF_INET, &new_client.client_addr.sin_addr, t_ip, sizeof(t_ip));
			new_client.ip = string(t_ip);
			int p_tmp;
			p_tmp = ntohs(new_client.client_addr.sin_port);
			char buf[10];
			sprintf(buf, "%d", p_tmp);
			new_client.port = string(buf);
			hi = hi + new_client.ip + "/" + new_client.port + "\r\n";
			int write_len = hi.length() + new_client.ip.length() + new_client.port.length() + 3;
			write(new_client.socket,hi.c_str(), write_len);
			v.push_back(new_client);
			for(int l = 0;l < v.size();l++){
				if(v[l].socket != new_client.socket)
					write(v[l].socket, hi_ex.c_str(), hi_ex.length());
			}
		}
		else{
			for(int i = 0;i < v.size();i++){
				char c;
				int msg_len;
				char buf[1024];
				if(FD_ISSET(v[i].socket, &readfds)){
					string tmp_str;
					msg_len = read(v[i].socket, &buf, 1023);
					if(msg_len == 0){
						tmp_str = "[Server] " + v[i].name + " is offline\r\n";
						for(int a = 0;a < v.size();a++){
							if(a != i)
								write(v[a].socket, tmp_str.c_str(), tmp_str.length());
						}
						//FD_CLR(v[i].socket, &readfds);
						close(v[i].socket);
						v.erase (v.begin()+i);
						i--;
						continue;
					}
					for(int y =0;y < 1024;y++){
						if(buf[y] == '\n')
							buf[y] = '\0';
						else if(buf[y] == '\r')
							buf[y] = '\0';
					}
					string buffer = string(buf);
					stringstream input;
					input << buffer;
					string command;
					input >> command;		
					if(command == "who"){
						for(int m = 0;m < v.size();m++){
							if(m == i)
								tmp_str = "[Server] "+ v[m].name + " " + v[m].ip + "/" + v[m].port + " ->me\r\n";
							else
								tmp_str = "[Server] "+ v[m].name + " " + v[m].ip + "/" + v[m].port + "\r\n";
							write(v[i].socket, tmp_str.c_str(), tmp_str.length());
						}
					}
					else if(command == "name"){
						string new_name, old_name;
						input >> new_name;
						old_name = v[i].name;
						int an = name_anon(new_name);
						int lenn = name_len(new_name);
						int dup = name_dup(new_name, i);
						//cout << "name: " << buffer << " len:" << buffer.length() << endl; 
						//cout << "an: " << an << " lenn:" << lenn << " dup:" << dup << endl;
						if(!an && !dup && !lenn){
							for(int m = 0;m < v.size();m++){
								if(i == m){
									tmp_str = "[Server] You're now known as "+ new_name + ".\r\n";
									v[m].name = new_name;
								}
								else
									tmp_str = "[Server] "+ old_name + " is now known as " + new_name + ".\r\n";
								write(v[m].socket, tmp_str.c_str(), tmp_str.length());	
							}
						}
						else if(an){
							tmp_str = "[Server] ERROR: Username cannot be anonymous.\r\n";
							write(v[i].socket, tmp_str.c_str(), tmp_str.length());	
						}
						else if(dup){
							tmp_str = "[Server] ERROR: " + new_name +" has been used by others.\r\n";
							write(v[i].socket, tmp_str.c_str(), tmp_str.length());	
						}
						else{
							tmp_str = "[Server] ERROR: Username can only consists of 2~12 English letters.\r\n";
							write(v[i].socket, tmp_str.c_str(), tmp_str.length());	
						}

					}
					else if(command == "tell"){
						string s_rec, s_send, n_rec, msg;
						input >> n_rec;
						getline(input, msg);
						//cout << "to: " << n_rec << n_rec.length() << " msg: " << msg << endl;
						int rec_socket, rec, send;
						rec_socket = tell_rec(n_rec);
						rec =  tell_r_anon(n_rec);
						send = tell_send(v[i].name);
						//cout << "rec_socket: " << rec_socket << " rec: " << rec << " send: " << send << endl;
						if(!rec && !send && (rec_socket != 0)){
							s_rec = "[Server] " + v[i].name + " tell you" + msg + "\r\n";
							s_send = "[Server] SUCCESS: Your message has been sent.\r\n";
							write(rec_socket, s_rec.c_str(), s_rec.length());	
						}
						else if(send){
							s_send = "[Server] ERROR: You are anonymous.\r\n";
						}
						else if(rec){
							s_send = "[Server] ERROR: The client to which you sent is anonymous.\r\n";
						}
						else{
							s_send = "[Server] ERROR: The receiver dones't exist.\r\n";
						}
						write(v[i].socket, s_send.c_str(), s_send.length());	
					}
					else if(command == "yell"){
						string yell_msg;
						getline(input, yell_msg);
						tmp_str = "[Server] " + v[i].name + " yell " + yell_msg + "\r\n";
						for(int w = 0;w < v.size();w++){
							write(v[w].socket, tmp_str.c_str(), tmp_str.length());	
						}
					}
					else{
						if(command == "");
						else	
							tmp_str = "[Server] ERROR: Error command.\r\n";
						write(v[i].socket, tmp_str.c_str(), tmp_str.length());	
					}
				}
			}
		}
	}
    close(listenfd);
    return 0;
}
int name_anon(string s_name){
	return (s_name == "anonymous");
}
int name_dup(string s_name, int in){
	for(int m = 0;m < v.size();m++){
		if(s_name == v[m].name && m != in)
			return 1;
	}
   return 0;	
}
int name_len(string s_name){
	int n_len = s_name.length();
	if(n_len < 2||n_len > 12){
		return 1;
	}
	for(int u = 0;u < s_name.length();u++){
		if(!isalpha(s_name[u]))
			return 1;
	}
	return 0;
}
int tell_r_anon(string s_rec){
	return !(strcmp(s_rec.c_str(), "anonymous"));
}
int tell_rec(string s_rec){
	for(int m = 0;m < v.size();m++){
		if(s_rec == v[m].name)
			return v[m].socket;
	}
	return 0;
}
int tell_send(string s_send){
	return !(strcmp(s_send.c_str(), "anonymous"));
}
