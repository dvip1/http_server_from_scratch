#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define MYPORT 3500

int main(){
    char buffer[1000];

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd ==-1)printf("error in creating socket");
    struct sockaddr_in addr; 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MYPORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bind_conn = bind(sock_fd, (struct sockaddr*) &addr, sizeof(addr));
    if(bind_conn !=0) {printf("error binding connection");return -1;}

    int listen_conn = listen(sock_fd, 5);
    if(listen_conn !=0) {printf("error listening to connection");return -1;}

    while (1){
     int accept_conn = accept(sock_fd, NULL, NULL);
        if(accept_conn<0) {printf("error accepting connection");return -1;}
        int received= recv(accept_conn, buffer, sizeof(buffer)-1, 0);
        // receive
        if(received>0) buffer[received]= '\0';
        printf("client said %s", buffer);
        //send

        char buffer1[1000]; 
        snprintf(buffer1, sizeof(buffer1),"%s%s","your response is ", buffer);
        send(accept_conn, buffer1,strlen(buffer1), 0);
        if (close(accept_conn) == -1) {
          perror("close failed");
        }

    }
}
