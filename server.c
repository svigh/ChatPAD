#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_ALLOWED_USERS 100

#pragma pack(1)
struct client_info {
	int sockno;
	char ip[INET_ADDRSTRLEN];
};
#pragma pack(0)

int clients[100];
int n = 0;
int totalUsers = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

#pragma pack(1)
struct PACKET{
	char header[10];
	char username[100];
	char password[100];
};
#pragma pack(0)

struct USER{
	char username[100];
	char password[100];
}existingUsers[MAX_ALLOWED_USERS];

typedef struct PACKET Packet;
typedef struct USER User;

void send_to_all(char *msg,int curr)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++) {
		if(clients[i] != curr) {
			if(send(clients[i],msg,strlen(msg),0) < 0) {
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}

int userExists(Packet newUser)
{
	int i;
	pthread_mutex_lock(&mutex2);
	for(i = 0; i < totalUsers; i++)
	{
		if(((strcmp(existingUsers[i].username, newUser.username)) == 0) && ((strcmp(existingUsers[i].password, newUser.password)) == 0) )
		{
			return 1;
		}
	}
	if((strcmp(newUser.header, "REGISTER")) == 0)
	{
		strcpy(existingUsers[i].username, newUser.username);
		strcpy(existingUsers[i].password, newUser.password);
		totalUsers++;
	}
	pthread_mutex_unlock(&mutex2);
	return 0;
}

int *check_credentials(void *sock)
{
	struct client_info cl = *((struct client_info *)sock);
	Packet p;
	char msg[500];
	int len;
	int i;
	int j;
	int approve = 0;
	memset(&p, 0, sizeof(Packet));
	len = recv(cl.sockno,(void*) &p,sizeof(Packet),0);

	if(len < 0) {
		perror("Message receive error");
		exit(1);
	}

	printf("\npacket\n\tuser: %s\n\tpassword: %s\n\theader: %s\n", p.username, p.password, p.header);

	// while(!approve)
		if((strcmp(p.header, "REGISTER")) == 0)
		{
			approve = 0;
			if(userExists(p))
			{
				approve = 0;
				int convNumber = htonl(approve);
				printf("\nUser exists\n");
				write(cl.sockno, &convNumber, sizeof(convNumber));
				pthread_mutex_lock(&mutex3);
				printf("%s disconnected\n",cl.ip);
					for(i = 0; i < n; i++) {
						if(clients[i] == cl.sockno) {
							j = i;
							while(j < n-1) {
								clients[j] = clients[j+1];
								j++;
							}
						}
					}
					n--;
				pthread_mutex_unlock(&mutex3);
				return 0;
			}
			else
			{
				approve = 1;
				int convNumber = htonl(approve);
				printf("\nUser register succesful\n");
				// pthread_mutex_lock(&mutex3);
				write(cl.sockno, &convNumber, sizeof(convNumber));
				// pthread_mutex_unlock(&mutex3);
				return 1;
			}
		}
		else
		{
			approve = 0;
			if((strcmp(p.header, "LOGIN")) == 0)
			{
				if(!userExists(p))
				{

					approve = 0;
					int convNumber = htonl(approve);
					printf("\nUser doesn't exist in database\n");
					write(cl.sockno, &convNumber, sizeof(convNumber));
					pthread_mutex_lock(&mutex3);
					printf("%s disconnected\n",cl.ip);
					for(i = 0; i < n; i++) {
						if(clients[i] == cl.sockno) {
							j = i;
							while(j < n-1) {
								clients[j] = clients[j+1];
								j++;
							}
						}
					}
					n--;
					pthread_mutex_unlock(&mutex3);
					return 0;
				}
				else
				{
					approve = 2;
					int convNumber = htonl(approve);
					printf("\nUser login succesful\n");
					pthread_mutex_lock(&mutex3);
					write(cl.sockno, &convNumber, sizeof(convNumber));
					pthread_mutex_unlock(&mutex3);
					return 2;
				}
			}
			else
			{
				printf("This should not have happened\n");
			}
		}

}

void *recvmg(void *sock)
{
	struct client_info cl = *((struct client_info *)sock);
	char msg[500];
	int len;
	int i;
	int j;
	while((len = recv(cl.sockno,msg,500,0)) > 0) {
		msg[len] = '\0';
		send_to_all(msg,cl.sockno);
		memset(msg,'\0',sizeof(msg));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n",cl.ip);
	for(i = 0; i < n; i++) {
		if(clients[i] == cl.sockno) {
			j = i;
			while(j < n-1) {
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	n--;
	pthread_mutex_unlock(&mutex);
}


int main(int argc,char **argv)
{
	struct sockaddr_in my_addr,their_addr;
	int my_sock;
	int their_sock;
	socklen_t their_addr_size;
	int portno;
	char ipno[20];
	pthread_t sendt,recvt;
	char msg[500];
	int len;
	struct client_info cl;
	char ip[INET_ADDRSTRLEN];

	if(argc > 3) {
		printf("too many arguments");
		exit(1);
	}
	strcpy(ipno, argv[1]);
	portno = atoi(argv[2]);
	my_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portno);
	my_addr.sin_addr.s_addr = inet_addr(ipno);
	their_addr_size = sizeof(their_addr);

	while(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
		printf("Binding unsuccessful\nRetrying\n");
		my_addr.sin_port = htons(++portno);
		// exit(1);
	}

	if(listen(my_sock,5) != 0) {
		perror("listening unsuccessful");
		exit(1);
	}

	while(1) {
		if((their_sock = accept(my_sock,(struct sockaddr *)&their_addr,&their_addr_size)) < 0) {
			perror("accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
		printf("%s connected\n",ip);
		cl.sockno = their_sock;
		strcpy(cl.ip,ip);
		clients[n] = their_sock;
		n++;

		if(check_credentials(&cl))
		{
			pthread_create(&recvt,NULL,recvmg,&cl);
			pthread_mutex_unlock(&mutex);
		}
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}