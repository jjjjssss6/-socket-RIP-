#include "common.h"
#include <string.h>

#include <string>
#include <iostream>

#include <sys/wait.h>

RoutingInformation myInfo;
RoutingTable routingTable;
DistanceVector distanceVector;

in_addr_t ipAddress;
in_port_t recvPort, sendPort;

static volatile pid_t pid;
typedef	void	Sigfunc(int);

void errorPrint(const char* msg) {
    system("clear");
    puts(msg);
    exit(1);
}

void pressEnterToContinue() {
    //利用此函数确保所有提示信息能够让用户接收到，可以将其改进为弹窗提示
    std::cout << "请按回车继续！" << std::endl;
    std::string s; std::getline(std::cin, s); 
    std::getline(std::cin, s); 
}

void confirmMessage(const char* msg) {
    std::cout << msg << std::endl;
    pressEnterToContinue();
}

void handle() {
    static size_t cnt = 0;
    static char count[BUFF_LEN] = "RIPdata";
    static char buf[BUFF_LEN];
    for (;;) {
        sprintf(count + 7, "%u-%lu", recvPort, cnt);
        FILE* data = fopen(count, "r");
        if (data == NULL) return;
        memset(buf, 0, BUFF_LEN);
        fread(buf, 1, BUFF_LEN, data);
        fclose(data); 
        RIPPackage pkg = recvHandler(buf);
        bool existFlag = false;
        for (auto& adj: routingTable)
            if (adj.ipAddr == pkg.fromAddr && adj.port == pkg.fromPort) {
                existFlag = true;
                break;
            }
        if (!existFlag) {++cnt; continue;}
        if (pkg.isRequest) {
            sendPackage(pullResponse(pkg.fromAddr, pkg.fromPort));
        }
        else {
            if (handleRIPPackage(pkg))
                pullResponseToAll(false);
        }
        cnt++;
    }
}

void addAdjacent() {
    std::cout << "请输入IP地址" << std::endl;
    static std::string temp;
    std::cin >> temp;
    static in_addr_t ipAddr, netMask = htonl(INADDR_NONE);
    if (temp == "localhost") ipAddr = htonl(INADDR_ANY);
	else ipAddr = inet_addr(temp.c_str());
    if (ipAddr == -1) {
        confirmMessage("ip address error!");
        return;
    }
    std::cout << "请输入接收端口" << std::endl;
    static in_port_t port;
    std::cin >> temp;
    try {    
        port = htons(stoi(temp));
    } catch (std::invalid_argument) {
        confirmMessage("port error!");
        return;
    }
    RoutingInformation newAdj = (RoutingInformation){ipAddr, netMask, port};
    for (auto& adj: routingTable) {
        if (isSame(adj, newAdj)) {
            confirmMessage("This server has been adjacent!");
            return;
        }
    }
    routingTable.emplace_back(newAdj);
    if (pid) sendPackage(pullRequest(newAdj.ipAddr, newAdj.port));
}

void removeAdjacent() {
    std::cout << "请输入要删除的IP地址" << std::endl;
    static std::string temp;
    std::cin >> temp;
    static in_addr_t ipAddr, netMask = htonl(INADDR_NONE);
    if (temp == "localhost") ipAddr = htonl(INADDR_ANY);
	else ipAddr = inet_addr(temp.c_str());
    if (ipAddr == -1) {
        confirmMessage("ip address error!");
        return;
    }
    std::cout << "请输入要删除的接收端口" << std::endl;
    static in_port_t port;
    std::cin >> temp;
    try {    
        port = htons(stoi(temp));
    } catch (std::invalid_argument) {
        confirmMessage("port error!");
        return;
    }
    RoutingInformation rmvAdj = (RoutingInformation){ipAddr, netMask, port};
    for (auto it = routingTable.begin(); it != routingTable.end(); ++it) {
        auto& adj = *it;
        if (isSame(adj, rmvAdj)) {
            if (pid) {
                for (auto& e: distanceVector)
                    if (isSame(e.nextHop, rmvAdj)) e.distance = 16;
                sendPackage(pullRequest(rmvAdj.ipAddr, rmvAdj.port));
            }
            routingTable.erase(it);
            return;
        }
    }
    confirmMessage("This server isn't adjacent!");
    return;
}

