#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

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
}existingUsers[MAX_ALLOWED_USERS], activeUsers[MAX_ALLOWED_USERS];

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
	// pthread_mutex_lock(&mutex2);
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
	// pthread_mutex_unlock(&mutex2);
	return 0;
}

int userActive(Packet newUser)
{
	int i;
	for(i = 0; i < totalUsers; i++)
	{
		if(((strcmp(activeUsers[i].username, newUser.username)) == 0))
		{
			return 1;
		}
	}
	return 0;
}

void addToActiveUsers(Packet newUser)
{
	strcpy(activeUsers[++totalUsers].username, newUser.username);
	strcpy(activeUsers[++totalUsers].password, newUser.password);
}

void removeActiveUser(Packet newUser)
{
	int i;
	for(i = 0; i < totalUsers; i++)
	{
		if(strcmp(activeUsers[i].username, newUser.username) == 0)
		{
				strcpy(activeUsers[i].username, "\0");
				strcpy(activeUsers[i].password, "\0");
		}
	}
}

int check_credentials(void *sock)
{
	struct client_info clientInfo = *((struct client_info *)sock);
	Packet p;
	int len;
	int i;
	int j;
	int approve = 0;
	memset(&p, 0, sizeof(Packet));
	len = recv(clientInfo.sockno,(void*) &p,sizeof(Packet),0);

	if(len < 0) {
		perror("Message receive error");
		exit(1);
	}

	printf("\npacket\n\tuser: %s\n\tpassword: %s\n\theader: %s\n", p.username, p.password, p.header);

	if((strcmp(p.header, "REGISTER")) == 0)
	{
		approve = 0;
		if(userExists(p) || userActive(p))
		{
			approve = 0;
			int convNumber = htonl(approve);
			printf("\nUser exists\n");
			send(clientInfo.sockno, &convNumber, sizeof(convNumber), 0);
			// pthread_mutex_lock(&mutex3);
			printf("%s disconnected\n",clientInfo.ip);
				for(i = 0; i < n; i++) {
					if(clients[i] == clientInfo.sockno) {
						j = i;
						while(j < n-1) {
							clients[j] = clients[j+1];
							j++;
						}
					}
				}
				n--;
				// removeActiveUser(p);
			// pthread_mutex_unlock(&mutex3);
			return 0;
		}
		else
		{
			approve = 1;
			int convNumber = htonl(approve);

			len = send(clientInfo.sockno, &convNumber, sizeof(convNumber), 0);

			printf("\nUser register succesful: %d , %d\n ", len, errno);
			return 1;
		}
	}
	else
	{
		approve = 0;
		if((strcmp(p.header, "LOGIN")) == 0)
		{
			if(!userExists(p) || userActive(p))
			{

				approve = 0;
				int convNumber = htonl(approve);
				printf("\nUser doesn't exist in database or already active\n");
				send(clientInfo.sockno, &convNumber, sizeof(convNumber), 0);
				pthread_mutex_lock(&mutex3);
				printf("%s disconnected\n",clientInfo.ip);
				for(i = 0; i < n; i++) {
					if(clients[i] == clientInfo.sockno) {
						j = i;
						while(j < n-1) {
							clients[j] = clients[j+1];
							j++;
						}
					}
				}
				n--;
				// removeActiveUser(p);
				pthread_mutex_unlock(&mutex3);
				return 0;
			}
			else
			{
				approve = 2;
				int convNumber = htonl(approve);
				printf("\nUser login succesful\n");
				// addToActiveUsers(p);
				// pthread_mutex_lock(&mutex3);
				send(clientInfo.sockno, &convNumber, sizeof(convNumber), 0);
				// pthread_mutex_unlock(&mutex3);
				return 2;
			}
		}
		else
		{
			printf("This should not have happened\n");
			return 0;
		}
	}

}

void *recvmg(void *sock)
{
	struct client_info clientInfo = *((struct client_info *)sock);
	char msg[500];
	int len;
	int i;
	int j;
	// addToActiveUsers(p);
	while((len = recv(clientInfo.sockno,msg,500,0)) > 0) {
		msg[len] = '\0';
		send_to_all(msg,clientInfo.sockno);
		memset(msg,'\0',sizeof(msg));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n",clientInfo.ip);
	for(i = 0; i < n; i++) {
		if(clients[i] == clientInfo.sockno) {
			j = i;
			while(j < n-1) {
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	n--;
	// removeActiveUser(p);
	pthread_mutex_unlock(&mutex);

	return NULL;
}


int main(int argc,char **argv)
{
	struct sockaddr_in my_addr,client_addr;
	int my_sock;
	int client_sock;
	socklen_t client_addr_size;
	int portno;
	char ipno[20];
	pthread_t recvt;
	// char msg[500];
	// int len;
	struct client_info clientInfo;
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
	client_addr_size = sizeof(client_addr);

	while(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
		printf("Binding unsuccessful\nRetrying\n");
		my_addr.sin_port = htons(++portno);
	}

	if(listen(my_sock,MAX_ALLOWED_USERS) != 0) {
		perror("listening unsuccessful");
		exit(1);
	}

	while(1) {
		if((client_sock = accept(my_sock,(struct sockaddr *)&client_addr,&client_addr_size)) < 0) {
			perror("accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *)&client_addr, ip, INET_ADDRSTRLEN);
		printf("%s connected\n",ip);
		clientInfo.sockno = client_sock;
		strcpy(clientInfo.ip,ip);
		clients[n] = client_sock;
		n++;

		if(check_credentials(&clientInfo))
		{
			pthread_create(&recvt,NULL,recvmg,&clientInfo);
			pthread_mutex_unlock(&mutex);
		}
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}