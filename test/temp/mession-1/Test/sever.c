/*
 * 功能：TCP服务器
 * 作者：zhengquan
 *
 */
 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 66

int main()
{
	
	printf("功能：TCP服务器 \r\n作者：zhengquan\r\n");	
	
	//创建socket函数返回的文件描述符
	int serverSocket ;
	
	//声明两个套接字sockaddr_in结构体变量，分别表示客户端和服务器
	struct sockaddr_in server_addr; //服务器
	struct sockaddr_in client_addr; //客户端
	
	int addr_len = sizeof (client_addr) ;
	int client;
	char buffer[200];
	int iDataNum;
//1、创建socket  int socket (int domain ,int type ,int protcol)
	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("socket error");
		return -1;
		
	}
	bzero(&server_addr, sizeof(server_addr));
	
	//初始化服务器端的套接字，并用htons和htonl将端口和地址转成网络字节序
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//避免服务器异常退出端口被占用的问题
	int opt = 1;
	setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof( opt ));
//2、绑定socket  int bind (int sockfd, const struct sockaddr* my_addr, socklen_t addrlen)
	if(bind(serverSocket,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
	{
		perror("bind error!");
		return -1;
	}	

//3、监听socket  int listen ( int sockfd, int backlog)
	if(listen(serverSocket,5) < 0)
	{
		perror("listen error");
		return -1;
	}
	
	while(1)
	{
		printf("正在监听端口：%d\n",SERVER_PORT);
	//4、接受连接   int int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
		client = accept(serverSocket,(struct sockaddr*)&client_addr,(socklen_t*)&addr_len);
		if(client < 0)
		{
			perror("accept error!");
			return -1;
		}

		printf ("等待中。。。。\n");
		printf("IP is %s\n",inet_ntoa(client_addr.sin_addr));
		printf("port is %d\n",htons(client_addr.sin_port));
		printf("正在读取客户端发送的信息......");
	//6、通信
		while(1)
		{	
	
			//服务器接收客户端发的信息
			printf("客户端：");
			buffer[0]='\0';
			iDataNum = recv(client ,buffer,1024,0);
			if(iDataNum < 0)
			{
				perror("recv NULL");
				continue;
			}
			buffer[iDataNum] = '\0';
			if(strcmp(buffer,"quit") == 0) //如果客户端退出
			break;
			printf("%s\n", buffer);
			
			
			//服务器发送信息到客户端
			printf("发送信息：");
			scanf("%s",buffer);	
			send(client,buffer,strlen(buffer),0);
			if(strcmp(buffer,"quit") ==0 )
			{
				printf("已经将port为%d的客户端提出\r\n",htons(client_addr.sin_port));
				break;
			}
		}	
	}	

//7、关闭连接，int close(int fd)

		close(serverSocket);
		return 0;
}







