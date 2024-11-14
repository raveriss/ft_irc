/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:07:13 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/14 02:20:47 by raveriss         ###   ########.fr       */
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
	if (signal == CTRL_C)
		signalName = "SIGINT (ctrl + c)";
	else if (signal == CTRL_Z)
		signalName = "SIGTSTP (ctrl + z)";
	else
		signalName = "Unknown";

	std::cout << "\nSignal " << signalName << " reçu, fermeture du serveur..." << std::endl;
	if (instance)
		instance->shutdown();
	exit(0);
}

/**
 * Ferme le serveur proprement
 */
void Server::shutdown()
{

	/* Libération des clients */
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		delete *it;
	}
	_clients.clear();

	/* Libération des clients dans _clientsToRemove */
	for (std::vector<Client*>::iterator it = _clientsToRemove.begin(); it != _clientsToRemove.end(); ++it)
	{
		delete *it;
	}
	_clientsToRemove.clear();

	/* Réinitialise complètement la capacité des vecteurs */
	std::vector<Client*>().swap(_clients);
	std::vector<Client*>().swap(_clientsToRemove);

	/* Libération des canaux */
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) 
	{
		delete it->second;
	}
	_channels.clear();

	/* Fermer le socket d'écoute */
	if (_listenSocket >= 0) 
	{
		close(_listenSocket);
		_listenSocket = -1;
	}

	/* Libération du Bot */
	_bot.~Bot();

	/* Libération des descripteurs de fichiers */
	FD_ZERO(&_masterSet);
	_fdMax = 0;

	/* Libération de l'instance statique */
	instance = NULL;

	/* Fermer tous les descripteurs de fichiers ouverts */
	for (int fd = 0; fd <= _fdMax; ++fd)
	{
		if (FD_ISSET(fd, &_masterSet))
			close(fd);
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

	/**
	 * Initialise un gestionnaire de signaux pour permettre une fermeture propre du serveur.
	 * Configuration pour intercepter SIGINT (Ctrl+C) et SIGTSTP (Ctrl+Z).
	 * `sa` for S.igA.ction
	 */
	struct sigaction sa;

    /**
     * Définit la fonction de gestion des signaux.
     * Lorsqu'un signal est reçu, Server::handleSignal sera appelé pour une fermeture propre.
     */
	sa.sa_handler = &Server::handleSignal;

    /**
     * Initialise le masque de signaux à vide, ne bloque aucun autre signal.
     * Le serveur pourra continuer à recevoir d'autres signaux pendant l'exécution de handleSignal.
     */
	sigemptyset(&sa.sa_mask);

    /**
     * Définit le drapeau SA_RESTART pour relancer les appels système interrompus.
     * Cela assure que certaines fonctions bloquantes, comme accept(), reprennent automatiquement.
     */
	sa.sa_flags = SA_RESTART;

    /**
     * Configure les signaux SIGINT et SIGTSTP pour utiliser la structure sa.
     * Si l'une des configurations échoue, lance une exception.
     */
	if (sigaction(SIGINT, &sa, NULL) == FAILURE || sigaction(SIGTSTP, &sa, NULL) == FAILURE)
		throw std::runtime_error("Erreur lors de la configuration du signal SIGINT.");
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
	_listenSocket = socket(IPV4, TCP, 0);
	if (_listenSocket < 0)
		throw std::runtime_error("Erreur lors de la création du socket d'écoute.");

	/* Configuration pour réutiliser l'adresse et le port */
	int opt = OPT_ON;
	if (setsockopt(_listenSocket, TCP, REUSE_ADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Erreur lors de la configuration du socket.");

	/* Déclare la structure d'adresse du serveur */
	sockaddr_in serverAddr;

	/* Initialise la structure avec des zéros */
	std::memset(&serverAddr, 0, sizeof(serverAddr));

	/**
	 * `sin` for S.ocket IN.ternet
	 */
	
	/* Définit la famille d'adresses à IPv4 */
	serverAddr.sin_family = IPV4;

	/* Définit l'adresse IP du serveur à INADDR_ANY */
	serverAddr.sin_addr.s_addr = ALL_ADDR;

	/* Définit le port du serveur en convertissant _port en format réseau (big-endian) avec htons (Host TO Network Short) */
	serverAddr.sin_port = htons(_port);

	/*
	* Associe le socket d'écoute (_listenSocket) à l'adresse IP et au port spécifiés dans serverAddr
	* en utilisant bind() (lie le descripteur de socket à une adresse locale) ;
	* lève une exception avec un message détaillé en cas d'échec
	*/
	if (bind(_listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error("Erreur lors de la liaison du socket : " + std::string(strerror(errno)));

	/* 
	* Met le socket en mode écoute pour accepter les connexions entrantes,
	* en spécifiant la taille maximale de la file d'attente avec MAX_CONNEXIONS
	* (nombre maximal de connexions en attente autorisées par le système) ;
	* lève une exception en cas d'échec
	*/
	if (listen(_listenSocket, MAX_CONNEXIONS) < 0)
		throw std::runtime_error("Erreur lors de l'écoute sur le socket.");

	/**
	 * _listenSocket : Socket principal d'écoute, utilisé pour accepter les nouvelles
	 *                 connexions entrantes des clients.
	 */

	/**
	 * _masterSet : Ensemble de descripteurs de fichiers, incluant le socket d'écoute
	 *              (_listenSocket) et les sockets clients actifs, surveillé par select().
	 */

	/* Initialise le set de descripteurs de fichiers pour les connexions */
	FD_ZERO(&_masterSet);

	/* Ajoute le socket d'écoute au set de descripteurs */
	FD_SET(_listenSocket, &_masterSet);

	/* Met à jour la valeur maximale des descripteurs de fichiers */
	_fdMax = _listenSocket;

    /**
	 * `if` for I.nterF.ace
	 * Utilise `getifaddrs` pour obtenir les adresses réseau (ifaddr) de chaque interface (if pour "interface").
     * `ifa` pointe sur chaque adresse dans la liste chaînée d'interfaces.
     * `ifa_next` permet de passer à l'interface suivante.
     * Vérifie chaque adresse avec `ifa_addr` pour exclure les locales (127.0.0.1)
     * et sélectionne la première IPv4 valide (sin_addr).
     */
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == FAILURE)
    {
        perror("getifaddrs");
		
		/* Utilise l'adresse locale par défaut */
        _serverIp = "127.0.0.1";
    }
    else
    {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL) continue;

			/* Vérifie si IPv4 */
            if (ifa->ifa_addr->sa_family == IPV4)
            {
                struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
                std::string ip = inet_ntoa(addr->sin_addr);

				/* Ignore l'adresse locale */
                if (ip != "127.0.0.1")
                {
                    _serverIp = ip;
                    break;
                }
            }
        }

		/* Libère la mémoire allouée par getifaddrs */
        freeifaddrs(ifaddr);
    }

    std::cout << "Serveur IRC démarré sur " << _serverIp << ":" << _port << std::endl;
}

/**
 * Run the server
 */
void Server::run()
{

	/* Boucle principale du serveur, tourne indéfiniment */
	while (true)
	{
        /** 
         * _masterSet :
         * Contient tous les descripteurs actifs à surveiller (ex. : socket d'écoute, clients connectés).
         * Initialisé au démarrage du serveur et mis à jour avec chaque connexion ou déconnexion.
		 * 
         * readSet :
         * Copie de _masterSet à chaque itération. Utilisé pour vérifier quels descripteurs sont prêts pour la lecture
         * (données des clients ou nouvelles connexions).
         */
		fd_set readSet = _masterSet;

        /** 
         * writeSet :
         * Ensemble temporaire pour les descripteurs prêts pour l'écriture. Initialisé à vide ici avec FD_ZERO.
         * Peut être configuré si le serveur gère l'écriture asynchrone vers les clients.
         */
		fd_set writeSet;
		FD_ZERO(&writeSet);
		
		/* Délai pour la fonction select */
		struct timeval timeout;

		/* Intervalle de 1 seconde pour déclencher régulièrement les vérifications PING */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

        /** 
         * Surveille l'activité sur les descripteurs dans readSet, jusqu'à _fdMax.
         * Timeout défini à 1 seconde pour des vérifications régulières.
         * Lance une exception en cas d'erreur.
         */
		if (select(_fdMax + 1, &readSet, NULL, NULL, &timeout) < 0)
			throw std::runtime_error("Erreur lors de la sélection des descripteurs.");

		/* Parcourt tous les descripteurs pour vérifier les événements */
		for (int fd = 0; fd <= _fdMax; ++fd)
		{            
			/** 
			 * Vérifie si le descripteur fd contient des données prêtes à être lues dans readSet.
			 * Indique qu'il y a des données en attente de traitement sur ce descripteur.
			 */
			if (FD_ISSET(fd, &readSet))
			{
                /** 
                 * Si le client se présente au _listenSocket, c'est un nouveau client.
                 * handleNewConnection() est appelé pour accepter cette connexion et
                 * lui attribuer un descripteur de socket spécifique.
                 */
				if (fd == _listenSocket)
				{
					/* Nouvelle connexion entrante */
					handleNewConnection();
				}
				
                /** 
                 * Sinon, le client est déjà connecté : on traite le message reçu.
                 */
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
						handleClientMessage(client);
				}
			}
		}

		/* Suppression différée des clients après la boucle principale */
		for (std::vector<Client*>::iterator it = _clientsToRemove.begin(); it != _clientsToRemove.end(); ++it)
		{
			delete *it;
		}
		_clientsToRemove.clear();
	}
}

