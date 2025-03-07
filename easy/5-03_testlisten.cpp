/**
 * 编写一个服务器程序
 * 探究backlog参数对listen系统调用的实际影响
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;
// SIGHTERM信号的处理函数，触发时结束主程序的循环
static void handle_term(int sig)
{
    stop = true;
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, handle_term);

    if (argc <= 3)
    {
        printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    struct sockaddr_in address; // 创建一个IPv4 socket地址
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET; // 设定地址组为AF_INET(TCP/IPv4协议族)
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, backlog);
    assert(ret != -1);

    // 循环等待连接，直到有SIGTERM信号将其终端
    while (!stop)
    {
        sleep(1);
    }

    close(sock);
    return 0;
}
