#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#pragma pack(1)
struct PACKET{
	char header[10];
	char username[100];
	char password[100];
};
#pragma pack(0)
typedef struct PACKET Packet;

// char *packet_to_char(Packet processPacket)
// {
// 	char *loginCred = (char * )malloc(sizeof(processPacket));
// 	strcpy(loginCred, "LOGIN::");
// 	strcat(loginCred, processPacket.username);
// 	strcat(loginCred, "::");
// 	strcat(loginCred, processPacket.password);
// 	loginCred[strcspn(loginCred, "\n")] = 0;
// 	return loginCred;
// }

Packet create_login_packet(char *username, char *password)
{
	Packet loginPacket;
	strcpy(loginPacket.header, "LOGIN");
	strcpy(loginPacket.username, username);
	strcpy(loginPacket.password, password);
	return loginPacket;
}

Packet create_register_packet(char *username, char *password)
{
	Packet loginPacket;
	strcpy(loginPacket.header, "REGISTER");
	strcpy(loginPacket.username, username);
	strcpy(loginPacket.password, password);
	return loginPacket;
}

void *recvmg(void *sock)
{
	int their_sock = *((int *)sock);
	char msg[500];
	int len;
	while((len = recv(their_sock,msg,500,0)) > 0) {
		msg[len] = '\0';
		fputs(msg,stdout);
		memset(msg,'\0',sizeof(msg));
	}
}

int main(int argc, char **argv)
{
	struct sockaddr_in their_addr;
	int my_sock;
	int their_sock;
	int their_addr_size;
	int portno;
	char ipno[20];
	pthread_t sendt,recvt;
	char msg[500];
	char username[100];
	char password[100];
	char res[600];
	char ip[INET_ADDRSTRLEN];
	int len;

	Packet loginPacket;

	if(argc > 5) {
		printf("too many arguments");
		exit(1);
	}

	strcpy(ipno, argv[1]);
	portno = atoi(argv[2]);
	// strcpy(loginPacket.username,argv[1]);
	// strcpy(loginPacket.password,argv[2]);


	my_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(their_addr.sin_zero,'\0',sizeof(their_addr.sin_zero));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(portno);
	their_addr.sin_addr.s_addr = inet_addr(ipno);

	while(connect(my_sock,(struct sockaddr *)&their_addr,sizeof(their_addr)) < 0) {
		printf("Connection not esatablished\nRetrying\n");
		their_addr.sin_port = htons(++portno);
		// exit(1);
	}

	inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
	printf("connected to %s, start chatting\n",ip);
	pthread_create(&recvt,NULL,recvmg,&my_sock);

	int optionSelect = 0;
	int approve = 0;
	int return_status;

	while(!approve)
	{
		printf("Select option \
\n\t1)REGISTER \
\n\t2)LOGIN\n");
		scanf("%d", &optionSelect);

		switch(optionSelect)
		{
			case 1:
				printf("Username: ");
				scanf("%s", &username);
				// fgets(username, 100, stdin);
				username[strcspn(username, "\n")] = 0; // Remove trailing newline

				printf("Password: ");
				scanf("%s", &password);
				// fgets(password, 100, stdin);
				password[strcspn(password, "\n")] = 0;

				// printf("\nuser: %s \t pass: %s", username, password);

				Packet registerCredentialsPacket = create_register_packet(username, password);
				printf("\npacket\n\tuser: %s\n\tpassword: %s\n\theader: %s\n", registerCredentialsPacket.username, registerCredentialsPacket.password, registerCredentialsPacket.header);

				len = send(my_sock, (void*)&registerCredentialsPacket,sizeof(Packet), 0);
				if(len < 0) {
					perror("message not sent");
					exit(1);
				}
				return_status = recv(my_sock, &approve, sizeof(int), 0);
				approve = ntohl(approve);
				if(approve)
				{
					printf("APPROVE: %d\n", approve);
					// break;
				}
				else
				{
					printf("Not APPROVED\n\tUSER EXISTS\n");
					exit(-1);
				}

				break;

			case 2:
				printf("Username: ");
				scanf("%s", &username);
				// fgets(username, 100, stdin);
				username[strcspn(username, "\n")] = 0; // Remove trailing newline

				printf("Password: ");
				scanf("%s", &password);
				// fgets(password, 100, stdin);
				password[strcspn(password, "\n")] = 0;

				// printf("\nuser: %s \t pass: %s", username, password);

				Packet loginCredentialsPacket = create_login_packet(username, password);
				printf("\npacket\n\tuser: %s\n\tpassword: %s\n\theader: %s\n", loginCredentialsPacket.username, loginCredentialsPacket.password, loginCredentialsPacket.header);

				len = send(my_sock, (void*)&loginCredentialsPacket,sizeof(Packet), 0);
				if(len < 0) {
					perror("message not sent\n");
					exit(1);
				}
				return_status = read(my_sock, &approve, sizeof(int));
				approve = ntohl(approve);

				if(approve)
				{
					printf("APPROVE: %d\n", approve);
					// break;
				}
				else
				{
					printf("Not APPROVED\n\tUSER NOT IN DATABASE OR ALREADY ACTIVE\n");
					exit(-1);
				}
				break;
			default:
				exit(-1);
		}
	}
	// fgets(msg, 500, stdin);
	// while(fgets(msg,500,stdin) > 0) {
	while(scanf("%s", &msg) > 0) {
		strcpy(res, username);
		strcat(res,":");
		strcat(res,msg);
		strcat(res, "\n");
		len = write(my_sock,res,strlen(res));
		if(len < 0) {
			perror("message not sent");
			exit(1);
		}
		memset(msg,'\0',sizeof(msg));
		memset(res,'\0',sizeof(res));
	}
	pthread_join(recvt,NULL);
	close(my_sock);

	return 0;
}