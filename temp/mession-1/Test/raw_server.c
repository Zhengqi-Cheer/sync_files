#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<linux/if_ether.h>
#include<errno.h>

#define SRC "192.168.0.128" //源IP
#define SPORT  66



struct iphead{            //IP首部
    unsigned char ip_hl:4, ip_version:4;
    unsigned char ip_tos;
    unsigned short int ip_len;
    unsigned short int ip_id;
    unsigned short int ip_off;
    unsigned char ip_ttl;
    unsigned char ip_pro;
    unsigned short int ip_sum;
    unsigned int ip_src;
    unsigned int ip_dst;
};

struct tcphead{      //TCP首部
    unsigned short tcp_sport;
    unsigned short tcp_dport;
    unsigned int tcp_seq;
    unsigned int tcp_ack;
    unsigned char tcp_off:4, tcp_len:4;
    unsigned char tcp_flag;
    unsigned short tcp_win;
    unsigned short tcp_sum;
    unsigned short tcp_urp;
};

struct psdhead{ //TCP伪首部
    unsigned int saddr; //源地址
    unsigned int daddr; //目的地址
    unsigned char mbz;//置空
    unsigned char ptcl; //协议类型
    unsigned short tcpl; //TCP长度
};

unsigned short cksum(unsigned char packet[], int len){   //校验函数
    unsigned long sum = 0;
    unsigned short * temp;
    unsigned short answer;
    temp = (unsigned short *)packet;
	int n = sizeof(packet+len);

    for(int i=0 ; temp < (n >> 1);i += 1)	
        sum += *(temp+i);
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
    /*
    长度可能奇数，此处需完善
    */
}

int linsen_raw(int sockfd,struct sockaddr_in seraddr);
int make_SYN_ACK(int sendsockfd,unsigned int op_seq,unsigned short op_ip,unsigned short op_port);

char SYN_okflage = 1;
int main(){
	int recv_sockfd,send_sockfd;
	struct sockaddr_in  seraddr;
	//1、创建套接字，收发套接字
	int opt = 1;
	setsockopt(recv_sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof( opt ));
	//收套接字
	if((recv_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) <0)
	{
		perror("recv_sockfd error");
		return -1;		
	}
	else printf("创建原始套接字成功：recv_sockfd\n");
	
	//发套接字
	
	bzero(&seraddr, sizeof(seraddr));
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(SPORT);
	seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	
	if(bind(recv_sockfd,(struct sockaddr *)&seraddr,sizeof(seraddr)) < 0)
	{
		perror("bind error!");
		return -1;
	}	
	
	printf("服务端ip:192.168.0.128\n");
	printf("服务端port:66\n");
	printf("等待客户端连接\n");
	
	while(SYN_okflage)
	{		
		linsen_raw(recv_sockfd,seraddr);
		
	}
		while(1);
	close (recv_sockfd);
	
}

