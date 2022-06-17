#include "dameon.h"

struct event_list *_event_P = NULL; //事件队列头指针
struct hard_link_list *hard_file_head = NULL;

/*
 * 创建TCP连接
 */
int init_tcp(void)
{
    //socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket error");
        return -1;
    }

    //connect	
    struct sockaddr_in serv_addr;
    (void)memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
 
    int opt = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect error");
        close(fd);
        return -1;
    }

    return fd;	
}

/*
 *同步增加文件函数
 *flag: 1 --> 第一次上传
 *      0 --> 后续不需要检测时
 */
void mk_file(int flag, char* file_name, int sock) 
{   
    int filesize = if_hard_first(flag, sock, file_name);
    if (filesize == -1) { //硬链接同inode文件已经上传过
        return; 
    }

    char sendBuf[SIZE] = {'\0'};
    (void)memcpy(sendBuf, &filesize, sizeof(int));
    (void)memcpy(sendBuf+sizeof(int), "up", strlen("up"));
    (void)memcpy(sendBuf+sizeof(int)+strlen("up"), file_name, strlen(file_name));
    int c_fd = sock;
    write(c_fd, sendBuf, sizeof(sendBuf));
    (void)memset(sendBuf, 0, sizeof(sendBuf));
    int fdfile = open(file_name, O_RDONLY);
    if (-1 == fdfile) {
        perror("open");
        printf("open error[%s]\n", file_name);
        return;	
    }

    int n_read = 0;
    while (n_read = read(fdfile, sendBuf, sizeof(sendBuf))) {
        int n_write = write(c_fd, sendBuf, n_read);
        if (-1 == n_write) {
            perror("write");
            printf("[%s]\n", file_name);
            close(fdfile);
            return;
        }

        (void)memset(sendBuf, 0, sizeof(sendBuf));
    }

    close(fdfile);
    (void)memset(sendBuf, 0, sizeof(sendBuf));
    strcpy(sendBuf, "上传文件-->");
    strcat(sendBuf, file_name);
    journal_write(sendBuf); 
}

/*
 *同步删除文件函数
 */
void rm_file(char* file_name, int sock)
{
    int  c_fd = sock;
    char sendbuf[SIZE] = {"rm"};
    strcat(sendbuf, file_name);
    int n_write = write(c_fd, sendbuf, sizeof(sendbuf));
    if (n_write == -1) {
        perror("rm_file send error");
        return;
    }

    (void)memset(sendbuf, 0, sizeof(sendbuf));
    strcpy(sendbuf, "删除文件-->");
    strcat(sendbuf, file_name);
    journal_write(sendbuf);
}

/**
 *同步增加目录函数
 **/
void mk_dir (char *dir_path, int sock)
{
    int  c_fd = sock;
    char sendbuf[SIZE] = {"mk"};
    strcat(sendbuf, dir_path);
    int n_write = write(c_fd, sendbuf, sizeof(sendbuf));
    if (n_write == -1) {
        perror("directory synchronization  error : mk ");
        printf("[%s]\n",dir_path);
        return;
    }

    (void)memset(sendbuf, 0, sizeof(sendbuf));
    strcpy(sendbuf, "创建目录-->");
    strcat(sendbuf, dir_path);
    journal_write(sendbuf);
}

/*****
 *  void rm_dir(char *dir_path);
 *  同步删除目录函数
 *****/
void rm_dir(char *dir_path, int sock)
{
    int  c_fd = sock;
    char send_buff[SIZE] = {"rf"};
    strcat(send_buff, dir_path);
    int n_write = write(c_fd, send_buff, sizeof(send_buff));
    if (n_write == -1) {
        perror(" directory synchronization  error : rm ");
        return;
    }

    (void)memset(send_buff, 0, sizeof(send_buff));
    strcpy(send_buff, "删除目录-->");
    strcat(send_buff, dir_path);
    journal_write(send_buff);
}
/**
  功能：第一次上传判断硬链接源文件是否是第一次同步
  返回值：filesize --> yes
          -1       --> no
 **/
int if_hard_first(int flag, int sock, char* file_name)
{
    //读取文件信息
    struct stat file;
    if (lstat(file_name, &file)) {
        perror(" stat error");
        return -1;
    }

    int  filesize = file.st_size;
    int  hard_num = file.st_nlink;
    int  inode_id = file.st_ino;
    if (flag == 0) {
        return filesize;
    } else if (hard_num > 1) {
        if (hard_file_head == NULL) {
            hard_list(inode_id, file_name);
            return filesize;
        } else {
            struct hard_link_list *temp_head = hard_file_head;
            while (temp_head) {
                if (temp_head -> inode == inode_id) {
                    hard_link(sock, file_name, temp_head -> path);
                    return -1;
                }

                temp_head = temp_head -> next;
            }

            hard_list(inode_id, file_name);
            return filesize;
        }

    } else {
        return filesize;
    }
}

