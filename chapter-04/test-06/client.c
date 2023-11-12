#include <sys/syscall.h>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// 接收内存文件句柄
static int receive_mem_file(int conn) {
    struct msghdr msgh;
    struct iovec iov;
    union {
        struct cmsghdr cmsgh;
        char   control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmsgh;

    char placeholder;
    iov.iov_base = &placeholder;
    iov.iov_len = sizeof(char);
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_control = control_un.control;
    msgh.msg_controllen = sizeof(control_un.control);

    int size = recvmsg(conn, &msgh, 0);
    if (size == -1){
    	printf("recvmsg() error\n");
    	exit(0);    
    }
    cmsgh = CMSG_FIRSTHDR(&msgh);
    if (!cmsgh){
    	printf("Expected one recvmsg() header with a passed memfd fd, But zero headers got!\n");
    	exit(0);    
    }
    return *((int *) CMSG_DATA(cmsgh));
}

int main(int argc, char **argv) {
	char *shm, *shm1, *shm2;
    const int shm_size = 1024;
    int conn, ret, fd, seals;
    struct sockaddr_un address;

    conn = socket(PF_UNIX, SOCK_STREAM, 0);
    if (conn == -1){
        printf("socket() error\n");
    	exit(0);    
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, "./unix_socket");

    ret = connect(conn, (struct sockaddr *)&address, sizeof(struct sockaddr_un));
    if (ret != 0)
    {        
    	printf("connect() error\n");
    	exit(0);    
    }

    // 接收文件句柄
    fd = receive_mem_file(conn);

    // 读取文件内容
    shm = mmap(NULL, shm_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (shm == MAP_FAILED){
        printf("mmap() error\n");
    	exit(0);    
    }
    printf("共享内存中的文件内容是: %s\n", shm);
}