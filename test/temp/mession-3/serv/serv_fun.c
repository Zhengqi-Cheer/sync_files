#include "serv.h"

int init_sock(void)
{
    //socket
    int _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) { 
        perror("sock error");
        return -1;
    }

    //bind
    struct sockaddr_in serv_addr;
    (void)memset(&serv_addr,0,sizeof(serv_addr));

    serv_addr.sin_port = htons(8877);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    (void)setsockopt(_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if (bind(_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror ("bind error");
        close(_fd);
        return -1;
    }

    //listen
    if (listen(_fd,4) < 0) {
        perror("listen error");
        close(_fd);
        return -1;
    }

    return _fd;
}

/*
 * add file 
 */
void add_file(int c_fd, char* file_name)//创建文件
{
    unsigned int filesize = 0;
    filesize = *(int*)file_name;
    char recvBuf[SIZE];
    memset(recvBuf, 0, sizeof(recvBuf));
    int  n_read = 0;
    FILE *fdfile = fopen(file_name+sizeof(int)+strlen("up"), "w+");
    if (NULL == fdfile) {
        perror("open");
        printf("NO: %s\n",file_name);
        return;
    }

    while (1) {
        filesize -= n_read;
        if (filesize < sizeof(recvBuf)) {
            n_read = recv(c_fd, recvBuf, filesize, MSG_WAITALL);
        } else {
            n_read = recv(c_fd, recvBuf, sizeof(recvBuf),0);
        }

        if (n_read == -1) {
            perror("fd read error");
            fclose(fdfile);
        }

        int n_write = fwrite(recvBuf, sizeof(char), n_read, fdfile);
        if (-1 == n_write) {
            perror("write");
            fclose(fdfile);
            return;
        }

        if (filesize < SIZE) { 
            break;
        }

        (void)memset(recvBuf, 0, sizeof(recvBuf));
    }

    printf("mkfile[%s]successfully\n",file_name+sizeof(int)+2);
    fclose(fdfile);
}

/*
 * delete file 
 */
void rm_file(char *file_name)
{
    int rmfile = remove(file_name);
    if (rmfile == -1) {
        perror("rm file error");
        close(rmfile);
    } else {
        printf("remove [%s] sucsse!\n", file_name);
    }
}

/***********
 *mkdir directory
 ***********/
void mk_dir(char *dir_path)
{
    int n_dir = mkdir(dir_path, 0755);
    if (n_dir == -1) {
        perror("error mkdir");
        printf("error mkdir:%s\n ", dir_path);
    } else {
        printf("mkdir [%s] succes !\n", dir_path);
    }
}

/******
 *delete directory 
 ******/
void rm_dir(char *dir_path)
{
    int n_dir = rmdir(dir_path);
    if (n_dir == -1) { 
        printf("error rmdir:%s\n", dir_path);
        perror("rmdir error");
    } else {
        printf("rmdir [%s] succes ! \n", dir_path);
    }
}

/*******
 * mkdir link file
 数据格式："文件路径+'\0'+"link路径"
 *******/
void mk_linkfile(char *file_name)
{  
    char Sour_file_path[SIZE >> 2] = {'\0'};//符号文件链接对象路径
    strcpy(Sour_file_path, file_name+strlen(file_name)+1);
    int _link = symlink(Sour_file_path, file_name);
    if (_link == -1) {
        perror("link file error ");
        printf("error-->:%s\n", file_name);
        return ;
    }

    printf("linkfile %s ok !\n", file_name);
}

/********
 *移动文件夹
 *********/
void mv_dir(int num,char *dir_path)
{
    int sys_return = system(dir_path);
    if (sys_return == -1) {
        perror("no do system");
        return;
    } else if (sys_return == 127) {
        perror("shell failed");
        return;
    }

    printf("mv [%s] successly !\n", dir_path);
}

/********
  创建一个硬链接文件
  参数格式：o_path+'\0'+s_path
 *******/
void hard_file(char *S_path)
{
    char *O_path = S_path+strlen(S_path)+1;
    int  n_ = link(S_path,O_path);
    if (n_ == -1) {
        printf("hard file error:%s-->%s\n", O_path,S_path);
        perror("hard file error");
        return;
    } else {
        printf("hard_file [%s] successly !\n", O_path);
    }
}