/*
 * 第一次上传储存硬链接
 */
void hard_list(int inode, char *path)
{
    struct hard_link_list *newnode = (struct hard_link_list *)malloc(sizeof(struct hard_link_list));
    newnode -> path = (char*)malloc(strlen(path)+1);
    strncpy(newnode -> path, path, strlen(path));
    newnode -> inode = inode;
    newnode -> next = NULL;

    if (hard_file_head == NULL) {
        hard_file_head = newnode;
    } else {
        struct hard_link_list *temp = hard_file_head;
        while (temp -> next) {
            temp = temp -> next;
        }

        temp -> next = newnode;
    }
}
/*
 *寻找源文件
 */
char* seek_h_Source_file(char* seek_dir, int inode, char *file_name)
{   
    DIR *s_dir = opendir(seek_dir);
    if (s_dir == NULL) { 
        perror("seek -> opendir error");
        printf("open dir error:%s\n", seek_dir);
        return NULL;
    }

    char s_path[SIZE] = {'\0'};
    (void)memcpy(s_path, seek_dir, strlen(seek_dir));
    strcat(s_path, "/");
    struct dirent *directory = NULL;
    char *sp = NULL;
    while (directory = readdir(s_dir)) {
        if (!strcmp(directory -> d_name, ".")) {
            continue;
        } else if (!strcmp(directory -> d_name, "..")) {
            continue;
        }

        strncat(s_path, directory -> d_name, strlen(directory -> d_name));

        //目录文件
        if (directory -> d_type == 4) {
            sp = seek_h_Source_file(s_path, inode, file_name);
            if (sp) {
                break;
            }
            //文件
        } else if (directory -> d_type == 8) {
            struct stat s_stat;
            (void)memset(&s_stat, 0, sizeof(s_stat));
            if (lstat(s_path, &s_stat)) {
                perror("seek file error:");
                printf("s_path error:%s\n", s_path);
                return NULL;
            }

            if ((s_stat.st_ino == inode) && strcmp(s_path, file_name)) {
                printf("s_path:%s\n", s_path);
                char *_sp = (char*)malloc(strlen(s_path)+1);
                strcpy(_sp, s_path);
                return _sp;
            }
        }

        char *p = strrchr(s_path, '/');   
        *(p+1) = '\0';//修正地址
    }

    closedir(s_dir);
    return sp;
}


/*
 *同步硬链接命令
 */
void hard_link(int sock, char *O_path, char *S_path)
{   
    char send_buff[SIZE] = {'\0'};
    (void)memcpy(send_buff, "hd", 2);
    (void)memcpy(send_buff+2, S_path, strlen(S_path));
    (void)memcpy(send_buff+strlen(S_path)+2+1, O_path, strlen(O_path));

    int n_write = write(sock, send_buff, sizeof(send_buff));
    if (n_write == -1) {
        perror("hard_link error ");
        return;
    }
}

/*
 *发送链接文件命令
 */
void mk_linkfile(char *dir_path, int sock)
{   
    int  c_fd = sock;
    char send_buff[SIZE] = {"ln"};
    strcat(send_buff, dir_path);
    int  n_read = readlink(dir_path, send_buff+strlen(send_buff)+1, sizeof(send_buff));
    int  n_write = write(c_fd, send_buff, sizeof(send_buff));
    if (n_write == -1) {
        perror("linkfile synchronization  error : ln ");
        return;
    }

    (void)memset(send_buff, 0, sizeof(send_buff));
    strcpy(send_buff, "创建链接文件-->");
    strcat(send_buff, dir_path);
    journal_write(send_buff);

}

/*
 *判断是否为链接文件
 *硬：inode number
 *软：S_IFLNK
 */
int  _if_linkfile(char *filename)
{   
    struct stat stat_buf;
    struct stat *p = &stat_buf;
    (void)memset(&stat_buf, 0, sizeof(stat_buf));
    if (-1 == lstat(filename, p)) {
        perror("lstat error");
    }
    
    if (S_ISLNK(p -> st_mode)) {
        return S_IFLNK;
    } else if (p -> st_nlink > 1) {
        return p -> st_ino;
    } else {
        return 0;
    }
}

/*
 *程序运行的第一件事：同步指定路径的文件夹
 */
