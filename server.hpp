#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

/* for std::remove */
#include <algorithm>

#include <string>
#include <vector>
#include <map>
#include <set>

/* for atoi */
#include <cstdlib>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 512

class Server {
private:
    int _port;
    int _server_fd;
    std::string _password;  // Ajout du mot de passe
    fd_set read_fds, master_fds;
    std::map<int, std::string> clients;
    std::set<int> client_fds;
    
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

};

#endif
