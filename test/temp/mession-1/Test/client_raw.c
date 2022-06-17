#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<linux/if_ether.h>
#include <errno.h>


#define DST "192.168.0.128"  //目的IP
#define SRC "192.168.0.128" //源IP
#define SPORT 55  //源端口号
#define DPORT 66    //目的端口
#define SIZE 1024

struct iphead{            //IP首部
    unsigned char       ip_hl:4, ip_version:4;
    unsigned char       ip_tos;
    unsigned short int  ip_len;
    unsigned short int  ip_id;
    unsigned short int  ip_off;
    unsigned char       ip_ttl;
    unsigned char       ip_pro;
    unsigned short int  ip_sum;
    unsigned int        ip_src;
    unsigned int        ip_dst;
};

struct tcphead{      //TCP首部
    unsigned short  tcp_sport;
    unsigned short  tcp_dport;
    unsigned int    tcp_seq;
    unsigned int    tcp_ack;
    unsigned char   tcp_off:4, tcp_len:4;
    unsigned char   tcp_flag;
    unsigned short  tcp_win;
    unsigned short  tcp_sum;
    unsigned short  tcp_urp;
};

struct psdhead{ //TCP伪首部
    unsigned int    saddr; //源地址
    unsigned int    daddr; //目的地址
    unsigned char   mbz;//置空
    unsigned char   ptcl; //协议类型
    unsigned short  tcpl; //TCP长度
};

unsigned short cksum(unsigned char packet[], int len){   //校验函数
    unsigned long   sum = 0;
    unsigned short  *temp;
    unsigned short  answer;
    temp = (unsigned short *)packet;
    for ( ; temp < packet+len; temp += 1) {
        sum += *temp;
    }
        
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
    /*
       长度可能奇数，此处需完善
     */
}