void first_event (int i_fd, char *dir_path, int num, struct dir_link *head, int sock)
{
    //同步test文件夹
    if (num) { 
        mk_dir(dir_path, sock);
        int wd = inotify_add_watch(i_fd, dir_path, IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY);
        add_link(head, wd, dir_path);
    }
    //同步test文件夹内的文件
    char path[SIZE] = {'\0'};
    strcat(path, dir_path);
    DIR  *pdir = opendir(dir_path);
    strcat(path, "/");   //path --> <xxx/path/>
    struct dirent *directory = NULL;
    while (directory = readdir(pdir)) {
        if (!strcmp(directory -> d_name, ".")) {
            continue;
        }
        if (!strcmp(directory -> d_name, "..")) {
            continue;
        }
        strcat(path, directory -> d_name);
        //其他文件
        //目录文件
        if (directory -> d_type == 4) {
            int wd = inotify_add_watch(i_fd, path, IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_DELETE_SELF);//将目录加入监控队列
            //-->添加到链表
            add_link(head, wd, path);
            mk_dir(path, sock);
            first_event(i_fd, path, 0, head, sock);
        } else if (directory -> d_type == 8) {
            //普通文jian 
            mk_file(1, path, sock);
        } else if (directory -> d_type == 10) {
            //符号文件 
            mk_linkfile(path, sock);
        } else {
            printf("%s  -->", (char*)(directory -> d_name));
            printf("%d  -->", (directory -> d_type));
        }
        char *p = strrchr(path, '/'); //目录路径修正->返回上一级目录
        *(p+1) = '\0';
    }

    closedir(pdir);
}

/*
 *续链表节点
 */
void add_link(struct dir_link *head, int wd, char *add_path)
{
    //新链表节点
    struct dir_link *newnode = (struct dir_link *)malloc(sizeof(struct dir_link));
    newnode -> wd = wd;
    newnode -> dir_path = (char*)malloc(strlen(add_path)+1);
    strcpy(newnode -> dir_path, add_path);
    newnode -> next = NULL;

    if (head == NULL) {
        head -> next = newnode;
    } else {
        while (head -> next != NULL) {
            head = head -> next; 
        }   

        head -> next = newnode;
    }
}

/*
 *删除链表节点i
 *返回值： -1 --> 失败
 *         wd --> 成功
 */
int delete_link(struct dir_link *head, char *dir_path)
{   
    int wd = -1;
    struct dir_link *next_node = head -> next;
    while (next_node) {
        if (next_node -> dir_path == dir_path) {
            wd = next_node -> wd;
            free (next_node -> dir_path);
            head -> next = next_node -> next;
            free(next_node);
            return wd;
        } else {
            head = next_node;
            next_node = next_node -> next;
        }
    }

    return -1;
}

/*
 *添加/更新事件到事件队列中
 */
void add_list(int flag,struct event_list *head ,char *cmd, char *do_path)
{
    //改文件已经有事件发生了,
    struct event_list *temp_head = head;
    while (temp_head) { 
        int t = !strcmp(temp_head -> do_path, do_path);
        if (t) {
            //特：file创建->删除 = 无
            if (!strcmp(temp_head -> cmd, "mk")) { 
                if (!strcmp(cmd, "rf")) {
                    delete_list(temp_head, do_path);
                }
            }

            //同路径且与上一次的事件相同
            else if (!strcmp(temp_head -> cmd, cmd)) {
                return;
            } else {
                strcpy(temp_head -> cmd, cmd); 
            }
           
            return;
        }

        temp_head = temp_head -> next;
    }

    struct event_list *newnode = (struct event_list *)malloc(sizeof(struct event_list));
    newnode -> cmd = (char*)malloc(strlen(cmd)+1);
    strcpy(newnode -> cmd, cmd); 
    newnode -> do_path = (char*)malloc(strlen(do_path)+1);
    strcpy(newnode -> do_path,do_path);
    newnode -> inode = flag;
    newnode -> next = NULL;
    temp_head = head;
    if (temp_head == NULL) {
        _event_P = newnode;
    } else {
        while (temp_head -> next) {
            temp_head = temp_head -> next;
        }

        temp_head -> next = newnode;
    } 
}

/******
 *从事件队列中删除某个事件
 ******/
void delete_list(struct event_list *head, char *do_path)
{
    struct event_list *next_node = head -> next;
    while (next_node) {
        if (!strcmp(next_node -> do_path, do_path)) {
            free (next_node -> cmd);
            free (next_node -> do_path);
            head -> next = next_node -> next;
            free(next_node);
            break;
        } else {
            head = next_node;
            next_node = next_node -> next;
        }
    } 
}

/******
 *监控事件处理函数
 *将事件加入/更新到事件队列，添加/删除监控目录列表
 ******/