std::string Server::getBackgroundColorCode(int socket)
{
	/* Génère un code de couleur de fond de 41 à 47 */
	int colorCode = 41 + (socket % 7);
	std::stringstream color;

	/* 1 pour le gras, 97 pour le texte en blanc vif */
	color << "\033[1;97;" << colorCode << "m";
	return color.str();
}

void Server::printClientInfo(int newSocket, const std::string& host)
{
	std::string colorCode = getBackgroundColorCode(newSocket);
	std::cout << colorCode
			  << "\nClient " << newSocket << ", IP " << host
			  << ". Cmds PASS, NICK et USER pour enregistrer le client."
			  << "\033[0m\033[K" << std::endl;
}

/**
 * Process a command received from a client
 */
void Server::handleNewConnection()
{
    /** 
     * Attribue un fd unique au nouveau client présenté sur _listenSocket.
     */
	int fdNewClient = accept(_listenSocket, NULL, NULL);
	if (fdNewClient == FAILURE)
	{
		perror("accept");
		return;
	}

    /** 
     * Prépare une structure pour stocker l'adresse du client, compatible avec IPv4 et IPv6.
     */
	struct sockaddr_storage addr;

    /** 
     * Initialise la taille de la structure addr, nécessaire pour getpeername().
     */
	socklen_t addr_len = sizeof(addr);

    /**
     * Récupère l'adresse du client connecté via fdNewClient et la stocke dans addr.
     * Utilise addr_len pour indiquer et ajuster la taille de l'adresse récupérée.
     */
	if (getpeername(fdNewClient, (struct sockaddr*)&addr, &addr_len) == FAILURE)
	{
		perror("getpeername");
		close(fdNewClient);
		return;
	}

    /** 
     * Tableau pour stocker l'adresse IP ou le nom d'hôte du client, taille max définie par NI_MAXHOST.
     */
	char host[NI_MAXHOST];

    /** 
     * Remplit host avec l'adresse IP du client en format numérique.
     * Retourne une erreur si la récupération de l'adresse échoue.
     */
	if (getnameinfo((struct sockaddr*)&addr, addr_len, host, sizeof(host), NULL, 0, NI_NUMERICHOST) != 0)
	{
		std::cerr << "Erreur lors de la récupération du nom d'hôte." << std::endl;
		close(fdNewClient);
		return;
	}

    /** 
     * Crée un nouvel objet Client associé à ce client connecté.
     * Le descripteur fdNewClient permet d'identifier et de gérer les interactions
     * avec ce client spécifique tout au long de la session.
     */
	Client *newClient = new Client(fdNewClient);

    /** 
     * Définit une valeur par défaut pour le nom d'hôte, "unknown".
     * Cette valeur est utilisée si l'adresse IP du client ne peut pas être obtenue.
     */
	std::string defaultHost = "unknown";

    /** 
     * Enregistre l'adresse IP obtenue pour ce client dans _serverIp.
     * Cela permet au serveur de conserver une référence directe à l'adresse IP
     * du client connecté, facilitant les vérifications ultérieures.
     */
	_serverIp = host;

    /** 
     * Vérifie si l'adresse IP (host) du client est valide et non vide.
     * Si une adresse est présente dans host, elle est définie comme le nom d'hôte
     * du client via setHostname(). Sinon, "127.0.0.1" est utilisée comme adresse
     * par défaut, indiquant une connexion locale ou une adresse inconnue.
     */
	if (host[0] != '\0')
		newClient->setHostname(host);
	else
		newClient->setHostname("127.0.0.1");

	/* Initialiser l'activité du client */
	newClient->updateLastActivity();

	/* Ajouter le client à la liste */
	_clients.push_back(newClient);

	/* Ajouter le socket au master set */
	FD_SET(fdNewClient, &_masterSet);
	if (fdNewClient > _fdMax)
		_fdMax = fdNewClient;

	printClientInfo(fdNewClient, host);
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
		return _channels[channelName];
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
			std::cout << "Le client " << client->getSocket() << " a fermé la connexion.\033[0m" << std::endl;
		else
			std::cerr << "Erreur lors de la réception des données du client " << client->getSocket() << std::endl;
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
			
			/* Supprimer le retour chariot s'il est présent */
			if (!message.empty() && message[message.size() - 1] == '\r')
				message.erase(message.size() - 1);

			/* Traiter le message complet */
			processCommand(client, message);
			
			/* Pass the message to the bot for moderation */
			Channel *channel = getChannelFromMessage(message);
			if (channel)
				_bot.handleMessage(client, channel, message);
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
	if (params.size() == PARAMS_REQUIRED)
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
			adding = true;
			
		else if (modeChar == '-')
			adding = false;
			
		else if (modeChar == 'i' || modeChar == 't')
		{
			if (adding)
				channel->setMode(modeChar);
			else
				channel->unsetMode(modeChar);
		}
		
		else if (modeChar == 'k')
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
			}
			
			else
				channel->unsetKey();
		}
		
		else if (modeChar == 'l')
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
			}
			
			else
				channel->unsetUserLimit();
		}
		
		else if (modeChar == 'o')
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
				channel->addOperator(targetClient);
			
			else
				channel->removeOperator(targetClient);
		}
		
		else
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
	if (params.size() == PARAMS_REQUIRED)
	{
		if (channel->hasTopic())
		{
			std::string response = "332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
			send(client->getSocket(), response.c_str(), response.length(), 0);
		}
		
		else
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
		topic = topic.substr(1);
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
			comment = comment.substr(1);
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

bool Server::send_message(const std::string &message, int sender_fd)
{
	std::string tmp = message;
	if (tmp.size() > 510)
		tmp = tmp.substr(0, 510) + "\r\n";
	send(sender_fd, tmp.c_str(), tmp.size(), 0);
	std::cout << "\n\033[43mMessage sent: " << tmp << "\033[0m";
	return true;
}

/**
 * Process a PONG command received from a client
 */
bool Server::handlePingPongCommand(Client *client, const std::string &args)
{
	std::string client_id = "\033[43m" + client->getNickname() + "!" + client->getUsername() + "@" + _serverIp + "\033[0m";
	std::string response;

	if (!client->isRegistered())
	{
		if (args.empty())
			response = PING(client_id, "");
		else
			response = PING(client_id, args);
	}
	else
	{
		if (args.empty())
			response = PONG(client_id, "");
		else
			response = PONG(client_id, args);
	}

	send_message(response, client->getSocket());
	return true;
}

/**
 * Retourne le client avec le pseudonyme donné, ou NULL s'il n'existe pas.
 */
Client* Server::getClientByNickname(const std::string &nickname)
{
	for (size_t i = 0; i < _clients.size(); ++i)
	{
		if (_clients[i]->getNickname() == nickname)
			return _clients[i];
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
		return;

	std::string command = tokens[0];
	std::transform(command.begin(), command.end(), command.begin(), ::toupper);

	if (command == "CAP")
		handleCapCommand(client, tokens);
	else if (command == "PASS")
		handlePassCommand(client, tokens);
	else if (command == "NICK")
		handleNickCommand(client, tokens);
	else if (command == "QUIT")
		removeClient(client);
	else if (command == "USER")
		handleUserCommand(client, tokens);
	else if (command == "PING")
		handlePingPongCommand(client, tokens.size() > 1 ? tokens[1] : "");
	else if (command == "PONG")
		handlePingPongCommand(client, tokens.size() > 1 ? tokens[1] : "");
	else if (!client->isRegistered())
	{
		/* Le client doit être enregistré avant de pouvoir utiliser d'autres commandes */
		std::string error = "451 ERR_NOTREGISTERED :You have not registered\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
	}
	else if (command == "JOIN")
		handleJoinCommand(client, tokens);
	else if (command == "PART")
		handlePartCommand(client, tokens);
	else if (command == "PRIVMSG")
		handlePrivmsgCommand(client, tokens);
	else if (command == "MODE")
		handleModeCommand(client, tokens);
	else if (command == "INVITE")
		handleInviteCommand(client, tokens);
	else if (command == "TOPIC")
		handleTopicCommand(client, tokens);
	else if (command == "KICK")
		handleKickCommand(client, tokens);
	else
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
	if (params.size() < 2)
	{
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
		std::string nickChangeMsg = ":" + client->getNickname() + " NICK :" + newNickname + "." + "\033[0m\r\n";
		for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (*it != client && (*it)->isRegistered())
				send((*it)->getSocket(), nickChangeMsg.c_str(), nickChangeMsg.length(), 0);
		}
	}

	/* Mettre à jour le pseudonyme du client */
	client->setNickname(newNickname);
	std::cout << getBackgroundColorCode(client->getSocket()) 
			<< "\nClient " << client->getSocket() 
			<< " changed nickname to " << newNickname 
			<< "\033[0m\033[K" << std::endl;
			
	/* Si le client n'était pas encore enregistré, définir le drapeau SentNick */
	if (!client->isRegistered())
		client->setSentNick(true);
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

	/* Ajouter à la liste de suppression différée */
	_clientsToRemove.push_back(client);
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
			nickList += "@";
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
		key = params[2];

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
	}
	else
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
	std::string joinMsg = ":" + client->getNickname() + "!" + client->getRealname() + "@" + getServerIp() + " JOIN :" + channelName + "\033[0m" + "\r\n";
	std::cout << "\nCmd Join send by " << getBackgroundColorCode(client->getSocket()) << joinMsg << "\033[0m";
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
	}
	else
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
	std::string partMsg =  getBackgroundColorCode(client->getSocket()) + client->getNickname() + "!" + client->getRealname() + "@" + getServerIp() + " PART " + channelName + "\033[0m" + "\r\n";
	std::cout << "\033[0m\nCmd PART send by " << getBackgroundColorCode(client->getSocket()) << ":" << partMsg << "\033[0m";
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
				send(channelClients[i]->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
		}
	}
	
	else
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
			send(targetClient->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
		
		else
		{
			/* Envoyer le message au client cible */
			send(targetClient->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
		}
	}
}

/**
 * Getter 
 */
fd_set& Server::getMasterSet()
{
	return _masterSet;
}

/**
 * Get the maximum file descriptor
 */
int Server::getFdMax() const
{
	return _fdMax;
}

/**
 * Get the server name
 */
const std::string& Server::getServerName() const
{
	return _serverName;
}

/**
 * Set the maximum file descriptor
 */
void Server::setFdMax(int fd)
{
	if (fd > _fdMax)
		_fdMax = fd;
}

std::string & Server::getServerIp()
{
	return _serverIp;
}