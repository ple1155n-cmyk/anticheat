#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <functional>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSE_SOCKET close
#endif

class Server {
public:
    Server(int port);
    ~Server();

    void start(std::function<void(const std::string&, SOCKET)> onMessage);
    void stop();
    void sendToClient(SOCKET clientSocket, const std::string& message);

private:
    int m_port;
    SOCKET m_listenSocket;
    bool m_running;
    std::vector<std::thread> m_workerThreads;

    void acceptLoop(std::function<void(const std::string&, SOCKET)> onMessage);
    void handleClient(SOCKET clientSocket, std::function<void(const std::string&, SOCKET)> onMessage);
};

#endif // SERVER_H
