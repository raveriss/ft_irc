// server.hpp

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <fstream> /* For std::ofstream */

#include "client.hpp"
#include "channel.hpp"

class Server {
public:
    Server(const std::string& port, const std::string& password);
    ~Server();

    void run();

private:
    std::string _port;
    std::string _password;
    int _server_fd;
    std::vector<struct pollfd> _poll_fds;
    std::map<int, Client*> _clients;
    std::map<std::string, Channel*> _channels;
    std::map<std::string, std::string> _operatorPasswords;

    void initServer();
    void acceptConnections();
    void handleClient(int client_fd);

    void removeClient(int client_fd);
    void processCommand(Client* client, const std::string& command);

    // Command handlers
    void cmdJoin(Client* client, const std::string& params);
    void cmdPart(Client* client, const std::string& params);
    void cmdPrivmsg(Client* client, const std::string& params);
    void cmdKick(Client* client, const std::string& params);
    void cmdInvite(Client* client, const std::string& params);
    void cmdTopic(Client* client, const std::string& params);
    void cmdMode(Client* client, const std::string& channelName, const std::string& mode);
    void cmdOper(Client* client, const std::string& name, const std::string& password);
    void cmdKill(Client* client, const std::string& target);

    // Helper methods
    bool isNicknameInUse(const std::string& nickname) const;
    Client* getClientByNick(const std::string& nickname);
    Channel* getChannelByName(const std::string& channelName);
    void sendToClient(Client* client, const std::string& message);
    void sendNumericReply(Client* client, int code, const std::string& message);

    std::string getCurrentTime() const;

    // Security methods
    void sanitizeInput(std::string& input);

    // Logging
    void logMessage(const std::string& message);
    std::ofstream _logFile;
};

#endif // SERVER_HPP
