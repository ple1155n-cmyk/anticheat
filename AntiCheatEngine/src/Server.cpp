#include "Server.h"

Server::Server(int port) : m_port(port), m_listenSocket(INVALID_SOCKET), m_running(false) {}

Server::~Server() {
    stop();
}

void Server::start(std::function<void(const std::string&, SOCKET)> onMessage) {
    m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);

    if (bind(m_listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        CLOSE_SOCKET(m_listenSocket);
        return;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        CLOSE_SOCKET(m_listenSocket);
        return;
    }

    m_running = true;
    std::cout << "Server listening on 0.0.0.0:" << m_port << std::endl;

    m_workerThreads.emplace_back(&Server::acceptLoop, this, onMessage);
}

void Server::stop() {
    m_running = false;
    if (m_listenSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    for (auto& t : m_workerThreads) {
        if (t.joinable()) t.join();
    }
}

void Server::acceptLoop(std::function<void(const std::string&, SOCKET)> onMessage) {
    while (m_running) {
        sockaddr_in clientAddr{};
        socklen_t clientSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(m_listenSocket, (struct sockaddr*)&clientAddr, &clientSize);

        if (clientSocket != INVALID_SOCKET) {
            std::cout << "Client connected." << std::endl;
            m_workerThreads.emplace_back(&Server::handleClient, this, clientSocket, onMessage);
        }
    }
}

void Server::handleClient(SOCKET clientSocket, std::function<void(const std::string&, SOCKET)> onMessage) {
    char buffer[4096];
    std::string remaining;

    while (m_running) {
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) break;

        std::string data = remaining + std::string(buffer, bytesRead);
        remaining.clear();

        size_t pos;
        while ((pos = data.find('\n')) != std::string::npos) {
            std::string line = data.substr(0, pos);
            if (!line.empty()) {
                onMessage(line, clientSocket);
            }
            data.erase(0, pos + 1);
        }
        remaining = data;
    }

    CLOSE_SOCKET(clientSocket);
    std::cout << "Client disconnected." << std::endl;
}

void Server::sendToClient(SOCKET clientSocket, const std::string& message) {
    send(clientSocket, message.c_str(), (int)message.length(), 0);
}
