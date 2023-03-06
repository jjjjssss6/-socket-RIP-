#include "common.h"
#include <iostream>

/*client to do 将需要发送的RIP包转化编码 建立套接字发送 发送后关闭套接字*/
// 建立 Client 套接字
// 解码 RIPPackage
// 发送 RIPPackage
// 关闭套接字

void sendPackage(const RIPPackage &data) {
    int client_sock;
    struct sockaddr_in server_addr;
    char *host;
    char *BUF;
    BUF = sendHandler(data);

    int buff_len = strlen(BUF);

    int client_fd;
    struct sockaddr_in ser_addr;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0)
    {
        std::cerr << "create socket fail!\n";
        return;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = data.toAddr;
    ser_addr.sin_port = data.toPort;

    socklen_t len;
    struct sockaddr_in src;
    len = sizeof(*(struct sockaddr *)&ser_addr);
    // printf("client:%s\n", BUF); // 打印自己发送的信息
    sendto(client_fd, BUF, buff_len, 0, (struct sockaddr *)&ser_addr, len);
    memset(BUF, 0, buff_len);

    close(client_fd);
}