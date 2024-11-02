/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:07:13 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/02 00:58:14 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Inclusions pour les clients, les canaux et le serveur */
#include "../incs/Server.hpp"
#include "../incs/Client.hpp"
#include "../incs/Channel.hpp"

/* Déclaration statique de Server* pour gérer les signaux */
Server* Server::instance = NULL;

/**
 * Gestionnaire de signaux pour SIGINT et SIGTSTP
 */
void Server::handleSignal(int signal)
{
    const char* signalName;
    if (signal == SIGINT)
        signalName = "SIGINT (ctrl + c)";
    else if (signal == SIGTSTP)
        signalName = "SIGTSTP (ctrl + z)";
    else
        signalName = "Unknown";

    std::cout << "\nSignal " << signalName << " reçu, fermeture du serveur..." << std::endl;
    if (instance)
    {
        instance->shutdown();
    }
    exit(0);
}

/**
 * Ferme le serveur proprement
 */
void Server::shutdown() {

    /* Libération des clients */
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        delete *it;
    }
    _clients.clear();

    /* Réinitialise la capacité du vecteur à 0 */
    std::vector<Client*>().swap(_clients);

    /* Libération des canaux */
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    _channels.clear();

    /* Fermer le socket d'écoute */
    if (_listenSocket >= 0) {
        close(_listenSocket);
        _listenSocket = -1;
    }

    /* Libération du Bot */
    _bot.~Bot();

    /* Libération de DCCManager */
    _dccManager.~DCCManager();

    /* Libération des descripteurs de fichiers */
    FD_ZERO(&_masterSet);
    _fdMax = 0;

    /* Libération de l'instance statique */
    instance = NULL;

    /* Fermer tous les descripteurs de fichiers ouverts */
    for (int fd = 0; fd <= _fdMax; ++fd) {
        if (FD_ISSET(fd, &_masterSet)) {
            close(fd);
        }
    }
}


/**
 * Constructor
 */
Server::Server(unsigned short port, const std::string &password)
    : _port(port), _password(password), _serverName("ircserv"), _listenSocket(-1), _fdMax(0)
{
    /* Définit l'instance pour l'accès dans le gestionnaire */
    instance = this;
    struct sigaction sa;
    sa.sa_handler = &Server::handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTSTP, &sa, NULL) == -1) {
        throw std::runtime_error("Erreur lors de la configuration du signal SIGINT.");
    }
    init();
}

/**
 * Destructor
 */
Server::~Server()
{
    close(_listenSocket);
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        delete *it;
    }
}

/**
 * Initialize the server
 */
void Server::init()
{
    /* Création du socket d'écoute */
    _listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSocket < 0)
    {
        throw std::runtime_error("Erreur lors de la création du socket d'écoute.");
    }

    /* Configuration pour réutiliser l'adresse et le port */
    int opt = 1;
    if (setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::runtime_error("Erreur lors de la configuration du socket.");
    }

    /* Déclare la structure d'adresse du serveur */
    sockaddr_in serverAddr;

    /* Initialise la structure avec des zéros */
    std::memset(&serverAddr, 0, sizeof(serverAddr));

    /* Définit la famille d'adresses à IPv4 */
    serverAddr.sin_family = AF_INET;

    /* Définit l'adresse IP du serveur à INADDR_ANY */
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    /* Définit le port du serveur en utilisant le port spécifié */
    serverAddr.sin_port = htons(_port);

    /* Lie le socket à l'adresse du serveur spécifiée, génère une erreur en cas d'échec */
    if (bind(_listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        throw std::runtime_error("Erreur lors de la liaison du socket : " + std::string(strerror(errno)));
    }

    /* Met le socket en mode écoute, en attente de connexions entrantes */
    if (listen(_listenSocket, SOMAXCONN) < 0)
    {
        throw std::runtime_error("Erreur lors de l'écoute sur le socket.");
    }

    /* Initialise le set de descripteurs de fichiers pour les connexions */
    FD_ZERO(&_masterSet);

    /* Ajoute le socket d'écoute au set de descripteurs */
    FD_SET(_listenSocket, &_masterSet);

    /* Met à jour la valeur maximale des descripteurs de fichiers */
    _fdMax = _listenSocket;

    /* Récupérer et stocker l'adresse IP du serveur */
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == -1)
    {
        perror("gethostname");

        /* Valeur par défaut si l'obtention du nom d'hôte échoue */
        _serverIp = "127.0.0.1";
    }
    else
    {
        struct hostent* host = gethostbyname(hostname);
        if (host == NULL)
        {
            perror("gethostbyname");

            /* Valeur par défaut si la résolution du nom d'hôte échoue */
            _serverIp = "127.0.0.1";
        }
        else
        {
            _serverIp = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
        }
    }
}

/**
 * Supprime les clients inactifs de la liste
 */
