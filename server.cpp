// server.cpp

#include "server.hpp"

/* For std::remove_if */
#include <algorithm>

/* For std::isalnum */
#include <cctype> 

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sstream>
#include <ctime>

// Functor pour remplacer la lambda
struct IsNotAlnum {
    bool operator()(char c) const {
        return !std::isalnum(c);
    }
};

bool Server::isNicknameInUse(const std::string& nickname) const {
    for (std::map<int, Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nickname) {
            return true;
        }
    }
    return false;
}

void Server::sanitizeInput(std::string& input) {
    input.erase(std::remove_if(input.begin(), input.end(), IsNotAlnum()), input.end());
}

void Server::logMessage(const std::string& message) {
    if (_logFile.is_open()) {
        _logFile << "[" << getCurrentTime() << "] " << message << std::endl;
    }
}

std::string Server::getCurrentTime() const {
    std::time_t now = std::time(0);
    std::tm* localtm = std::localtime(&now);
    std::ostringstream oss;
    oss << (1900 + localtm->tm_year) << "-"
        << (1 + localtm->tm_mon) << "-"
        << localtm->tm_mday << " "
        << localtm->tm_hour << ":"
        << localtm->tm_min << ":"
        << localtm->tm_sec;
    return oss.str();
}

Server::Server(const std::string& port, const std::string& password)
    : _port(port), _password(password), _server_fd(-1) {
    // Initialize operator passwords (for demonstration purposes)
    _operatorPasswords["admin"] = "adminpass";

    initServer();

    // Open log file
    _logFile.open("server.log", std::ios::app);
    if (!_logFile.is_open()) {
        std::cerr << "Error: Unable to open log file" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Server::~Server() {
    if (_server_fd != -1) {
        close(_server_fd);
    }
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    if (_logFile.is_open()) {
        _logFile.close();
    }
}

void Server::initServer() {
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(std::atoi(_port.c_str())); // Correction appliquée

    if (bind(_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(_server_fd, SOMAXCONN) == -1) {
        throw std::runtime_error("Failed to listen on socket");
    }

    fcntl(_server_fd, F_SETFL, O_NONBLOCK);

    pollfd server_pollfd;
    server_pollfd.fd = _server_fd;
    server_pollfd.events = POLLIN;
    _poll_fds.push_back(server_pollfd);
}

void Server::run() {
    while (true) {
        int poll_count = poll(&_poll_fds[0], _poll_fds.size(), -1);
        if (poll_count == -1) {
            throw std::runtime_error("Poll failed");
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents & POLLIN) {
                if (_poll_fds[i].fd == _server_fd) {
                    acceptConnections();
                } else {
                    handleClient(_poll_fds[i].fd);
                }
            }
        }
    }
}

void Server::acceptConnections() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(_server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        std::cerr << "Failed to accept new connection" << std::endl;
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    pollfd client_pollfd;
    client_pollfd.fd = client_fd;
    client_pollfd.events = POLLIN;
    _poll_fds.push_back(client_pollfd);

    _clients[client_fd] = new Client(client_fd);
}

void Server::handleClient(int client_fd) {
    char buffer[512];
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        removeClient(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);

    // Handle the message (authentication, commands, etc.)
    // This part will be implemented later
}

void Server::removeClient(int client_fd) {
    close(client_fd);
    delete _clients[client_fd];
    _clients.erase(client_fd);

    for (size_t i = 0; i < _poll_fds.size(); ++i) {
        if (_poll_fds[i].fd == client_fd) {
            _poll_fds.erase(_poll_fds.begin() + i);
            break;
        }
    }
}

void Server::cmdPrivmsg(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string target, message;
    iss >> target;
    std::getline(iss, message);
    
    if (target.empty() || message.empty()) {
        sendNumericReply(client, 411, ":No recipient given (PRIVMSG)");
        return;
    }

    if (message[0] == ':') {
        message = message.substr(1);
    }

    Client* targetClient = getClientByNick(target);
    if (targetClient) {
        sendToClient(targetClient, ":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
    } else {
        Channel* targetChannel = getChannelByName(target);
        if (targetChannel) {
            targetChannel->broadcast(":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n", client);
        } else {
            sendNumericReply(client, 401, target + " :No such nick/channel");
        }
    }
}

void Server::processCommand(Client* client, const std::string& command) {
    // Example of how to split the command into parts
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "JOIN") {
        std::string params;
        std::getline(iss, params);
        cmdJoin(client, params);
    } else if (cmd == "PART") {
        std::string params;
        std::getline(iss, params);
        cmdPart(client, params);
    } else if (cmd == "PRIVMSG") {
        std::string params;
        std::getline(iss, params);
        cmdPrivmsg(client, params);
    } else if (cmd == "KICK") {
        std::string params;
        std::getline(iss, params);
        cmdKick(client, params);
    } else if (cmd == "INVITE") {
        std::string params;
        std::getline(iss, params);
        cmdInvite(client, params);
    } else if (cmd == "TOPIC") {
        std::string params;
        std::getline(iss, params);
        cmdTopic(client, params);
    } else if (cmd == "MODE") {
        std::string channelName, mode;
        iss >> channelName >> mode;
        cmdMode(client, channelName, mode);
    } else if (cmd == "OPER") {
        std::string name, password;
        iss >> name >> password;
        cmdOper(client, name, password);
    } else if (cmd == "KILL") {
        std::string target;
        iss >> target;
        cmdKill(client, target);
    }
}

Channel* Server::getChannelByName(const std::string& channelName) {
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    if (it != _channels.end()) {
        return it->second;
    }
    return NULL;
}

Client* Server::getClientByNick(const std::string& nickname) {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNickname() == nickname) {
            return it->second;
        }
    }
    return NULL;
}

