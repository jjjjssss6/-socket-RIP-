#include "common.h"
#include <iostream>

/*发送 RIP Request*/
RIPPackage pullRequest(in_addr_t toAddr, in_port_t toPort) {
    RIPPackage requestPackage;
    requestPackage.toAddr = toAddr;
    requestPackage.toPort = toPort;
    requestPackage.isRequest = 1;
    requestPackage.fromAddr = ipAddress;
    requestPackage.fromPort = recvPort;
    requestPackage.data = {};
    return requestPackage;
}

/*发送 RIP Response*/
RIPPackage pullResponse(in_addr_t toAddr, in_port_t toPort) {
    RIPPackage responsePackage;
    responsePackage.toAddr = toAddr;
    responsePackage.toPort = toPort;
    responsePackage.data = distanceVector;
    responsePackage.fromAddr = ipAddress;
    responsePackage.fromPort = recvPort;
    responsePackage.isRequest = 0;
    return responsePackage;
}

/*处理接收到的 RIP 协议包并进行路由表更新 返回是否对路由表进行了更新*/
bool handleRIPPackage(const RIPPackage& data) {
    DistanceVector tempDV = data.data;
    in_addr_t xAddr = data.fromAddr;
    in_port_t xPort = data.fromPort;
    RoutingInformation fromInfo = RoutingInformation{xAddr, htonl(INADDR_NONE), xPort};
    int flag = 0;
    for(int i = 0, j; i < tempDV.size(); i++) {
        //遍历tempDV
        //下一跳全部改为xAddr,xPort
        tempDV[i].nextHop = fromInfo;
        tempDV[i].distance++;//距离全部加1
        for(j = 0; j < distanceVector.size(); j++) {
            if( isSame( distanceVector[j].to, tempDV[i].to ))//第i个tempDV的目的地在路由表中有
            {
                if( isSame( distanceVector[j].nextHop, tempDV[i].nextHop ))//下一跳相同，要新的
                {
                    distanceVector[j].distance = tempDV[i].distance;
                    flag = 1;
                }
                else
                {
                    if(distanceVector[j].distance > tempDV[i].distance)//下一跳不同，要短的
                    {
                        distanceVector[j] = tempDV[i];
                        flag = 1;
                    }
                }
                break;
            }
        }
        if( j == distanceVector.size() )//第i个tempDV的目的地在路由表中没有，直接加入
        {
            distanceVector.push_back(tempDV[i]);
            flag = 1;
        }
    }
    return flag;
}
