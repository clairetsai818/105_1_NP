#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

const size_t MAX_LINE = 256;

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
	char buf[1024];
	while(1){
		FD_ZERO(&allset);
		FD_SET(fd_server,&allset);
		FD_SET(fd_stdin,&allset);
		select(max_fd+1, &allset, NULL, NULL, NULL);
		bzero(buf, 1024);
		if(FD_ISSET(fd_server, &allset)){
			int read_n = read(fd_server, buf, 1023);
			if(read_n <= 0){
				break;
			}
			cout << buf;
		}
		if(FD_ISSET(fd_stdin, &allset)){
			int read_n = read(fd_stdin, buf, 1023);
			if(read_n <= 0 || strncmp(buf,"exit",4)==0){
				if(read_n <= 0)
				//	cout << "Server disconnected." <<endl;
				FD_CLR(fd_stdin,&allset);
				shutdown(fd_server,SHUT_WR); 
			}
			else{
				write(fd_server, buf, 1024);
			}
		}
	}
	return 0;
}