void Server::sendToClient(Client* client, const std::string& message) {
    send(client->getFd(), message.c_str(), message.size(), 0);
}

void Server::sendNumericReply(Client* client, int code, const std::string& message) {
    std::ostringstream oss;
    oss << code << " " << message << "\r\n";
    sendToClient(client, oss.str());
}

void Server::cmdMode(Client* client, const std::string& channelName, const std::string& mode) {
    Channel* channel = getChannelByName(channelName);
    if (!channel) {
        sendNumericReply(client, 403, channelName + " :No such channel");
        return;
    }

    if (mode.empty()) {
        sendNumericReply(client, 461, "MODE :Not enough parameters");
        return;
    }

    char sign = mode[0];
    char flag = mode[1];
    std::string param = mode.substr(2);

    switch (flag) {
        case 'o': // Opérateur
            if (sign == '+') {
                Client* targetClient = getClientByNick(param);
                if (targetClient) {
                    channel->addOperator(targetClient);
                }
            } else {
                Client* targetClient = getClientByNick(param);
                if (targetClient) {
                    channel->removeOperator(targetClient);
                }
            }
            break;
        case 'l': // Limite d'utilisateurs
            if (sign == '+') {
                int limit = std::atoi(param.c_str());
                channel->setUserLimit(limit);
            } else {
                channel->removeUserLimit();
            }
            break;
        default:
            sendNumericReply(client, 472, std::string(1, flag) + " :is unknown mode char to me");
            return;
    }

    std::string response = ":" + client->getNickname() + " MODE " + channelName + " " + mode;
    if (!param.empty())
        response += " " + param;
    response += "\r\n";
    channel->broadcast(response, NULL);
}

void Server::cmdOper(Client* client, const std::string& name, const std::string& password) {
    if (_operatorPasswords[name] == password) {
        client->setOperator(true);
        sendNumericReply(client, 381, ":You are now an IRC operator");
    } else {
        sendNumericReply(client, 464, ":Password incorrect");
    }
}

void Server::cmdKill(Client* client, const std::string& target) {
    Client* targetClient = getClientByNick(target);
    if (!targetClient) {
        sendNumericReply(client, 401, target + " :No such nick");
        return;
    }
    std::string response = ":" + client->getNickname() + " KILL " + targetClient->getNickname() + " :Killed by operator\r\n";
    sendToClient(targetClient, response);
    removeClient(targetClient->getFd());
}


// Implémentation des méthodes de commande manquantes