int conn(int sendsockfd, int recsockfd, struct sockaddr_in seraddr){  // 三次握手
    unsigned char packet[sizeof(struct iphead) + sizeof(struct tcphead)];
    struct iphead* ip;
    struct tcphead* tcp;
    ip = (struct iphead*)packet;
    tcp = (struct tcphead*)(packet+sizeof(struct iphead));
    memset(packet, 0, sizeof(packet));
    /*以下分别设置IP，和TCP的首部，然后发送SYN报文段*/
    /*设置IP首部*/
    ip -> ip_hl = 5;
    ip -> ip_version = 4;
    ip -> ip_tos = 0;
    ip -> ip_len = htons(sizeof(struct iphead) + sizeof(struct tcphead));
    ip -> ip_id = htons(13542); // random()
    ip -> ip_off = htons(0x4000);
    ip -> ip_ttl = 64;
    ip -> ip_pro = IPPROTO_TCP;
    ip -> ip_src = inet_addr(SRC);
    ip -> ip_dst = inet_addr(DST);
    ip -> ip_sum = cksum(packet, 20);  //计算IP首部的校验和，必须在其他字段都赋值后再赋值该字段，赋值前为0

    /*设置TCP首部*/
    int my_seq = 10; //TCP序号
    tcp -> tcp_sport = htons(SPORT);
    tcp -> tcp_dport = htons(DPORT);
    tcp -> tcp_seq = htonl(my_seq);
    tcp -> tcp_ack = htons(0);
    tcp -> tcp_len = 5;  //发送SYN报文段时，设置TCP首部长度为20字节
    tcp -> tcp_off = 0;
    tcp -> tcp_flag = 0x02;  //SYN置位
    tcp -> tcp_win = htons(29200);
    tcp -> tcp_urp = htons(0);
    /*设置tcp伪首部，用于计算TCP报文段校验和*/
    struct psdhead psd;
    psd.saddr = inet_addr(SRC); //源IP地址
    psd.daddr = inet_addr(DST); //目的IP地址
    psd.mbz = 0;
    psd.ptcl = 6;  
    psd.tcpl = htons(20);

    unsigned char buffer[1000]; //用于存储TCP伪首部和TCP报文，计算校验码
    memcpy(buffer, &psd, sizeof(psd));
    memcpy(buffer+sizeof(psd), tcp, sizeof(struct tcphead));
    tcp -> tcp_sum = cksum(buffer, sizeof(psd) + sizeof(struct tcphead));  //计算检验码

    /*发送SYN报文段*/
    int send = sendto(sendsockfd, packet, htons(ip -> ip_len), 0,(struct sockaddr *)&seraddr, sizeof(seraddr));
    if (send < 0) {
        printf("send failed   sendcode=%d\n", send);
        return -1;
    }

    unsigned char rec[SIZE];
    int n = recvfrom(recsockfd, rec, SIZE, 0, NULL, NULL);  //接收SYN和ACK报文
    printf("收到的SYN_ACK报文receive %d bytes:\n", n);  //将接受的IP数据报输出
    for(int i=0; i<n; i++){
        if (i % 16 == 0) {
            printf("\n");
        }

        printf("%02x ", rec[i]);
    }

    printf("\n");
    /*校验接收到的IP数据报，重新计算校验和，结果应为0*/
    unsigned char ipheadlen = rec[0]; //取出IP数据包的长度
    ipheadlen = (ipheadlen & 0x0f);   //IP首部长度字段只占该字节后四位
    ipheadlen *= 4; //四个字节为单位
    unsigned short iplength = ntohs(*((unsigned short *)(rec+2))); //获取IP数据报长度
    unsigned short tcplength = iplength - ipheadlen;  //计算TCP数据报长度
    printf("ipcksum: %d\n", cksum(rec, ipheadlen));  //校验IP首部


    /*以下校验TCP报文，同样将伪首部和TCP报文放入buffer中*/

    memset(buffer, 0, sizeof(buffer));
    for(int i=0; i<8; i++)
        buffer[i] = rec[i + 12];  //获取源IP和目的IP
    buffer[8] = 0;  //伪首部的字段，可查阅资料
    buffer[9] = rec[9];  //IP首部“上层协议”字段，即IPPROTO_TCP
    buffer[10] = 0; //第10,11字节存储TCP报文长度，此处只考虑报文长度只用一个字节时，不会溢出，根据网络字节顺序存储
    unsigned char tcpheadlen = rec[32];  //获取TCP报文长度                  
    tcpheadlen = tcpheadlen >> 4;  //因为TCP报文长度只占该字节的高四位，需要取出该四位的值
    tcpheadlen *= 4;   //以四个字节为单位
    printf("tcpheadlen:%d\n", tcpheadlen);
    buffer[11] = tcpheadlen;  //将TCP长度存入
    for(int i=0; i<tcplength; i++)   //buffer中加入TCP报文
        buffer[i+12] = rec[i+ipheadlen]; 
    printf("tcpcksum:%d\n", cksum(buffer, 12+tcplength));  //得到校验和

    /*检验收到的是否是SYN+ACK包，是否与上一个SYN请求包对应*/
    unsigned int ack = ntohl(*((signed int*)(rec+ipheadlen+8))); //获取TCP首部的确认号
    printf("ACK:%d\n", ack);

    if (ack != my_seq + 1) { //判断是否是放一个SYN报的回应
        printf("该报不是对上一个SYN请求包的回应");
    } else {
        unsigned char flag = rec[13+ipheadlen]; //获取标志字段
        printf("flag:%02x\n", flag);
        flag = (flag & 0x12);  //只需要ACK和SYN标志的值
        if (flag != 0x12) {  //判断是否为SYN+ACK包
            printf("不是ACK+SYN包\n");   
        } else {
            printf("收到ACK+SYN包\n");
            /*接下来发送ACK确认包*/
            unsigned int op_seq; //获取接收到的ACK+SYN包的序列号
            op_seq = ntohl(*((unsigned int*)(rec+ipheadlen+4)));
            printf("op_seq:%d\n", op_seq);
            memset(packet, 0, sizeof(packet));  //重新赋值为0
            /*以下分别设置IP，和TCP的首部，然后发送ACK报文段*/
            /*设置IP首部*/
            ip -> ip_hl = 5;
            ip -> ip_version = 4;
            ip -> ip_tos = 0;
            ip -> ip_len = htons(sizeof(struct iphead) + sizeof(struct tcphead));
            ip -> ip_id = htons(13543); // random()
            ip -> ip_off = htons(0x4000);
            ip -> ip_ttl = 64;
            ip -> ip_pro = IPPROTO_TCP;
            ip -> ip_src = inet_addr(SRC);
            ip -> ip_dst = inet_addr(DST);
            ip -> ip_sum = cksum(packet, 20);  //计算IP首部的校验和，必须在其他字段都赋值后再赋值该字段，赋值前为0

            /*设置TCP首部*/
            my_seq ++;
            tcp -> tcp_sport = htons(SPORT);
            tcp -> tcp_dport = htons(DPORT);
            tcp -> tcp_seq = htonl(my_seq);
            printf("op_seq:%d\n", op_seq);
            tcp -> tcp_ack = ntohl(op_seq+1);
            tcp -> tcp_len = 5;  //发送SYN报文段时，设置TCP首部长度为20字节
            tcp -> tcp_off = 0;
            tcp -> tcp_flag = 0x10;  //SYN置位
            tcp -> tcp_win = htons(1000);
            tcp -> tcp_urp = htons(0);

            /*设置tcp伪首部，用于计算TCP报文段校验和*/
            //        struct psdhead psd;
            psd.saddr = inet_addr(SRC); //源IP地址
            psd.daddr = inet_addr(DST); //目的IP地址
            psd.mbz = 0;
            psd.ptcl = 6;  
            psd.tcpl = htons(20);
            unsigned char buffer[1000]; //用于存储TCP伪首部和TCP报文，计算校验码
            memcpy(buffer, &psd, sizeof(psd));
            memcpy(buffer + sizeof(psd), tcp, sizeof(struct tcphead));
            tcp -> tcp_sum = cksum(buffer, sizeof(psd) + sizeof(struct tcphead));  //计算检验码

            /*发送SYN报文段*/
            int send = sendto(sendsockfd, packet, htons(ip -> ip_len), 0,(struct sockaddr *)&seraddr, sizeof(seraddr));
            if (send < 0) { 
                printf("send failed   sendcode=%d\n", send);
                return -1;
            }

            printf("已发送ACK报文，已创建TCP连接\n");
            n = recvfrom(recsockfd, rec, SIZE, 0, NULL, NULL);  //接收IP数据报
            printf("receive %d bytes:\n", n);  //将接受的IP数据报输出
            for (int i=0; i<n; i++) {
                if (i % 16 == 0) {
                    printf("\n");
                }

                printf("%d ", rec[i]);
            } 
            printf("\n");
        }
    }
}

int main(){
    int     sendsockfd, recsockfd;
    struct  sockaddr_in seraddr;
    int     opt = 1;
    (void)setsockopt(sendsockfd, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof( opt ));
    sendsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sendsockfd < 0) {
        perror("create sendsocket failed\n");
        return -1;
    } 

    (void)setsockopt(recsockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof( opt ));
    recsockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);  //接收套接字
    if (recsockfd < 0) {
        perror("create recsockfd failed\n");
        return -1;
    }

    int one = 1;
    if (setsockopt(sendsockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {   //定义套接字不添加IP首部，代码中手工添加
        perror("setsockopt failed!\n");
        return -1;
    }

    seraddr.sin_family = AF_INET;  
    seraddr.sin_addr.s_addr = inet_addr(DST); //设置接收方IP
    conn(sendsockfd, recsockfd, seraddr);  //模拟创建TCP连接
    return 0;
}
