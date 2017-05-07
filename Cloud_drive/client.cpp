/*client*/
#include <cstdlib>
//#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
using namespace std;
int readline( int fd, char * vptr, size_t maxlen);


const size_t MAX_BUFFER = 1024*4;

int main(int argc, char * const argv[]) {
    if (argc != 3) {
        cerr << "usage: " << argv[0] << " [hostname] [port number]" << endl;
        exit(EXIT_FAILURE);
    }
    int fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_server < 0) {
        cerr << "unable to create socket" << endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in server_addr;
	//bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    if(connect(fd_server, (sockaddr*)&server_addr, sizeof(server_addr))<0){
		cerr << "connect failed." << endl;
		exit(EXIT_FAILURE);
	}
	FILE* fp = stdin;
	int fd_stdin = fileno(fp);
	int max_fd = max(fd_server, fd_stdin);
	fd_set allset;
	char buffer[MAX_BUFFER];
	while(1){
		FD_ZERO(&allset);
		FD_SET(fd_server,&allset);
		FD_SET(fd_stdin,&allset);
		select(max_fd+1, &allset, NULL, NULL, NULL);
		if(FD_ISSET(fd_server, &allset)){
		}
		if(FD_ISSET(fd_stdin, &allset)){
			memset( buffer, 0, MAX_BUFFER);
			stringstream std_in;
			int read_n = read(fd_stdin, buffer, MAX_BUFFER);
			std_in.str("");
			std_in << buffer;
			string msg_sent, new_name, old_name, command;
			std_in >> command;
			if(read_n <= 0 || command == "EXIT"){
				FD_CLR(fd_stdin,&allset);
				shutdown(fd_server,SHUT_WR); 
			}
			msg_sent = "CHECK\r\n";
			write( fd_server, msg_sent.c_str(), msg_sent.size());
			char check_buf[3];
			read_n = read( fd_server, check_buf, 3);
			//cout << "check_buf:" << check_buf << endl;
			int new_client = atoi( check_buf);
			//cout << "check: " << new_client << endl;
			string ID_str;
			int id;
			//cout << "command: " << command<< endl;
			while(new_client) {
				if(command == "CLIENTID:") {
					std_in >> ID_str;
					//cout << "ID: " << ID_str << endl;
					msg_sent = "ID "+ ID_str + "\r\n";
					write (fd_server, msg_sent.c_str(), msg_sent.size());
		//			write (1, msg_sent.c_str(), msg_sent.size());
					char accept[3];
					read (fd_server, accept, 3);
					//cout << "accept: " << accept << endl;
					if(atoi(accept)) {
						new_client = 0;
						cout << "ID accepted." << endl;
					}
					else {
						cout << "This ID is not usable, please use another one." <<endl;
						command = "";
					}
				}
				else {
					cout << "Please set your ID first." << endl;
					memset (buffer, 0, MAX_BUFFER);
					read ( fd_stdin, buffer, MAX_BUFFER);
					//cout << "buffer: " << buffer << endl;
					std_in.str("");
					std_in << buffer;
					std_in >> command;
					//cout << "command: " << command << endl;
				}
			}
			int file_size;
			char f_buf[MAX_BUFFER];
			bzero(f_buf, MAX_BUFFER);
			if(command == "LIST"){
				msg_sent.clear();
				msg_sent = "LIST";
				write(fd_server, msg_sent.c_str(), msg_sent.size());
				int read_n;
				char size_buf[100];
				char name_buf[100];
				memset( size_buf, 0, 100);
				read_n = readline( fd_server, size_buf, 100);
				int list_size = atoi(size_buf);
				//cout << list_size << endl;
				for(int g = 0;g < list_size;g++){
					memset( name_buf, 0, 100);
					read_n = readline( fd_server, name_buf, 100);
					//cout << "read_n: " << read_n << endl;
					cout << name_buf;
				}
				cout << "LIST succeeded." << endl;
			}
			else if(command == "PUT"){
				std_in >> old_name >> new_name;
				if(old_name == "" || new_name == ""){
					cout << "usage: PUT old_name new_name" << endl;
					continue;
				}
				char ack[10];
				int sfd = open( old_name.c_str(), O_RDONLY, 0644);
				struct stat st;
				stat(old_name.c_str(), &st);
				string f_size_str = to_string( st.st_size);
				msg_sent.clear();
				msg_sent = command + " " + old_name + " " + new_name + " " + f_size_str;
				write( fd_server, msg_sent.c_str(), msg_sent.size());  //command sent
				int ack_n = read( fd_server, ack, MAX_BUFFER);
				if(ack_n < 0){
					cout << "Ack error" << endl;
				}
				while(read_n = read( sfd, f_buf, MAX_BUFFER)){
					if(read_n < 0){
						cout << "Read source file failed." << endl;
						break;
					}
					write( fd_server, f_buf, read_n);	
					memset(f_buf, 0, MAX_BUFFER);
				}
				ack_n = read( fd_server, ack, MAX_BUFFER);
				cout << "PUT " << old_name << " " << new_name << " succeeded." << endl;
				close(sfd);
			}
			else if(command == "GET"){
				std_in >> old_name >> new_name;
		//		cout << old_name << " " << new_name << endl;
				if(old_name == "" || new_name == ""){
					cout << "usage: GET old_name new_name" << endl;
					continue;
				}
				msg_sent.clear();
				msg_sent = "GET " + old_name + " " + new_name + "\r\n";
		//		cout << "msg_sent(command): " << msg_sent << endl;
				write(fd_server, msg_sent.c_str(), msg_sent.size());
				char size_buffer[10];
				read_n = read( fd_server, size_buffer, MAX_BUFFER);
				int f_size = atoi(size_buffer);
				if(f_size == 0){
					cout << "The file does not exist." << endl;
					continue;
				}
		//		cout << "file_size: " << f_size << endl;
				if(read_n < 0){
					cout << "Get filesize failed." << endl;
					break;
				}
				int dfd = open( new_name.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
				if( dfd == -1){
					cout << "Destination file open failed." << endl;
					close(dfd);
					break;
				}
				int rd = 0;
				while(rd < f_size){
					read_n = read(fd_server, f_buf, MAX_BUFFER);
					rd += read_n;
					if(read_n < 0){
						cout << "Read source file failed." << endl;
						close(dfd);
						break;
					}
//					cout << rd <<" "<< f_size << endl;
					write( dfd, f_buf, read_n);
				}
				if(f_size == rd){
					cout << "GET " << old_name << " " << new_name << " succeeded." << endl;
				}
				close(dfd);	
			}
			else {
				continue;
			}
		}
	}
	return 0;
}

int readline ( int fd, char * vptr, size_t maxlen){
	int n, rc;
	char c, *ptr;

	ptr = vptr;
	for ( n = 1;n < maxlen;n++ ){
		again:
			if (( rc = read ( fd, &c, 1)) ==  1){
				*ptr++ = c;
				if(c == '\n'){
					break;
				}
			}
			else if (rc == 0){
				*ptr = 0;
				return (n - 1);
			}
			else {
				if (errno == EINTR){
					goto again;
				}
				return (-1);
			}
	}
	*ptr = 0;
	return (n);
}
