#include <sys/syscall.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#define MAX_CONNECT_BACKLOG  128

int memfd_create(const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

// 创建一个内存文件
int new_mem_file() {
    char *shm;
    const int shm_size = 1024;
    int fd, ret;

    fd = memfd_create("Server memfd", MFD_ALLOW_SEALING);
    if (ret != 0){
        printf("memfd_create() error\n");
    	exit(0);    
    }

    ret = ftruncate(fd, shm_size);
    if (ret == -1){
        printf("ftruncate() error\n");
    	exit(0);
    }

    shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED){
        printf("mmap() error\n");
    	exit(0);    
    }

    sprintf(shm, "这段内容是保存在共享内存里的，接收方和发送方都能根据自己的fd访问到这块内容");
    return fd;
}


// 将文件句柄对应的文件发送到客户端上
void send_mem_file(int conn, int fd) {
    struct msghdr msgh;
    struct iovec iov;
    union {
        struct cmsghdr cmsgh;
        char   control[CMSG_SPACE(sizeof(int))];
    } control_un;

    if (fd == -1) {
        printf("Cannot pass an invalid fd equaling -1\n");
        exit(0);
    }

    char info = 'A';
    iov.iov_base = &info;
    iov.iov_len = sizeof(info);

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = control_un.control;
    msgh.msg_controllen = sizeof(control_un.control);

    // 把 fd（对应的文件）发过去 
    control_un.cmsgh.cmsg_len = CMSG_LEN(sizeof(int));
    control_un.cmsgh.cmsg_level = SOL_SOCKET;
    control_un.cmsgh.cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(CMSG_FIRSTHDR(&msgh))) = fd;

    int size = sendmsg(conn, &msgh, 0);
    if (size < 0){
    	printf("Cannot pass an invalid fd equaling -1\n");
        exit(0);
    }
}

int main(int argc, char **argv) {
    int sock, conn, fd, ret;
    struct sockaddr_un address;
    socklen_t addrlen;

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock == -1){
        printf("create socket error\n");
    	exit(0);
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, "./unix_socket");

    ret = bind(sock, (struct sockaddr *) &address, sizeof(address));
    if (ret != 0){
        printf("socket bind error\n");
    	exit(0);    
    }

    ret = listen(sock, 1024);
    if (ret != 0){
        printf("socket listen error\n");
    	exit(0);    
    }    

    // 创建内存文件
    fd = new_mem_file();

    while (true) {
    	conn = accept(sock, (struct sockaddr *) &address, &addrlen);
        if (conn == -1)
            break;

        // 将内存文件句柄发送给客户端
        send_mem_file(conn, fd);
        close(conn);
    }

    close(fd);
    return 0;
}