void dispDistanceVector() {
    if (pid == 0) {
        confirmMessage("Server has not been started.");
        return;
    }
    handle();
    system("clear");
    for (auto c : distanceVector) {
        std::cout << c.to.ipAddr << ' ' << c.to.port << '\n';
        std::cout << c.nextHop.ipAddr << ' ' << c.nextHop.port << '\n';
        std::cout << (int)c.distance << '\n';
    }
    std::cout << std::flush;
    confirmMessage("Distance Vector has been displayed.\n");
}

void startServer() {
    // std::cerr << pid << std::endl;
    if (pid == 0) {
        distanceVector.clear();
        distanceVector.push_back(DistanceVectorElement{myInfo, myInfo, 0});
        pid = fork(); 
        if (pid < 0) errorPrint("Start Server Error: cannot fork.");
        if (pid == 0) {/* child */
            static char command[BUFF_LEN];
            sprintf(command, "rm -f RIPdata%u*", recvPort);
            system(command);
            listener(false);
            exit(0);
        }
        for (auto& adjacentHost: routingTable) {
            sendPackage(pullRequest(adjacentHost.ipAddr, adjacentHost.port));
        }
        confirmMessage("Server has been started.");
    }
    else confirmMessage("Another server has been started.");
}

void endServer() {
    // std::cerr << pid << std::endl;
    if (pid) {
        if (kill(pid, SIGKILL) < 0) 
            errorPrint("End Server Error: System cannot kill.");
        confirmMessage("Server has been shut down.");
        pid = 0;
    }
    else confirmMessage("Server has not been started.");
}

/* Reliable version of signal(), using POSIX sigaction().  */
Sigfunc *signal(int signumber, Sigfunc *func) {
    // 基于课本针对本实验进行了一定的修改
    struct sigaction act, oact;
    
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    
    if (signumber == SIGALRM) {
    #ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
    #endif
    } 
    else {
    #ifdef  SA_RESTART
        act.sa_flags |= SA_RESTART;
    #endif
    }

    if (sigaction(signumber, &act, &oact) < 0)
        return(SIG_ERR);
    return oact.sa_handler;
}


void sigQuit(int sigNumber) {
    if (pid > 0) {
        kill(pid, SIGKILL);
        printf("\n*** QUIT ***\n");
    }
    pid = 0;
}

void monitoringMode() {
    // std::cerr << pid << std::endl;
    if (pid > 0) {
        confirmMessage("Server has been started.");
        return;
    }
    system("clear");
    std::cout << "Entering monitoring mode ......" << std::endl;
    std::cout << "Press ctrl + \'\\\' to end the Server" << std::endl;
    pid = fork(); 
    if (pid < 0) errorPrint("Start Server Error: cannot fork.");
    if (pid == 0) { /* child */
        distanceVector.clear();
        distanceVector.push_back(DistanceVectorElement{myInfo, myInfo, 0});
        listener(true);
        exit(0);
    }
    /* parent */
    int status;
    if ((pid = waitpid(pid, &status, 0)) < 0)
        errorPrint("Server quit error.");
    pid = 0;
    confirmMessage("Returning to Menu ......");
}

/*
    1. add adjacent server
    2. remove adjacent server
    3. start server
    4. display distance vector
    5. end server
    6. monitoring mode (ctrl + '\' to exit)
*/
void menu() {
    system("clear");
    puts("1. add adjacent server");
    puts("2. remove adjacent server");
    puts("3. start server");
    puts("4. display distance vector");
    puts("5. end server");
    puts("6. monitoring mode");
    static std::string userInput;
    std::cin >> userInput;
    int op;
    try{
        op = std::stoi(userInput);
        switch (op) {
            case 1 : addAdjacent(); break;
            case 2 : removeAdjacent(); break;
            case 3 : startServer(); break;
            case 4 : dispDistanceVector(); break;
            case 5 : endServer(); break;
            case 6 : monitoringMode(); break;
        }
    } catch(std::invalid_argument) {
        
    }
}

int main(int argc, char* argv[]) {
    signal(SIGQUIT, sigQuit);

    if (argc != 4) errorPrint("Usage: ripserver <ip address> <recv port> <send port>");
    recvPort = htons(atoi(argv[2])), sendPort = htons(atoi(argv[3]));

    if (strcmp(argv[1], "localhost") == 0) ipAddress = htonl(INADDR_ANY);
	else ipAddress = inet_addr(argv[1]);

    myInfo.ipAddr = ipAddress;
    myInfo.netMask = htonl(INADDR_NONE);
    myInfo.port = recvPort;

    for (;;) {
        menu();
        if (pid) handle();
    }
}