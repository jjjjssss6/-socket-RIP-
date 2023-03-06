#include "common.h"

bool isSame(const RoutingInformation& a, const RoutingInformation& b) { // 方便比较
    return (a.ipAddr == b.ipAddr) && (a.netMask == b.netMask) && (a.port == b.port);
}


char *sendHandler(const RIPPackage &data) {
    static char send_msg[BUFF_LEN];
    char tmp[255], interval[] = "$";
    memset(send_msg, 0, sizeof(send_msg));
    send_msg[0] = 0;
    // std::cout << send_msg << '\n';
    sprintf(tmp, "%u", data.toAddr);
    strcat(send_msg, tmp);
    strcat(send_msg, interval);
    sprintf(tmp, "%u", data.fromAddr);
    strcat(send_msg, tmp);
    strcat(send_msg, interval);
    sprintf(tmp, "%u", data.toPort);
    strcat(send_msg, tmp);
    strcat(send_msg, interval);
    sprintf(tmp, "%u", data.fromPort);
    strcat(send_msg, tmp);
    strcat(send_msg, interval);
    sprintf(tmp, "%u", data.isRequest);
    strcat(send_msg, tmp);
    strcat(send_msg, interval);

    for (auto dv : data.data) {
        sprintf(tmp, "%u", dv.to.ipAddr);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
        sprintf(tmp, "%u", dv.to.netMask);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
        sprintf(tmp, "%u", dv.to.port);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
        sprintf(tmp, "%u", dv.nextHop.ipAddr);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
        sprintf(tmp, "%u", dv.nextHop.netMask);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
        sprintf(tmp, "%u", dv.nextHop.port);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
        sprintf(tmp, "%u", dv.distance);
        strcat(send_msg, tmp);
        strcat(send_msg, interval);
    }

    return send_msg;
}

RIPPackage recvHandler(char* message) {
    RIPPackage recv_package;
    DistanceVector distance_vector;
    DistanceVectorElement distance_vector_element;
    int len = strlen(message), num_of_interval = 0;
    unsigned int info = 0;
    for (int i = 0; i < len; i++)
    {
        if (message[i] == '$')
        {
            num_of_interval++;
            int id_of_interval = num_of_interval % 7;
            if (num_of_interval == 1)
            {
                recv_package.toAddr = info;
            }
            else if (num_of_interval == 2)
            {
                recv_package.fromAddr = info;
            }
            else if (num_of_interval == 3)
            {
                recv_package.toPort = info;
            }
            else if (num_of_interval == 4)
            {
                recv_package.fromPort = info;
            }
            else if (num_of_interval == 5)
            {
                recv_package.isRequest = info;
            }
            else
            {
                if (id_of_interval == 6)
                {
                    distance_vector_element.to.ipAddr = info;
                }
                else if (id_of_interval == 0)
                {
                    distance_vector_element.to.netMask = info;
                }
                else if (id_of_interval == 1)
                {
                    distance_vector_element.to.port = info;
                }
                else if (id_of_interval == 2)
                {
                    distance_vector_element.nextHop.ipAddr = info;
                }
                else if (id_of_interval == 3)
                {
                    distance_vector_element.nextHop.netMask = info;
                }
                else if (id_of_interval == 4)
                {
                    distance_vector_element.nextHop.port = info;
                }
                else
                {
                    distance_vector_element.distance = info;
                    distance_vector.push_back(distance_vector_element);
                }
            }
            info = 0;
        }
        else
        {
            info *= 10;
            info += message[i] - '0';
        }
    }
    recv_package.data = distance_vector;

    return recv_package;
}