#include "common.h"
#include <iostream>

void pullResponseToAll(bool isMonitoringMode) {
    for (auto& adjacentHost: routingTable) {
        if (isMonitoringMode)
            std::cout << "Pulling Response to: " << adjacentHost.ipAddr << ':' << adjacentHost.port << std::endl;
        sendPackage(pullResponse(adjacentHost.ipAddr, adjacentHost.port));
    }
}

void handle_udp_msg(int fd, bool isMonitoringMode) {
    if (isMonitoringMode) {
        for (auto& adjacentHost: routingTable) {
            if (isMonitoringMode)
                std::cout << "Pulling Request to: " << adjacentHost.ipAddr << ':' << adjacentHost.port << std::endl;
            sendPackage(pullRequest(adjacentHost.ipAddr, adjacentHost.port));
        }
    }
    char buf[BUFF_LEN];  //接收缓冲区，1024字节
    socklen_t len;
    int count;
    struct sockaddr_in clent_addr;  //clent_addr用于记录发送方的地址信息
    while(1)
    {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(clent_addr);
        // std::cout << "listen..." << '\n';
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&clent_addr, &len);  //recvfrom是拥塞函数，没有数据就一直拥塞
        if(count == -1)
        {
            std::cerr << "receive data error" << '\n';
            return;
        }
        if (isMonitoringMode) {
            RIPPackage pkg = recvHandler(buf);
            bool existFlag = false;
            for (auto& adj: routingTable)
                if (adj.ipAddr == pkg.fromAddr && adj.port == pkg.fromPort) {
                    existFlag = true;
                    break;
                }
            if (!existFlag) continue;
            std::cout << "Recieve a Package From ";
            std::cout << pkg.fromAddr << ' ' << pkg.fromPort << '\n';
            std::cout << "Type is " << pkg.isRequest << '\n';
            if (pkg.isRequest) {
                std::cout << "Pulling Response to: " << pkg.fromAddr << ':' << pkg.fromPort << std::endl;
                sendPackage(pullResponse(pkg.fromAddr, pkg.fromPort));
            }
            else {
                std::cout << "Distance Vector:\n";
                for (auto c : pkg.data)
                {
                    std::cout << c.to.ipAddr << ' ' << c.to.port << '\n';
                    std::cout << c.nextHop.ipAddr << ' ' << c.nextHop.port << '\n';
                    std::cout << (int)c.distance << '\n';
                }
                std::cout << std::flush;
                if (handleRIPPackage(pkg)) { 
                    std::cout << "Distance Vector has been changed:\n";
                    for (auto c : distanceVector) {
                        std::cout << c.to.ipAddr << ' ' << c.to.port << '\n';
                        std::cout << c.nextHop.ipAddr << ' ' << c.nextHop.port << '\n';
                        std::cout << (int)c.distance << '\n';
                    }
                    std::cout << std::flush;
                    pullResponseToAll(isMonitoringMode);
                }
            }
        }
        else {
            static size_t cnt = 0;
            static char count[BUFF_LEN] = "RIPdata";
            sprintf(count + 7, "%u-%lu", recvPort, cnt);
            FILE* data = fopen(count, "w");
            fwrite(buf, 1, BUFF_LEN, data);
            fclose(data); cnt++;
        }
    }
}

void listener(bool isMonitoringMode = false) {
    int server_fd, ret;
    struct sockaddr_in ser_addr; 

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if(server_fd < 0) {
        std::cerr << "create socket fail!\n";
        return;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = ipAddress; 
    ser_addr.sin_port = recvPort;

    ret = bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
    if(ret < 0) {
        std::cerr << "socket bind fail!\n";
        return;
    }

    handle_udp_msg(server_fd, isMonitoringMode);   //处理接收到的数据

    close(server_fd);
    return;
}