void do_event(int fd, struct inotify_event *event, struct dir_link *head, struct event_list *event_head)
{
    char temp[SIZE] = {'\0'};
    struct dir_link *temp_head = head;
    //找路径
    while (temp_head) {
        if (event -> wd == temp_head -> wd) {
            strcpy(temp, temp_head -> dir_path);
            strcat(temp, "/");
            strcat(temp, event -> name);
            break;
        } else {
            temp_head = temp_head -> next;
        }
    }

    //是目录？
    if (event -> mask & IN_ISDIR) {
        if (event -> mask & (IN_CREATE | IN_MOVED_TO)) {
            int wd = inotify_add_watch( fd, temp, IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_DELETE_SELF);
            add_link(head, wd, temp);
            if (event -> mask & IN_MOVED_TO) {
                add_list(0, event_head, "mo", temp);
            } else {
                add_list(0, event_head, "mk", temp);
            }
        }

        if (event -> mask & (IN_DELETE | IN_MOVED_FROM)) {
            int wd = delete_link(head, temp);
            inotify_rm_watch(fd, wd);
            if (event->mask & IN_MOVED_FROM) {
                add_list(0, event_head, "mf", temp);
            } else { 
                add_list(0, event_head, "rf", temp);
            }
        }

        //是其他、普通文件？
    } else {
        if (event -> mask & IN_MODIFY) { 
            add_list(0, event_head, "up", temp);
        } else if (event -> mask & (IN_CREATE | IN_MOVED_TO)) {
            //是符号文件？
            int flag = _if_linkfile(temp);  
            if (S_IFLNK == flag) {
                add_list(0, event_head, "ln", temp);
            } else if (flag) {
                add_list(flag, event_head, "hd", temp);
            } else {
                add_list(0, event_head, "up", temp);
            }

        } else if (event -> mask & (IN_DELETE | IN_MOVED_FROM)) {
            add_list(0, event_head, "rm", temp);
        } 
    }
}

/*******
 *信号处理
 *功能：对事件队列进行处理
 ********/
void signal_do(int num)
{  
    printf("zhengquan%d\n", num);
    p_list();
    struct event_list *temp_head = _event_P;
    while (temp_head != NULL) {
        if (!strcmp(temp_head -> cmd, "up")) {
            mk_file(0, temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head -> cmd, "rm")) { 
            rm_file(temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head->cmd, "mk")) {
            mk_dir(temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head -> cmd, "rf")) {
            rm_dir(temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head -> cmd, "ln")) {
            mk_linkfile(temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head -> cmd, "mf")) {
            mv_dir(FROM, temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head -> cmd, "mo")) {
            mv_dir(OUT, temp_head -> do_path, _sock);
        } else if (!strcmp(temp_head -> cmd, "hd")) {
            char *s_path = seek_h_Source_file(SYNC_PATH, temp_head -> inode, temp_head -> do_path);
            if (s_path) {
                hard_link(_sock, temp_head -> do_path, s_path);
                free(s_path);
            } else {
                printf("没有找到硬链接%s的源文件\n", temp_head -> do_path);
            }
        }
        struct event_list *free_node = temp_head;
        temp_head = temp_head -> next;
        delete_list(_event_P, free_node -> do_path);
    }
    _event_P = temp_head;
}


void p_list()
{
    struct event_list *temp_head = _event_P;
    while (temp_head) {
        printf("cmd:%s\n", temp_head -> cmd);
        printf("do_path:%s\n", temp_head -> do_path);
        temp_head = temp_head -> next;
    }
}

void p_dir(struct dir_link *dir_head)
{
    struct dir_link *temp_head = dir_head;
    while (temp_head) {
        printf("wd:%d\n", temp_head -> wd);
        printf("do_path:%s\n", temp_head -> dir_path);
        temp_head = temp_head -> next;
    }
}

/*******
 *记录同步日志
 *******/
void journal_write(char *write_buf)
{
    strcat(write_buf, "\n\r");
    int sync_file = open(SYNC_FILE, O_WRONLY | O_CREAT, 0644);
    if (sync_file == -1) {
        perror("sync_file open error");
        return;
    }

    lseek(sync_file, 0, SEEK_END);//从文件尾部开始写
    int n = write (sync_file, write_buf, sizeof(write_buf));
    if (n == -1) {
        perror("sync_file wirte error ");  
        close(sync_file);
        return ;
    }

    close(sync_file);
}

/******
 *移动文件夹指令
 ******/
char cmd[SIZE] = {'\0'};
void mv_dir(int num, char *dir_path, int sock)
{   
    char cmd_buf[SIZE] = {"mv "}; 
    if (FROM == num) {  //移入  
        strncpy(cmd, dir_path, strlen(dir_path));
        strcat(cmd+strlen(cmd)," ");
        return;
    } else if (OUT == num) { //移出
        strncat(cmd+strlen(cmd), dir_path, strlen(dir_path));
        strncat(cmd_buf, cmd, strlen(cmd));
        int c_fd = sock;
        int n_write = write(c_fd, cmd_buf, sizeof(cmd_buf));
        (void)memset(cmd, 0, sizeof(cmd));
        if (n_write == -1) {
            perror("mv dir error");
            return;
        }
    }
}