void Server::cleanupInactiveClients() {
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ) {
        Client* client = *it;

        /* Méthode à implémenter pour vérifier la suspension */
        if (client->isSuspended()) {
            std::cout << "Déconnexion du client inactif : " << client->getNickname() << std::endl;
            
            // removeClient(client);
            it = _clients.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * Run the server
 */
void Server::run()
{
    /* Temps initial pour le déclenchement des PING */
    time_t lastPingTime = time(NULL);

    /* Nouveau délai pour la vérification */
    time_t lastCheckPingTime = lastPingTime;

    
    /* Boucle principale du serveur, tourne indéfiniment */
    while (true)
    {
        /* Crée un ensemble de descripteurs à partir du master set */
        fd_set readSet = _masterSet;
        fd_set writeSet;
        FD_ZERO(&writeSet);

        /* Ajouter les sockets des transferts DCC */
        _dccManager.addSocketsToSet(readSet, writeSet, _fdMax);
        
        /* Délai pour la fonction select */
        struct timeval timeout;

        /* Intervalle de 1 seconde pour déclencher régulièrement les vérifications PING */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        /* Utilise select pour surveiller les descripteurs, en attente d'événements */
        if (select(_fdMax + 1, &readSet, NULL, NULL, &timeout) < 0)
        {
            throw std::runtime_error("Erreur lors de la sélection des descripteurs.");
        }
        
        time_t currentTime = time(NULL);

        if (currentTime - lastPingTime >= PING_INTERVAL)
        {
            sendPing();

            /*  Réinitialisez le temps du dernier PING */
            lastPingTime = currentTime;

            /*  /Délai de 10 secondes avant la vérification */
            lastCheckPingTime = currentTime + PING_RESPONSE_DELAY;
        }

        /* Vérifiez les réponses des clients après le délai */
        if (currentTime >= lastCheckPingTime)
        {
            checkPingResponses();

            /* Prochaine vérification dans 60 secondes */
            lastCheckPingTime = currentTime + 60;
        }
        
        /* Appel de la fonction de nettoyage des clients inactifs */
        cleanupInactiveClients();

        /* Parcourt tous les descripteurs pour vérifier les événements */
        for (int fd = 0; fd <= _fdMax; ++fd)
        {            
            /* Vérifie si ce descripteur est prêt à être lu */
            if (FD_ISSET(fd, &readSet))
            {
                /* Vérifie si le descripteur correspond au socket d'écoute */
                if (fd == _listenSocket)
                {
                    /* Nouvelle connexion entrante */
                    handleNewConnection();
                }
                else
                {
                    /* Traite les données provenant d'un client existant */
                    Client *client = NULL;
                    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
                    {
                        /* Trouve le client correspondant au descripteur */
                        if ((*it)->getSocket() == fd)
                        {
                            client = *it;
                            break;
                        }
                    }
                    /* Si un client est trouvé, traite le message reçu */
                    if (client)
                    {
                        handleClientMessage(client);
                    }
                }
            }
        }

        /* Gérer les transferts DCC */
        _dccManager.processTransfers(readSet, writeSet);
    }
}

/**
 * Process a command received from a client
 */
void Server::handleNewConnection()
{
    int newSocket = accept(_listenSocket, NULL, NULL);
    if (newSocket == -1)
    {
        perror("accept");
        return;
    }

    /* Récupérer l'adresse IP du client */
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(newSocket, (struct sockaddr*)&addr, &addr_len) == -1)
    {
        perror("getpeername");
        close(newSocket);
        return;
    }

    char host[NI_MAXHOST];
    if (getnameinfo((struct sockaddr*)&addr, addr_len, host, sizeof(host), NULL, 0, NI_NUMERICHOST) != 0)
    {
        std::cerr << "Erreur lors de la récupération du nom d'hôte." << std::endl;
        close(newSocket);
        return;
    }

    /* Créer un nouvel objet Client */
    Client *newClient = new Client(newSocket);
    std::string defaultHost = "unknown";
    _serverIp = host;
    newClient->setHostname(host ? host : "127.0.0.1");

    /* Initialiser l'activité du client */
    newClient->updateLastActivity();


    /* Ajouter le client à la liste */
    _clients.push_back(newClient);

    /* Ajouter le socket au master set */
    FD_SET(newSocket, &_masterSet);
    if (newSocket > _fdMax)
    {
        _fdMax = newSocket;
    }

    std::cout << "Client " << newSocket 
          << ", IP " << host << ". \nCmds PASS, NICK et USER pour enregistrer le client.\n"
          << std::endl;
}

/**
 * Return the channel object from a message
 */
Channel* Server::getChannelFromMessage(const std::string &message)
{
    std::istringstream iss(message);
    std::string command, channelName;
    iss >> command >> channelName;

    if (_channels.find(channelName) != _channels.end())
    {
        return _channels[channelName];
    }
    return NULL;
}

/**
 * Process a command received from a client
 */
void Server::handleClientMessage(Client *client)
{
    /* Mettre à jour le temps de la dernière activité */
    client->updateLastActivity();

    char buffer[1024];
    int bytesRead = recv(client->getSocket(), buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0)
    {
        if (bytesRead == 0)
        {
            std::cout << "Le client " << client->getSocket() << " a fermé la connexion." << std::endl;
        }
        else
        {
            std::cerr << "Erreur lors de la réception des données du client " << client->getSocket() << std::endl;
        }
        removeClient(client);
    }
    else
    {
        buffer[bytesRead] = '\0';
        std::string& messageBuffer = client->getMessageBuffer();
        messageBuffer.append(buffer);

        std::string::size_type pos;
        while ((pos = messageBuffer.find('\n')) != std::string::npos)
        {
            std::string message = messageBuffer.substr(0, pos);
            messageBuffer.erase(0, pos + 1);

            if (message.find("PONG") == 0)
            {
                client->setPingStatus(true);
            }
            
            /* Supprimer le retour chariot s'il est présent */
            else if (!message.empty() && message[message.size() - 1] == '\r')
            {
                message.erase(message.size() - 1);
            }

            /* Traiter le message complet */
            processCommand(client, message);
            
            /* Pass the message to the bot for moderation */
            Channel *channel = getChannelFromMessage(message);
            if (channel)
            {
                _bot.handleMessage(client, channel, message);
            }
        }
    }
}

/**
 * Process a Mode command received from a client
 */
void Server::handleModeCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS MODE :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = params[1];

    /* Vérifier que le canal existe */
    if (_channels.find(channelName) == _channels.end())
    {
        std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    Channel *channel = _channels[channelName];

    /* Si aucun autre paramètre, renvoyer les modes actuels du canal */
    if (params.size() == 2)
    {
        std::string modes = "+";
        if (channel->hasMode('i')) modes += "i";
        if (channel->hasMode('t')) modes += "t";
        if (channel->hasMode('k')) modes += "k";
        if (channel->hasMode('l')) modes += "l";
        std::string response = "324 " + client->getNickname() + " " + channelName + " " + modes + "\r\n";
        send(client->getSocket(), response.c_str(), response.length(), 0);
        return;
    }

    /* Vérifier que le client est opérateur du canal */
    if (!channel->isOperator(client))
    {
        std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string modeString = params[2];
    size_t paramIndex = 3;
    bool adding = true;

    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char modeChar = modeString[i];
        if (modeChar == '+')
        {
            adding = true;
        } else if (modeChar == '-')
        {
            adding = false;
        } else if (modeChar == 'i' || modeChar == 't')
        {
            if (adding)
            {
                channel->setMode(modeChar);
            } else
            {
                channel->unsetMode(modeChar);
            }
        } else if (modeChar == 'k')
        {
            if (adding)
            {
                if (params.size() <= paramIndex)
                {
                    std::string error = "461 ERR_NEEDMOREPARAMS MODE :Not enough parameters for mode k\r\n";
                    send(client->getSocket(), error.c_str(), error.length(), 0);
                    return;
                }
                std::string key = params[paramIndex++];
                channel->setKey(key);
            } else
            {
                channel->unsetKey();
            }
        } else if (modeChar == 'l')
        {
            if (adding)
            {
                if (params.size() <= paramIndex)
                {
                    std::string error = "461 ERR_NEEDMOREPARAMS MODE :Not enough parameters for mode l\r\n";
                    send(client->getSocket(), error.c_str(), error.length(), 0);
                    return;
                }
                int limit = atoi(params[paramIndex++].c_str());
                channel->setUserLimit(limit);
            } else
            {
                channel->unsetUserLimit();
            }
        } else if (modeChar == 'o')
        {
            if (params.size() <= paramIndex)
            {
                std::string error = "461 ERR_NEEDMOREPARAMS MODE :Not enough parameters for mode o\r\n";
                send(client->getSocket(), error.c_str(), error.length(), 0);
                return;
            }
            std::string nick = params[paramIndex++];
            Client *targetClient = NULL;
            for (size_t j = 0; j < _clients.size(); ++j)
            {
                if (_clients[j]->getNickname() == nick)
                {
                    targetClient = _clients[j];
                    break;
                }
            }
            if (!targetClient || !channel->hasClient(targetClient))
            {
                std::string error = "441 ERR_USERNOTINCHANNEL " + nick + " " + channelName + " :They aren't on that channel\r\n";
                send(client->getSocket(), error.c_str(), error.length(), 0);
                return;
            }
            if (adding)
            {
                channel->addOperator(targetClient);
            } else
            {
                channel->removeOperator(targetClient);
            }
        } else
            {
            std::string error = "472 ERR_UNKNOWNMODE " + std::string(1, modeChar) + " :is unknown mode char to me\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }
    }

    /* Notifier les membres du canal des changements de modes */
    std::string modeChangeMsg = ":" + client->getNickname() + " MODE " + channelName + " " + modeString;
    for (size_t i = 3; i < paramIndex; ++i)
    {
        modeChangeMsg += " " + params[i];
    }
    
    modeChangeMsg += "\r\n";

    const std::vector<Client*> &channelClients = channel->getClients();
    for (size_t i = 0; i < channelClients.size(); ++i)
    {
        send(channelClients[i]->getSocket(), modeChangeMsg.c_str(), modeChangeMsg.length(), 0);
    }
}

/**
 * Process a Invite command received from a client
 */
void Server::handleInviteCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 3)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS INVITE :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string targetNick = params[1];
    std::string channelName = params[2];

    /* Vérifier que le canal existe */
    if (_channels.find(channelName) == _channels.end())
    {
        std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    Channel *channel = _channels[channelName];

    /* Vérifier que le client est dans le canal */
    if (!channel->hasClient(client))
    {
        std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Vérifier que le client est opérateur si le canal est en mode 'i' */
    if (channel->hasMode('i') && !channel->isOperator(client))
    {
        std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Trouver le client cible */
    Client *targetClient = NULL;
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        if (_clients[i]->getNickname() == targetNick)
        {
            targetClient = _clients[i];
            break;
        }
    }

    if (!targetClient)
    {
        std::string error = "401 ERR_NOSUCHNICK " + targetNick + " :No such nick/channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Vérifier si le client cible est déjà dans le canal */
    if (channel->hasClient(targetClient))
    {
        std::string error = "443 ERR_USERONCHANNEL " + targetNick + " " + channelName + " :is already on channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Ajouter le client cible à la liste des invités */
    channel->inviteClient(targetClient);

    /* Informer le client cible de l'invitation */
    std::string inviteMsg = ":" + client->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n";
    send(targetClient->getSocket(), inviteMsg.c_str(), inviteMsg.length(), 0);

    /* Informer le client qui invite que l'invitation a été envoyée */
    std::string reply = "341 " + client->getNickname() + " " + targetNick + " :" + channelName + "\r\n";
    send(client->getSocket(), reply.c_str(), reply.length(), 0);
}

/**
 * Process a Topic command received from a client
 */
void Server::handleTopicCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS TOPIC :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = params[1];

    /* Vérifier que le canal existe */
    if (_channels.find(channelName) == _channels.end())
    {
        std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    Channel *channel = _channels[channelName];

    /* Vérifier que le client est dans le canal */
    if (!channel->hasClient(client))
    {
        std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Si aucun sujet n'est fourni, renvoyer le sujet actuel */
    if (params.size() == 2)
    {
        if (channel->hasTopic())
        {
            std::string response = "332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
            send(client->getSocket(), response.c_str(), response.length(), 0);
        } else
        {
            std::string response = "331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
            send(client->getSocket(), response.c_str(), response.length(), 0);
        }
        return;
    }

    /* Vérifier les permissions si le canal est en mode 't' */
    if (channel->hasMode('t') && !channel->isOperator(client))
    {
        std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Construire le nouveau sujet */
    std::string topic = params[2];
    if (topic[0] == ':')
    {
        topic = topic.substr(1);
    }
    for (size_t i = 3; i < params.size(); ++i)
    {
        topic += " " + params[i];
    }

    /* Définir le nouveau sujet */
    channel->setTopic(topic);

    /* Notifier les membres du canal */
    std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
    const std::vector<Client*> &channelClients = channel->getClients();
    for (size_t i = 0; i < channelClients.size(); ++i)
    {
        send(channelClients[i]->getSocket(), topicMsg.c_str(), topicMsg.length(), 0);
    }
}

/**
 * Process a Kick command received from a client
 */
void Server::handleKickCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 3)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS KICK :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = params[1];
    std::string targetNick = params[2];

    /* Construire le commentaire si fourni */
    /* Par défaut, le pseudonyme de l'expéditeur */
    std::string comment = client->getNickname();
    if (params.size() >= 4)
    {
        comment = params[3];
        if (comment[0] == ':')
        {
            comment = comment.substr(1);
        }
        for (size_t i = 4; i < params.size(); ++i)
        {
            comment += " " + params[i];
        }
    }

    /* Vérifier que le canal existe */
    if (_channels.find(channelName) == _channels.end())
    {
        std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    Channel *channel = _channels[channelName];

    /* Vérifier que le client est dans le canal */
    if (!channel->hasClient(client))
    {
        std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Vérifier que le client est opérateur du canal */
    if (!channel->isOperator(client))
    {
        std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Trouver le client cible */
    Client *targetClient = NULL;
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        if (_clients[i]->getNickname() == targetNick)
        {
            targetClient = _clients[i];
            break;
        }
    }

    if (!targetClient)
    {
        std::string error = "401 ERR_NOSUCHNICK " + targetNick + " :No such nick/channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Vérifier que le client cible est dans le canal */
    if (!channel->hasClient(targetClient))
    {
        std::string error = "441 ERR_USERNOTINCHANNEL " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Notifier les membres du canal du kick */
    std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName + " " + targetNick + " :" + comment + "\r\n";
    const std::vector<Client*> &channelClients = channel->getClients();
    for (size_t i = 0; i < channelClients.size(); ++i)
    {
        send(channelClients[i]->getSocket(), kickMsg.c_str(), kickMsg.length(), 0);
    }

    /* Retirer le client cible du canal */
    channel->removeClient(targetClient);
    targetClient->leaveChannel(channel);

    /* Si le canal est vide, le supprimer */
    if (channel->getClients().empty())
    {
        _channels.erase(channelName);
        delete channel;
    }
}

/**
 * Process a Cap command received from a client
 */
void Server::handleCapCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        std::string nick = client->isRegistered() ? client->getNickname() : "*";
        std::string error = ":" + _serverName + " 461 " + nick + " CAP :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string subCommand = params[1];

    if (subCommand == "LS")
    {
        /* Laisser vide si aucune capacité n'est supportée */
        std::string capabilities = "";

        /* Gérer les capacités multi-lignes si le client le demande */
        if (params.size() >= 3 && params[2] == "302")
        {
            /* Gérer les capacités multi-lignes si nécessaire */
        }

        std::string nick = client->isRegistered() ? client->getNickname() : "*";

        std::string response = ":" + _serverName + " CAP " + nick + " LS :" + capabilities + "\r\n";
        send(client->getSocket(), response.c_str(), response.length(), 0);
    } else if (subCommand == "END")
    {
        /* Le client termine la négociation des capacités */
        /* Aucune action spécifique requise pour une implémentation de base */
    } else
    {
        /* Sous-commande inconnue */
        std::string nick = client->isRegistered() ? client->getNickname() : "*";
        std::string error = ":" + _serverName + " 410 " + nick + " " + subCommand + " :Invalid CAP subcommand\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
    }
}

/**
 * Process a PONG command received from a client
 */
void Server::handlePongCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        /* Le paramètre <server> est manquant  */
        std::string error = ":" + _serverName + " 409 " + client->getNickname() + " :No origin specified\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Mettre à jour le temps du dernier PONG reçu */
    client->setLastPongTime(time(NULL));

    /* Optionnel : Vous pouvez également gérer le cas où <server2> est spécifié */
    if (params.size() >= 3)
    {
        std::string targetServer = params[2];
        /* Si vous avez une implémentation multi-serveur, vous devez retransmettre le PONG à targetServer */
        /* Dans le cas d'un micro serveur, vous pouvez ignorer ou loguer cette information */
    }

    /* Aucun autre traitement n'est nécessaire pour un PONG standard */
}

/**
 * Process a PING command received from a client
 */
void Server::handlePingCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        /* Le paramètre <server1> est manquant */
        std::string error = ":" + _serverName + " 409 " + client->getNickname() + " :No origin specified\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Extraire le token à partir de params[1] */
    std::string token = params[1];
    if (!token.empty() && token[0] == ':')
    {
        /* Supprimer le ':' initial si présent */
        token = token.substr(1);
    }

    /* Construire la réponse PONG conformément au protocole */
    std::string response = ":" + _serverName + " PONG " + _serverName + " :" + token + "\r\n";

    /* Envoyer la réponse au client */
    send(client->getSocket(), response.c_str(), response.length(), 0);
}

/**
 * Retourne le client avec le pseudonyme donné, ou NULL s'il n'existe pas.
 */
Client* Server::getClientByNickname(const std::string &nickname)
{
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        if (_clients[i]->getNickname() == nickname)
        {
            return _clients[i];
        }
    }
    return NULL;
}

/**
 * Process a Privmsg command received from a client
 */
void Server::processCommand(Client *client, const std::string &message)
{
    /* Découpage de la commande et des paramètres */
    std::vector<std::string> tokens = split(message, " ");
    if (tokens.empty())
    {
        return;
    }

    std::string command = tokens[0];
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "CAP")
    {
        handleCapCommand(client, tokens);
    } else if (command == "PASS")
    {
        handlePassCommand(client, tokens);
    } else if (command == "NICK")
    {
        handleNickCommand(client, tokens);
    } else if (command == "QUIT")
    {
        removeClient(client);
    } else if (command == "USER")
    {
        handleUserCommand(client, tokens);
    } else if (command == "PING")
    {
        handlePingCommand(client, tokens);
    } else if (command == "PONG")
    {
        handlePongCommand(client, tokens);
    } else if (!client->isRegistered())
    {
        /* Le client doit être enregistré avant de pouvoir utiliser d'autres commandes */
        std::string error = "451 ERR_NOTREGISTERED :You have not registered\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
    } else if (command == "JOIN")
    {
        handleJoinCommand(client, tokens);
    } else if (command == "PART")
    {
        handlePartCommand(client, tokens);
    } else if (command == "PRIVMSG")
    {
        handlePrivmsgCommand(client, tokens);
    } else if (command == "MODE")
    {
        handleModeCommand(client, tokens);
    } else if (command == "INVITE")
    {
        handleInviteCommand(client, tokens);
    } else if (command == "TOPIC")
    {   
        handleTopicCommand(client, tokens);
    } else if (command == "KICK")
    {
        handleKickCommand(client, tokens);
    } else
    {
        /* Commande inconnue */
        std::string error = "421 ERR_UNKNOWNCOMMAND " + command + " :Unknown command\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
    }

    /* Vérifier si le client est maintenant enregistré */
    registerClient(client);
}

/**
 * Enregistre le client si toutes les conditions de connexion sont remplies.
 */
void Server::registerClient(Client *client)
{
    if (!client->isRegistered() && client->hasSentPass() && client->hasSentNick() && client->hasSentUser())
    {
        client->setRegistered(true);
        sendMotd(client);
    }
}

/**
 * Gère la commande PASS reçue d'un client.
 */
void Server::handlePassCommand(Client *client, const std::vector<std::string> &params)
{
    if (client->hasSentPass())
    {
        std::string error = "462 ERR_ALREADYREGISTRED :You may not reregister\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    if (params.size() < 2)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS PASS :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string password = params[1];

    if (password != _password)
    {
        std::string error = "464 ERR_PASSWDMISMATCH :Password incorrect\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        removeClient(client);
        return;
    }

    client->setSentPass(true);
}

/**
 * Vérifie si le pseudonyme est valide.
 */
bool Server::isValidNickname(const std::string &nickname)
{
    if (nickname.empty() || nickname.length() > 9)
        return false;

    /* Caractères spéciaux autorisés */
    std::string specialChars = "-[]\\`^{}|";

    /* Vérification du premier caractère */
    char firstChar = nickname[0];
    if (!std::isalpha(firstChar) && specialChars.find(firstChar) == std::string::npos)
        return false;

    /* Vérification des caractères restants */
    for (size_t i = 1; i < nickname.length(); ++i)
    {
        char c = nickname[i];
        if (!std::isalnum(c) && specialChars.find(c) == std::string::npos)
            return false;
    }

    return true;
}

/**
 * Process a Nick command received from a client
 */
void Server::handleNickCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2) {
        std::string error = "431 ERR_NONICKNAMEGIVEN :No nickname given\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string newNickname = params[1];

    /* Validation du pseudonyme */
    if (!isValidNickname(newNickname))
    {
        std::string error = "432 ERR_ERRONEUSNICKNAME " + newNickname + " :Erroneous nickname\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Vérifier si le pseudonyme est déjà utilisé */
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if ((*it)->getNickname() == newNickname && *it != client)
        {
            std::string error = "433 ERR_NICKNAMEINUSE " + newNickname + " :Nickname is already in use\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }
    }

    /* Si le client est déjà enregistré, notifier les autres clients du changement de pseudonyme */
    if (client->isRegistered())
    {
        std::string nickChangeMsg = ":" + client->getNickname() + " NICK :" + newNickname + "\r\n";
        for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        {
            if (*it != client && (*it)->isRegistered())
            {
                send((*it)->getSocket(), nickChangeMsg.c_str(), nickChangeMsg.length(), 0);
            }
        }
    }

    /* Mettre à jour le pseudonyme du client */
    client->setNickname(newNickname);
    std::cout << "Client " << client->getSocket() << " changed nickname to " << newNickname << std::endl << std::endl;

    /* Si le client n'était pas encore enregistré, définir le drapeau SentNick */
    if (!client->isRegistered())
    {
        client->setSentNick(true);
    }
}

/**
 * Vérifie si le nom d'utilisateur est valide.
 */
bool Server::isValidUsername(const std::string &username)
{
    /* Limite de longueur arbitraire */
    if (username.empty() || username.length() > 10) 
        return false;

    for (size_t i = 0; i < username.length(); ++i)
    {
        char c = username[i];
        if (!std::isalnum(c) && c != '_' && c != '-')
            return false;
    }

    return true;
}

/**
 * Process a User command received from a client
 */
void Server::handleUserCommand(Client *client, const std::vector<std::string> &params)
{
    if (client->hasSentUser())
    {
        std::string error = "462 ERR_ALREADYREGISTRED :You may not reregister\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    if (params.size() < 5)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS USER :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string username = params[1];
    std::string hostname = params[2];
    std::string servername = params[3];
    std::string realname = params[4];

    /* Si le realname est précédé de ':' et contient des espaces */
    if (realname[0] == ':')
    {
        realname = realname.substr(1);
        for (size_t i = 5; i < params.size(); ++i)
        {
            realname += " " + params[i];
        }
    }

    /* Validation du username */
    if (!isValidUsername(username))
    {
        std::string error = "432 ERR_ERRONEUSNICKNAME " + username + " :Erroneous username\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Définir le nom d'hôte, utiliser une valeur par défaut si nécessaire */
    if (hostname.empty())
    {
        /* Valeur par défaut si le nom d'hôte n'est pas fourni */
        hostname = "unknown";
    }

    client->setUsername(username);
    client->setHostname(hostname);
    client->setServername(servername);
    client->setRealname(realname);
    client->setSentUser(true);

}

/**
 * Send the Message of the Day (MOTD) to a client
 */
void Server::sendMotd(Client *client)
{
    std::string nick = client->getNickname();
    std::string username = client->getUsername();

    /* 001 RPL_WELCOME */
    std::string welcome = ":" + _serverName + " 001 " + nick + " :Welcome to the Internet Relay Network " + client->getNickname() + "!" + client->getRealname() + "@" + getServerIp() + "\r\n";
    send(client->getSocket(), welcome.c_str(), welcome.length(), 0);

    /* 002 RPL_YOURHOST */
    std::string yourHost = ":" + _serverName + " 002 " + nick + " :Your host is " + _serverName + ", running version 1.0\r\n";
    send(client->getSocket(), yourHost.c_str(), yourHost.length(), 0);

    /* 003 RPL_CREATED */
    std::string created = ":" + _serverName + " 003 " + nick + " :This server was created at some point in the past\r\n";
    send(client->getSocket(), created.c_str(), created.length(), 0);

    /* 004 RPL_MYINFO */
    std::string myInfo = ":" + _serverName + " 004 " + nick + " " + _serverName + " 1.0 o o\r\n";
    send(client->getSocket(), myInfo.c_str(), myInfo.length(), 0);

    /* Envoyer le MOTD (Message of the Day) */
    std::string motdStart = ":" + _serverName + " 375 " + nick + " :- " + _serverName + " Message of the Day - \r\n";
    send(client->getSocket(), motdStart.c_str(), motdStart.length(), 0);

    std::string motd = ":" + _serverName + " 372 " + nick + " :- Welcome to our IRC server!\r\n";
    send(client->getSocket(), motd.c_str(), motd.length(), 0);

    std::string motdEnd = ":" + _serverName + " 376 " + nick + " :End of /MOTD command.\r\n";
    send(client->getSocket(), motdEnd.c_str(), motdEnd.length(), 0);
}

/**
 * Remove a client from the server
 */
void Server::removeClient(Client *client)
{
    close(client->getSocket());
    FD_CLR(client->getSocket(), &_masterSet);
    _clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());
    delete client;
}

/**
 * Split a string into tokens using a delimiter
 */
std::vector<std::string> Server::split(const std::string &str, const std::string &delim)
{
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos - prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

void Server::sendNamesReply(Client *client, Channel *channel)
{
    /* Construire la liste des pseudonymes */
    std::string nickList;
    const std::vector<Client*> &clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        /* Inclure les préfixes pour les opérateurs ('@') ou les utilisateurs avec voix ('+') */
        if (channel->isOperator(clients[i]))
        {
            nickList += "@";
        }
        nickList += clients[i]->getNickname() + " ";
    }

    /* RPL_NAMREPLY (353) */
    std::string namesReply = ":" + _serverName + " 353 " + client->getNickname() + " = " +
        channel->getName() + " :" + nickList + "\r\n";
    send(client->getSocket(), namesReply.c_str(), namesReply.length(), 0);

    /* RPL_ENDOFNAMES (366) */
    std::string endOfNames = ":" + _serverName + " 366 " + client->getNickname() + " " +
        channel->getName() + " :End of /NAMES list.\r\n";
    send(client->getSocket(), endOfNames.c_str(), endOfNames.length(), 0);
}

/**
 * Process a Join command received from a client
 */
void Server::handleJoinCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS JOIN :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = params[1];
    std::string key = "";
    if (params.size() >= 3)
    {
        key = params[2];
    }

    /* Vérifier que le nom du canal commence par '#' ou '&' */
    if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
    {
        std::string error = "476 ERR_BADCHANMASK " + channelName + " :Bad Channel Mask\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    Channel *channel = NULL;

    /* Si le canal n'existe pas, le créer */
    if (_channels.find(channelName) == _channels.end())
    {
        channel = new Channel(channelName);
        _channels[channelName] = channel;
        /* Le premier client à rejoindre le canal devient opérateur */
        channel->addOperator(client);
    } else
    {
        channel = _channels[channelName];

        /* Vérifier le mode 'i' (invitation uniquement) */
        if (channel->hasMode('i') && !channel->isInvited(client))
        {
            std::string error = "473 ERR_INVITEONLYCHAN " + channelName + " :Cannot join channel (+i)\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }

        /* Vérifier le mode 'k' (clé du canal) */
        if (channel->hasMode('k') && !channel->checkKey(key))
        {
            std::string error = "475 ERR_BADCHANNELKEY " + channelName + " :Cannot join channel (+k)\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }

        /* Vérifier le mode 'l' (limite d'utilisateurs) */
        if (channel->isFull())
        {
            std::string error = "471 ERR_CHANNELISFULL " + channelName + " :Cannot join channel (+l)\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }
    }

    /* Ajouter le client au canal */
    channel->addClient(client);
    client->joinChannel(channel);

    /* Supprimer l'invitation si elle existait */
    channel->removeInvitation(client);

    /* Notifier les autres clients du canal */
    std::string joinMsg = ":" + client->getNickname() + "!" + client->getRealname() + "@" + getServerIp() + " JOIN :" + channelName + "\r\n";
    std::cout << "Message JOIN envoyé " << joinMsg << std::endl;
    const std::vector<Client*> &channelClients = channel->getClients();
    for (size_t i = 0; i < channelClients.size(); ++i)
    {
        send(channelClients[i]->getSocket(), joinMsg.c_str(), joinMsg.length(), 0);
    }

    /* Envoyer le sujet si défini (RPL_TOPIC ou RPL_NOTOPIC) */
    if (channel->hasTopic())
    {
        std::string topicMsg = ":" + _serverName + " 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
        send(client->getSocket(), topicMsg.c_str(), topicMsg.length(), 0);
    } else
    {
        std::string noTopicMsg = ":" + _serverName + " 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
        send(client->getSocket(), noTopicMsg.c_str(), noTopicMsg.length(), 0);
    }

    /* Envoyer la liste des NOMS au client */
    sendNamesReply(client, channel);
}

/**
 * Process a Part command received from a client
 */
void Server::handlePartCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        std::string error = "461 ERR_NEEDMOREPARAMS PART :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string channelName = params[1];

    /* Vérifier que le canal existe */
    if (_channels.find(channelName) == _channels.end())
    {
        std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    Channel *channel = _channels[channelName];

    /* Vérifier que le client est dans le canal */
    if (!channel->hasClient(client))
    {
        std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Notifier les autres clients du canal */
    std::string partMsg = ":" + client->getNickname() + "!" + client->getRealname() + "@" + client->getHostname() + " PART " + channelName + "\r\n";
    std::cout << "Message PART envoyé : " << partMsg << std::endl;
    const std::vector<Client*> &channelClients = channel->getClients();
    for (size_t i = 0; i < channelClients.size(); ++i)
    {
        send(channelClients[i]->getSocket(), partMsg.c_str(), partMsg.length(), 0);
    }

    /* Retirer le client du canal */
    channel->removeClient(client);
    client->leaveChannel(channel);

    /* Si le canal est vide, le supprimer */
    if (channel->getClients().empty())
    {
        _channels.erase(channelName);
        delete channel;
    }
}

/**
 * Process a DCC SEND command received from a client
 */
void Server::handleDCCSendCommand(Client *client, const std::vector<std::string> &ctcpParams, const std::string &targetNick)
{
    /* Commande DCC SEND invalide */
    if (ctcpParams.size() < 5)
    {
        return;
    }

    std::string filename = ctcpParams[2];
    uint32_t filesize = 0;
    if (ctcpParams.size() >= 6)
        filesize = static_cast<uint32_t>(atoi(ctcpParams[5].c_str()));

    /* Trouver le destinataire */
    Client *receiver = getClientByNickname(targetNick);
    if (!receiver)
    {
        std::string error = ":" + _serverName + " 401 " + client->getNickname() + " " + targetNick + " :No such nick/channel\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    /* Créer le transfert DCC */
    DCCTransfer *transfer = new DCCTransfer(this, client, receiver, filename, filesize);
    if (!transfer->start())
    {
        delete transfer;
        /* Échec du démarrage du transfert */
        return;
    }

    /* Ajouter le transfert au gestionnaire */
    _dccManager.addTransfer(transfer);

    /* Informer le destinataire */
    std::string serverIp = _serverIp;
    uint16_t serverPort = transfer->getListenPort();

    std::ostringstream oss;
    oss << "\x01"
        << "DCC SEND " << filename << " " << serverIp << " " << serverPort << " " << filesize << "\x01";
    std::string dccSendMsg = ":" + client->getNickname() + " PRIVMSG " + receiver->getNickname() + " :" + oss.str() + "\r\n";

    /* Pour vérification, afficher le message DCC SEND */
    std::cout << "DCC SEND message sent to receiver: " << dccSendMsg << std::endl;

    send(receiver->getSocket(), dccSendMsg.c_str(), dccSendMsg.length(), 0);
}

/**
 * Implement handleDCCAcceptCommand if needed
 */
void Server::handleDCCAcceptCommand(Client *client, const std::vector<std::string> &ctcpParams)
{
    (void)ctcpParams;
    /* For now, you can leave it empty or add a placeholder */
    std::cout << "Received DCC ACCEPT command from " << client->getNickname() << std::endl;
}

/**
 * Process a DCC command received from a client
 */
void Server::handleDCCCommand(Client *client, const std::vector<std::string> &ctcpParams, const std::string &target)
{
    /* Commande DCC invalide */
    if (ctcpParams.size() < 2)
    {
        return;
    }

    std::string dccCommand = ctcpParams[1];

    if (dccCommand == "SEND")
    {
        handleDCCSendCommand(client, ctcpParams, target);
    }
    else if (dccCommand == "ACCEPT")
    {
        handleDCCAcceptCommand(client, ctcpParams);
    }
    /* ... (gestion d'autres commandes DCC si nécessaire) */
}

/**
 * Process a Privmsg command received from a client
 */
void Server::handlePrivmsgCommand(Client *client, const std::vector<std::string> &params)
{
    if (params.size() < 3)
    {
        std::string error = ":" + _serverName + " 461 " + client->getNickname() + " PRIVMSG :Not enough parameters\r\n";
        send(client->getSocket(), error.c_str(), error.length(), 0);
        return;
    }

    std::string target = params[1];
    std::string message = params[2];

    /* Si le message commence par ':', concaténer les paramètres suivants */
    if (message[0] == ':')
    {
        message = message.substr(1);
        for (size_t i = 3; i < params.size(); ++i)
        {
            message += " " + params[i];
        }
    }

    /* Détection des messages CTCP */
    bool isCTCP = false;
    if (!message.empty() && message[0] == '\x01' && message[message.size() - 1] == '\x01')
    {
        isCTCP = true;
        std::string ctcpMessage = message.substr(1, message.size() - 2);
        std::vector<std::string> ctcpParams = split(ctcpMessage, " ");
        std::string ctcpCommand = ctcpParams[0];

        /* Gérer les commandes CTCP */
        if (ctcpCommand == "DCC")
        {
            handleDCCCommand(client, ctcpParams, target);
            return;
        }

        /* Gérer la commande CTCP PING */
        if (ctcpCommand == "PING")
        {
            /* Répondre au CTCP PING */
            std::string response = ":" + client->getNickname() + " NOTICE " + target + " :\x01PING";
            if (ctcpParams.size() > 1)
            {
                /* Inclure le timestamp */
                response += " " + ctcpParams[1];
            }
            response += "\x01\r\n";

            /* Envoyer la réponse au client */
            send(client->getSocket(), response.c_str(), response.length(), 0);
            return;
        }
        /* Gérer d'autres commandes CTCP si nécessaire */
    }

    std::string fullMsg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";

    if (target[0] == '#' || target[0] == '&')
    {
        /* Cible est un canal */
        if (_channels.find(target) == _channels.end())
        {
            std::string error = ":" + _serverName + " 403 " + client->getNickname() + " " + target + " :No such channel\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }

        Channel *channel = _channels[target];

        /* Vérifier que le client est dans le canal */
        if (!channel->hasClient(client))
        {
            std::string error = ":" + _serverName + " 404 " + client->getNickname() + " " + target + " :Cannot send to channel\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }

        /* Envoyer le message aux autres clients du canal */
        const std::vector<Client*> &channelClients = channel->getClients();
        for (size_t i = 0; i < channelClients.size(); ++i)
        {
            if (channelClients[i] != client)
            {
                send(channelClients[i]->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
            }
        }
    } else
    {
        /* Cible est un utilisateur */
        Client *targetClient = NULL;
        for (size_t i = 0; i < _clients.size(); ++i)
        {
            if (_clients[i]->getNickname() == target)
            {
                targetClient = _clients[i];
                break;
            }
        }

        /* Vérifier si le client cible existe */
        if (!targetClient)
        {
            std::string error = ":" + _serverName + " 401 " + client->getNickname() + " " + target + " :No such nick/channel\r\n";
            send(client->getSocket(), error.c_str(), error.length(), 0);
            return;
        }

        /* Si c'est un CTCP, n'envoyer le message qu'au destinataire */
        if (isCTCP)
        {
            send(targetClient->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
        } else
        {
            /* Envoyer le message au client cible */
            send(targetClient->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
        }
    }
}

/**
 * Getter 
 */
fd_set& Server::getMasterSet() {
    return _masterSet;
}

/**
 * Get the maximum file descriptor
 */
int Server::getFdMax() const {
    return _fdMax;
}

/**
 * Get the server name
 */
const std::string& Server::getServerName() const {
    return _serverName;
}

/**
 * 
 */
DCCManager& Server::getDCCManager() {
    return _dccManager;
}

/**
 * Set the maximum file descriptor
 */
void Server::setFdMax(int fd) {
    if (fd > _fdMax) {
        _fdMax = fd;
    }
}

void Server::sendPing()
{
    /* Message PING envoyé aux clients pour vérifier l'activité */
    std::string pingMsg = "PING :" + _serverName + "\r\n";
    
    /* Envoie le PING à chaque client et marque comme "non-répondant" */
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        send(_clients[i]->getSocket(), pingMsg.c_str(), pingMsg.length(), 0);

        /* Marquer le client comme "non-répondant" */
        _clients[i]->setPingStatus(false);
    }
}

void Server::checkPingResponses()
{
    /* Parcourt chaque client pour vérifier s'il a répondu au PING */
    for (size_t i = 0; i < _clients.size(); ++i)
    {
        /* Vérifier si le client a répondu au PING */
        if (!_clients[i]->isPingReceived())
        {
            /* Le client n'a pas répondu, le déconnecter */
            std::cout << "Client inactif, déconnexion du socket : " << _clients[i]->getSocket() << std::endl;

            /* Fermer le socket du client */
            close(_clients[i]->getSocket());

            /* Supprimer du set maître */
            FD_CLR(_clients[i]->getSocket(), &_masterSet);

            /* Libérer la mémoire allouée pour le client */
            delete _clients[i];

            /* Retirer de la liste des clients */
            _clients.erase(_clients.begin() + i);

            /* Réajuster l'index après suppression */
            --i;
        }
    }
}

std::string & Server::getServerIp()
{
    return _serverIp;
}