void Server::cmdJoin(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName;
    iss >> channelName;

    if (channelName.empty()) {
        sendNumericReply(client, 461, "JOIN :Not enough parameters");
        return;
    }
    if (channelName[0] != '#') {
        sendNumericReply(client, 476, channelName + " :Invalid channel name");
        return;
    }

    Channel* channel;
    if (_channels.find(channelName) == _channels.end()) {
        channel = new Channel(channelName);
        _channels[channelName] = channel;
    } else {
        channel = _channels[channelName];
    }

    if (channel->hasClient(client)) {
        sendNumericReply(client, 443, channelName + " :You're already on that channel");
        return;
    }

    channel->addClient(client);
    std::string response = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
    channel->broadcast(response, NULL);
    sendToClient(client, response);

    // Envoyer le topic actuel
    if (!channel->getTopic().empty()) {
        sendNumericReply(client, 332, channelName + " :" + channel->getTopic());
    } else {
        sendNumericReply(client, 331, channelName + " :No topic is set");
    }
}

void Server::cmdPart(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName;
    iss >> channelName;

    if (channelName.empty()) {
        sendNumericReply(client, 461, "PART :Not enough parameters");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel) {
        sendNumericReply(client, 403, channelName + " :No such channel");
        return;
    }
    if (!channel->hasClient(client)) {
        sendNumericReply(client, 442, channelName + " :You're not on that channel");
        return;
    }

    channel->removeClient(client);
    std::string response = ":" + client->getNickname() + " PART " + channelName + "\r\n";
    channel->broadcast(response, NULL);
    sendToClient(client, response);

    if (channel->isEmpty()) {
        delete channel;
        _channels.erase(channelName);
    }
}

void Server::cmdKick(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName, targetNick;
    iss >> channelName >> targetNick;

    if (channelName.empty() || targetNick.empty()) {
        sendNumericReply(client, 461, "KICK :Not enough parameters");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel) {
        sendNumericReply(client, 403, channelName + " :No such channel");
        return;
    }
    if (!channel->isOperator(client)) {
        sendNumericReply(client, 482, channelName + " :You're not channel operator");
        return;
    }

    Client* targetClient = channel->getClientByNick(targetNick);
    if (!targetClient) {
        sendNumericReply(client, 441, targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    channel->removeClient(targetClient);
    std::string response = ":" + client->getNickname() + " KICK " + channelName + " " + targetNick + " :Kicked\r\n";
    channel->broadcast(response, NULL);
    sendToClient(targetClient, response);

    // Vérifier si le canal est vide
    if (channel->isEmpty()) {
        delete channel;
        _channels.erase(channelName);
    }
}

void Server::cmdInvite(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string targetNick, channelName;
    iss >> targetNick >> channelName;

    if (targetNick.empty() || channelName.empty()) {
        sendNumericReply(client, 461, "INVITE :Not enough parameters");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel) {
        sendNumericReply(client, 403, channelName + " :No such channel");
        return;
    }
    if (!channel->isOperator(client)) {
        sendNumericReply(client, 482, channelName + " :You're not channel operator");
        return;
    }

    Client* targetClient = getClientByNick(targetNick);
    if (!targetClient) {
        sendNumericReply(client, 401, targetNick + " :No such nick");
        return;
    }

    channel->addInvitedClient(targetClient);
    std::string response = ":" + client->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    sendToClient(client, response);
    sendToClient(targetClient, response);
}

void Server::cmdTopic(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName;
    iss >> channelName;

    if (channelName.empty()) {
        sendNumericReply(client, 461, "TOPIC :Not enough parameters");
        return;
    }

    Channel* channel = _channels[channelName];
    if (!channel) {
        sendNumericReply(client, 403, channelName + " :No such channel");
        return;
    }
    if (!channel->hasClient(client)) {
        sendNumericReply(client, 442, channelName + " :You're not on that channel");
        return;
    }

    std::string topic;
    std::getline(iss, topic);

    if (topic.empty()) {
        // Récupérer le topic
        if (channel->getTopic().empty()) {
            sendNumericReply(client, 331, channelName + " :No topic is set");
        } else {
            sendNumericReply(client, 332, channelName + " :" + channel->getTopic());
        }
    } else {
        // Définir le topic
        if (channel->isTopicRestricted() && !channel->isOperator(client)) {
            sendNumericReply(client, 482, channelName + " :You're not channel operator");
            return;
        }
        if (topic[0] == ' ')
            topic = topic.substr(1);
        if (topic[0] == ':')
            topic = topic.substr(1);
        channel->setTopic(topic);
        std::string response = ":" + client->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
        channel->broadcast(response, NULL);
    }
}

