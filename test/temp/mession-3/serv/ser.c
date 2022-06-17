#include "serv.h"

//主程序
int main(void)
{//socket
    int s_fd = init_sock();
    struct sockaddr_in cli_addr;
    int c_len = sizeof(cli_addr);
    memset(&cli_addr, 0, sizeof(cli_addr));

    //listen-accept
    while (1) {
        printf("linstening .................\n");
        int c_fd = accept(s_fd,(struct sockaddr*)&cli_addr,&c_len);
        if(c_fd == -1){
            perror("accept error");
            continue;
        }

        //begin
        char recv_buff[SIZE] = {'\0'};
        int  n_recv = 0;
        while(n_recv = recv(c_fd,recv_buff,sizeof(recv_buff),MSG_WAITALL)){
            if(n_recv== -1){
                perror("recv error");
                close(c_fd);
                printf("c_fd:%d\n",c_fd);
                continue;
            }else{
                if (!strncmp("up", recv_buff+sizeof(int), 2)) {
                    add_file(c_fd,recv_buff);	
                }else if(!strncmp("rm",recv_buff,2)){
                    rm_file(recv_buff+2);
                }else if(!strncmp("mk",recv_buff,2)){
                    mk_dir(recv_buff+2);
                }else if(!strncmp("rf",recv_buff,2)){
                    rm_dir(recv_buff+2);
                }else if(!strncmp("ln",recv_buff,2)){
                    mk_linkfile(recv_buff+2);
                }else if(!strncmp("mv",recv_buff,2)){
                    mv_dir(OUT,recv_buff);
                }else if(!strncmp("hd",recv_buff,2)){
                    hard_file(recv_buff+2);
                }
            }

            memset(recv_buff,0,sizeof(recv_buff));
        }
        
        close(c_fd);
        printf("\n客户端已断开 !\n");
    }
}