int linsen_raw(int sockfd,struct sockaddr_in seraddr)
{	
	

//1、接收ip报文
	unsigned char rec[1024] ;
	struct sockaddr_in addr_client;
	unsigned int addlen = sizeof(addr_client);
	
	
	int n = recvfrom(sockfd, rec, 1024, 0, (struct sockaddr *)&addr_client, &addlen);  //接收ip报文
	
	
//2、分析ip报文
/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Identification        |Flags|      Fragment Offset    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Time to Live |    Protocol   |         Header Checksum       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source Address                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination Address                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
	
	

	unsigned char iphead_len = rec[0];
	iphead_len &= 0x0f;
	iphead_len *= 4;

	unsigned short ip_len = ntohs((unsigned short *)(rec+2));
	unsigned short tcp_len = ip_len - iphead_len;
	
	unsigned short d_port = ntohl(*((unsigned short*)(rec+iphead_len+2)));
	
	//if(d_port == 66)
	//{
		printf("ipcksum: %d\n", cksum(rec, iphead_len));  //校验IP首部
		
		//（1）取标志位，判断是不是SYN报
		unsigned char flag = rec[13+iphead_len]; //获取标志字段
		flag = 0x02 & flag ;
		if(flag != 0x02)
			printf("不是SYN包");
		else 
		{
			SYN_okflage &= 0;
			printf("flag:%02x\n", flag);
			printf("收到SYN包\n");
			printf("收到的SYN报文receive %d bytes:\n", n);  //将接受的IP数据报输出
			for(int i=0; i<n; i++)
			{
				if(i % 16 == 0)
					printf("\n");
				printf("%02x ", rec[i]);
			}
			printf("\n");
			
			//memset(buffer, 0, sizeof(buffer));
			unsigned int op_seq; 
			//获取相关IP，port
			//printf("%s",inet_ntoa(ntohl(*((unsigned short*)(rec+12)))));
			//unsigned short op_ip = ntohl(*((unsigned short*)(rec+12)));
			
			 unsigned int op_port = ntohl(*((unsigned int *)(rec+iphead_len))) >>16 ;
			
			//printf("IP:%08x 请求连接\n",op_port);
			
			op_seq = ntohl(*((unsigned int*)(rec+iphead_len+4)));
			printf("op_seq:%d\n", op_seq);
			//unsigned short  _ip = addr_client.sin_addr,s_addr;
			printf("客户端IP是：%x\n",addr_client.sin_addr.s_addr);
			printf("客户端port是：%d\n",op_port);
			unsigned char packet[sizeof(struct iphead) + sizeof(struct tcphead)] = {0};
			struct iphead* ip;
			struct tcphead* tcp;
			//ip = (struct iphead*)packet;
			//tcp = (struct tcphead*)(packet+sizeof(struct iphead));
			tcp = (struct tcphead*)(packet);
			memset(packet, 0, sizeof(packet));  //重新赋值为0

			/*以下分别设置IP，和TCP的首部，然后发送ACK报文段*/
			/*设置IP首部*/
			/*
			ip->ip_hl = 5;
			ip->ip_version = 4;
			ip->ip_tos = 0;
			ip->ip_len = htons(sizeof(struct iphead) + sizeof(struct tcphead));
			ip->ip_id = htons(7777); // random()
			ip->ip_off = htons(0x4000);
			ip->ip_ttl = 64;
			ip->ip_pro = IPPROTO_TCP;
			ip->ip_src = inet_addr(SRC);
			ip->ip_dst = addr_client.sin_addr.s_addr;//inet_addr(op_ip);
			ip->ip_sum = cksum(packet, 20);  //计算IP首部的校验和，必须在其他字段都赋值后再赋值该字段，赋值前为0
					
			/*设置TCP首部*/
			int my_seq  = 22;
			tcp->tcp_sport = htons(SPORT);
			tcp->tcp_dport =htons(op_port);
			tcp->tcp_seq = htonl(my_seq);
			printf("op_seq:%d\n", op_seq);
			tcp->tcp_ack = htonl(op_seq+1);
			tcp->tcp_len = 5;  //发送SYN报文段时，设置TCP首部长度为20字节
			tcp->tcp_off = 0;
			tcp->tcp_flag = 0x12;  //SYN_ACK置位
			tcp->tcp_win = htons(1000);
			tcp->tcp_urp = htons(0);
					
			/*设置tcp伪首部，用于计算TCP报文段校验和*/
			struct psdhead psd;
			psd.saddr = inet_addr(SRC); //源IP地址
			psd.daddr = addr_client.sin_addr.s_addr;//inet_addr(op_ip); //目的IP地址
			psd.mbz = 0;
			psd.ptcl = 6;  
			psd.tcpl = htons(20);
			unsigned char buffer[1000]; //用于存储TCP伪首部和TCP报文，计算校验码
			memcpy(buffer, &psd, sizeof(psd));
			memcpy(buffer+sizeof(psd), tcp, sizeof(struct tcphead));
			tcp->tcp_sum = cksum(buffer, sizeof(psd) + sizeof(struct tcphead));  //计算检验码

			/*发送SYN报文段*/
			//int seraddr_len = sizeof(seraddr);
			int send = sendto(sockfd, packet, sizeof(packet), 0,(struct sockaddr *)&addr_client,addlen);
			if(send < 0){
				perror("send failed   sendcode=%d\n");
				return -1;
			 }
			 else {
				 printf("send OK sendcode=%d\n", send);
				//make_SYN_ACK(sockfd, op_seq,op_ip,op_port,seraddr);
				for(int i=0; i<send; i++)
				{
					if(i % 16 == 0)
						printf("\n");
					printf("%02x ", packet[i]);
				}
				printf("\n");
			 }
	//}

	}
	
}
/*
int make_SYN_ACK(int sendsockfd,unsigned int op_seq,unsigned short op_ip,unsigned short op_port,struct sockaddr_in seraddr)
{
	
	
	
}
*/


