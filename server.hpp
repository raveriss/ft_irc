#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <algorithm>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include "client.hpp"
#include <cerrno>   // Pour utiliser errno et EINTR
#include <cstring>  // Pour utiliser strerror()


#define MAX_CLIENTS 100
#define BUFFER_SIZE 512

class Server {
private:
    int port;
    int server_fd;
    fd_set read_fds, master_fds;
    std::map<int, Client> clients;
    std::set<int> client_fds;
    std::string _password;  // Ajout du mot de passe

    
public:
    Server(int port, const std::string &password);
    ~Server();

    void run();
    void stop();
    void handleNewConnection();
    void handleClientMessage(int client_fd);
    void removeClient(int client_fd);
    std::string receivePasswordFromClient(int client_fd);
    void sendMessage(int client_fd, const std::string &message);
    std::string parseCommand(const std::string &message);
    std::string extractPassword(const std::string &message);
    std::string receiveMessage(int client_fd);
    std::string extractNickname(const std::string &message);
    bool isNicknameInUse(const std::string &nickname);
    std::string extractUsername(const std::string &message);





};

#endif
