#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <vector>

#define BUFF_LEN 1024

extern in_addr_t ipAddress;
extern in_port_t recvPort, sendPort;

struct RoutingInformation {
    in_addr_t ipAddr, netMask;
    in_port_t port;
};

typedef std::vector<RoutingInformation> RoutingTable;

struct DistanceVectorElement {
    RoutingInformation to, nextHop;
    uint8_t distance;
};

typedef std::vector<DistanceVectorElement> DistanceVector;

struct RIPPackage {
    in_addr_t toAddr, fromAddr;
    in_port_t toPort, fromPort;
    int isRequest;
    DistanceVector data;
};

extern RoutingTable routingTable;
extern DistanceVector distanceVector;

RIPPackage recvHandler(char*);
char* sendHandler(const RIPPackage&);
bool isSame(const RoutingInformation&, const RoutingInformation&);

/*以下函数是RIP协议需要处理的*/

/*发送 RIP Request*/
RIPPackage pullRequest(in_addr_t toAddr, in_port_t toPort);

/*发送 RIP Response*/
RIPPackage pullResponse(in_addr_t toAddr, in_port_t toPort);

/*处理接收到的 RIP 协议包并进行路由表更新 返回是否对路由表进行了更新*/
bool handleRIPPackage(const RIPPackage& data);


/*以下函数是服务器部分需要处理的*/

/*client to do 将需要发送的RIP包转化编码 建立套接字发送 发送后关闭套接字*/
void sendPackage(const RIPPackage& data);
/*server to do 将接收到的信息转化为RIP包返回处理*/
RIPPackage recvHandler(char*);
/*如果对路由表进行了更新 向所有相邻节点发response*/
void pullResponseToAll(bool);
/*server to do 监听信息的主程序 直接当作main来写*/
void listener(bool);