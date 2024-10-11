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
        std::cout << poll_count << std::endl;
        if (poll_count == -1) {
            throw std::runtime_error("Poll failed");
        }

        for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].revents & POLLIN) {
                if (_poll_fds[i].fd == _server_fd) {
                    acceptConnections();
                    std::cout << "Conection" << std::endl;
                } else {
                    handleClient(_poll_fds[i].fd);
                    std::cout << _poll_fds[i].fd << std::cout;
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
        if (bytes_received == 0) {
            // Le client a fermé la connexion
            std::cout << "Client déconnecté : " << client_fd << std::endl;
        } else {
            std::cerr << "Erreur : réception échouée pour le client " << client_fd << std::endl;
        }
        removeClient(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);

    // Récupérer le client correspondant
    Client* client = _clients[client_fd];

    // Ajouter le message au tampon du client
    client->appendBuffer(message);

    // Traiter les commandes complètes
    std::string command;
    while (!(command = client->getNextCommand()).empty()) {
        // Enregistrer la commande dans les logs
        logMessage(client->getNickname() + ": " + command);

        // Traiter la commande
        processCommand(client, command);
    }
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
    std::string target;
    iss >> target;
    
    std::string message;
    std::getline(iss, message);
    
    if (target.empty()) {
        sendNumericReply(client, 411, ":No recipient given (PRIVMSG)"); // ERR_NORECIPIENT
        return;
    }
    
    if (message.empty()) {
        sendNumericReply(client, 412, ":No text to send"); // ERR_NOTEXTTOSEND
        return;
    }
    
    if (message[0] == ':')
        message = message.substr(1);
    
    Client* targetClient = getClientByNick(target);
    if (targetClient) {
        std::string response = ":" + client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
        sendToClient(targetClient, response);
    } else {
        Channel* targetChannel = getChannelByName(target);
        if (targetChannel) {
            if (!targetChannel->hasClient(client)) {
                sendNumericReply(client, 404, target + " :Cannot send to channel"); // ERR_CANNOTSENDTOCHAN
                return;
            }
            std::string response = ":" + client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
            broadcastToChannel(targetChannel, response, client);
        } else {
            sendNumericReply(client, 401, target + " :No such nick/channel"); // ERR_NOSUCHNICK
        }
    }
}


bool Server::isValidNickname(const std::string& nickname) {
    if (nickname.length() > 9)
        return false;
    for (size_t i = 0; i < nickname.length(); ++i) {
        if (!std::isalnum(nickname[i]) && nickname[i] != '-' && nickname[i] != '_')
            return false;
    }
    return true;
}

void Server::processCommand(Client* client, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    // Convertir la commande en majuscules pour la comparaison
    for (size_t i = 0; i < cmd.size(); ++i) {
        cmd[i] = std::toupper(cmd[i]);
    }

    if (cmd == "PASS")
    {
        std::string password;
        iss >> password;
        client->setPassword(password);
    } 
    else if (cmd == "NICK")
    {
        std::string nickname;
        iss >> nickname;
        if (nickname.empty()) {
            sendNumericReply(client, 431, ":No nickname given"); // ERR_NONICKNAMEGIVEN
            return;
        }
        // Vérifier si le pseudo contient des caractères invalides
        if (!isValidNickname(nickname)) {
            sendNumericReply(client, 432, nickname + " :Erroneous nickname"); // ERR_ERRONEUSNICKNAME
            return;
        }
        if (isNicknameInUse(nickname)) {
            sendNumericReply(client, 433, nickname + " :Nickname is already in use"); // ERR_NICKNAMEINUSE
            return;
        }
        client->setNickname(nickname);
}

    else if (cmd == "USER") {
        std::string username, hostname, servername, realname;
        iss >> username >> hostname >> servername;
        std::getline(iss, realname);

        if (username.empty() || hostname.empty() || servername.empty() || realname.empty()) {
            sendNumericReply(client, 461, "USER :Not enough parameters"); // ERR_NEEDMOREPARAMS
            return;
        }

        if (realname[0] == ':')
            realname = realname.substr(1);

        client->setUsername(username);
        client->setRealname(realname);
}


    if (!client->isAuthenticated()) {
        if (client->hasNickname() && client->hasUsername() && client->hasPassword()) {
            if (client->isPasswordValid(_password)) {
                client->authenticate();
                sendNumericReply(client, 001, ":Welcome to the IRC server " + client->getNickname());
            } else {
                sendNumericReply(client, 464, ":Password incorrect");
                removeClient(client->getFd());
            }
        } else {
            // Attendre que le client envoie toutes les informations requises
            return;
        }
    } else {
        // Traiter les autres commandes une fois authentifié
        if (cmd == "PING") {
            std::string token;
            iss >> token;
            std::string response = "PONG " + token + "\r\n";
            sendToClient(client, response);
        } else if (cmd == "JOIN") {
            std::string params;
            std::getline(iss, params);
            cmdJoin(client, params);
        }
        // ... [Autres commandes]
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

void Server::sendNamesReply(Client* client, Channel* channel) {
    std::string nickList;
    const std::set<Client*>& clients = channel->getClients();
    for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (channel->isOperator(*it))
            nickList += "@";
        nickList += (*it)->getNickname() + " ";
    }
    sendNumericReply(client, 353, "= " + channel->getName() + " :" + nickList); // RPL_NAMREPLY
    sendNumericReply(client, 366, channel->getName() + " :End of /NAMES list"); // RPL_ENDOFNAMES
}

void Server::broadcastToChannel(Channel* channel, const std::string& message, Client* sender) {
    const std::set<Client*>& clients = channel->getClients();
    for (std::set<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (*it != sender) {
            sendToClient(*it, message);
        }
    }
}

void Server::cmdJoin(Client* client, const std::string& params) {
    std::istringstream iss(params);
    std::string channelName, key;
    iss >> channelName;

    if (channelName.empty()) {
        sendNumericReply(client, 461, "JOIN :Not enough parameters"); // ERR_NEEDMOREPARAMS
        return;
    }
    if (channelName[0] != '#' && channelName[0] != '&') {
        sendNumericReply(client, 476, channelName + " :Invalid channel name"); // ERR_BADCHANMASK
        return;
    }

    Channel* channel;
    if (_channels.find(channelName) == _channels.end()) {
        // Créer le canal
        channel = new Channel(channelName);
        _channels[channelName] = channel;
    } else {
        channel = _channels[channelName];
        // Vérifier les conditions d'accès au canal
        if (channel->isInviteOnly() && !channel->isInvited(client)) {
            sendNumericReply(client, 473, channelName + " :Cannot join channel (+i)"); // ERR_INVITEONLYCHAN
            return;
        }
        if (channel->hasUserLimit() && channel->getClientCount() >= channel->getUserLimit()) {
            sendNumericReply(client, 471, channelName + " :Cannot join channel (+l)"); // ERR_CHANNELISFULL
            return;
        }
        if (channel->hasKey()) {
            iss >> key;
            if (key != channel->getKey()) {
                sendNumericReply(client, 475, channelName + " :Cannot join channel (+k)"); // ERR_BADCHANNELKEY
                return;
            }
        }
    }

    if (channel->hasClient(client)) {
        sendNumericReply(client, 443, client->getNickname() + " " + channelName + " :is already on channel"); // ERR_USERONCHANNEL
        return;
    }

    channel->addClient(client);
    std::string response = ":" + client->getNickname() + " JOIN " + channelName + "\r\n";
    sendToClient(client, response);
    broadcastToChannel(channel, response, client);


    // Envoyer la liste des utilisateurs sur le canal
    sendNamesReply(client, channel);

    // Envoyer le topic actuel
    if (!channel->getTopic().empty()) {
        sendNumericReply(client, 332, channelName + " :" + channel->getTopic()); // RPL_TOPIC
    } else {
        sendNumericReply(client, 331, channelName + " :No topic is set"); // RPL_NOTOPIC
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
    sendToClient(client, response);
    broadcastToChannel(channel, response, client);


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
    broadcastToChannel(channel, response, targetClient);
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
        broadcastToChannel(channel, response, client);
    }
}