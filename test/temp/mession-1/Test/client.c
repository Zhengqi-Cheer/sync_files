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
#include <unistd.h>
#define SERVER_PORT 66


int main()
{
    int  clientSocket;
    char sendbuf[200];
    char recvbuf[200];
    struct sockaddr_in server_addr;

    int i_datenum;
    //1.
    if( (clientSocket = socket(AF_INET,SOCK_STREAM,0)) < 0 )
    {
        perror("socket error!");
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("192.168.0.129");
    //2.connet

    if(connect(clientSocket,(struct sockaddr *)& server_addr,sizeof(server_addr)) < 0)
    {
        perror("connect error");
        return -1;
    }
    printf("正在连接服务器\r\n");

    while(1)
    {//送
        printf("发送信息：");
        scanf("%s",sendbuf);
        printf("\n");
        send(clientSocket,sendbuf,strlen(sendbuf),0);//发送信息

        if(strcmp("quit",sendbuf) == 0)
            break;

        //收
        printf("主机：");
        recvbuf[0] = '\0';
        i_datenum = recv(clientSocket,recvbuf,200,0);
        recvbuf[i_datenum] = '\0';
        printf("%s",recvbuf);
        printf("\n");
    }
    close(clientSocket);
    return 0;
}
