/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:07:13 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/19 01:06:29 by raveriss         ###   ########.fr       */
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
	 * `sa` for S.igA.ction
	 */
	sigemptyset(&sa.sa_mask);

	/**
	 * Définit le drapeau SA_RESTART pour relancer les appels système interrompus.
	 * Cela assure que certaines fonctions bloquantes, comme accept(), reprennent automatiquement.
	 * `sa` for S.igA.ction
	 */
	sa.sa_flags = SA_RESTART;

	/**
	 * Configure les signaux SIGINT et SIGTSTP pour utiliser la structure sa.
	 * Si l'une des configurations échoue, lance une exception.
	 * `sa` for S.igA.ction
	 */
	if (sigaction(CTRL_C, &sa, NULL) == FAILURE || sigaction(CTRL_Z, &sa, NULL) == FAILURE)
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
	/**
	 * `iss` for i.nput s.tring s.tream
	 *
     * Crée un flux de chaîne pour lire le message mot par mot.
     * Ce flux permet de séparer la commande (JOIN, PART, etc.)
     * et le nom du canal.
     */
	std::istringstream iss(message);

    /**
     * Variables pour stocker la commande et le nom du canal extraits
     * depuis le flux. La commande sera le premier mot, le nom du canal
     * sera le second mot.
     */
	std::string command, channelName;

    /**
     * Lit le premier mot du flux dans `command`, puis le second mot
     * dans `channelName`. Le message est supposé avoir le format :
     * "COMMAND #channel_name".
     */
	iss >> command >> channelName;

    /**
     * Vérifie si le canal existe dans la collection `_channels`.
     * Si `channelName` correspond à un canal, retourne un pointeur
     * vers ce canal ; sinon, retourne NULL pour indiquer l'absence.
     */
	if (_channels.find(channelName) != _channels.end())
		return _channels[channelName];

    /** 
     * Retourne NULL si le canal n'existe pas, indiquant que le canal
     * mentionné dans le message n'est pas répertorié sur le serveur.
     */
	return NULL;
}

/**
 * Gère un message reçu d'un client. Met à jour l'activité du client,
 * lit les données envoyées, traite les messages complets et les passe au bot.
 * @param client : un pointeur vers l'objet Client
 */
void Server::handleClientMessage(Client *client)
{
    /** 
     * Met à jour le temps de la dernière activité du client.
     * Cela permet de suivre l'activité et de détecter les clients inactifs.
     */
	client->updateLastActivity();

    /**
     * Buffer pour stocker les données reçues du client. La taille est
     * définie par IRC_BUFFER_SIZE pour éviter l'utilisation de valeurs en dur.
     */
	char buffer[IRC_BUFFER_SIZE];

    /**
     * Reçoit les données du socket client et les stocke dans buffer.
     * `bytesRead` contient le nombre d'octets lus, ou -1 en cas d'erreur.
     */
	int bytesRead = recv(client->getSocket(), buffer, sizeof(buffer) - 1, 0);
	
    /**
     * Vérifie si la connexion a été fermée ou s'il y a eu une erreur.
     * Si bytesRead est 0, le client a fermé la connexion ; si -1, il y a eu une erreur.
     */	
	if (bytesRead <= 0)
	{
		if (bytesRead == 0)
			std::cout << "Le client " << client->getSocket() << " a fermé la connexion.\033[0m" << std::endl;
		else
			std::cerr << "Erreur lors de la réception des données du client " << client->getSocket() << std::endl;

        /** 
         * Supprime le client de la liste active car la connexion est fermée ou en erreur.
         */		
		removeClient(client);
	}
	else
	{
        /**
         * Termine la chaîne reçue en ajoutant '\0' pour rendre le buffer utilisable comme chaîne C.
         * Cela garantit que les données lues dans buffer sont bien formées en tant que chaîne.
         */
		buffer[bytesRead] = '\0';

        /**
         * Référence vers le tampon de messages incomplets du client.
         * Les nouvelles données reçues sont ajoutées pour être traitées plus tard.
         */
		std::string& messageBuffer = client->getMessageBuffer();
		messageBuffer.append(buffer);

        /**
         * Boucle pour rechercher des messages complets terminés par '\n' dans messageBuffer.
         * `find('\n')` retourne la position de '\n' s'il est trouvé, ou `npos` sinon.
         */
		std::string::size_type pos;
		while ((pos = messageBuffer.find('\n')) != std::string::npos)
		{
            /**
             * Extrait un message complet (jusqu'à '\n') de messageBuffer et le copie dans `message`.
             * `erase` supprime ce message de messageBuffer, laissant les données restantes.
             */
			std::string message = messageBuffer.substr(0, pos);
			messageBuffer.erase(0, pos + 1);
			
            /** 
             * Supprime le caractère '\r' en fin de message, si présent.
             * Ceci gère les terminators CRLF des messages réseau (IRC utilise "\r\n").
             */
			if (!message.empty() && message[message.size() - 1] == '\r')
				message.erase(message.size() - 1);

            /**
             * Passe le message complet à la fonction de traitement de commandes.
             * La fonction `processCommand` gère les commandes IRC envoyées par le client.
             */
			processCommand(client, message);
			
            /**
             * Passe le message au bot pour modération et traitement automatique.
             * `getChannelFromMessage` identifie le canal associé au message, si applicable.
             * Si le message correspond à un canal, le bot le modère.
             */
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
    /**
     * Vérifie si le nombre d'arguments est suffisant pour traiter la commande.
     * Si les arguments sont insuffisants, envoie une erreur 461 (ERR_NEEDMOREPARAMS).
     */
	if (params.size() < TWO_ARGMNTS)
	{
		std::string error = IRCCodes::ERR_NEEDMOREPARAMS + " MODE :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le nom du canal cible de la commande.
     * `params[1]` contient le nom du canal.
     */
	std::string channelName = params[1];

    /**
     * Vérifie si le canal spécifié existe dans la liste des canaux.
     * Si le canal n'existe pas, envoie une erreur 403 (ERR_NOSUCHCHANNEL).
     */
	if (_channels.find(channelName) == _channels.end())
	{
		std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère un pointeur vers le canal existant.
     * Ce pointeur permet d'accéder aux propriétés et modes du canal.
     */
	Channel *channel = _channels[channelName];

    /**
     * Si aucun autre paramètre n'est fourni, renvoie les modes actuels du canal.
     * Cette réponse commence par "+", suivi des modes activés (ex: "+itk").
     */
	if (params.size() == TWO_ARGMNTS)
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

    /**
     * Vérifie si le client est un opérateur du canal avant de modifier les modes.
     * Si le client n'est pas un opérateur, envoie une erreur 482 (ERR_CHANOPRIVSNEEDED).
     */
	if (!channel->isOperator(client))
	{
		std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère la chaîne de caractères des modes de la commande.
     * Chaque caractère de cette chaîne représente un mode à ajouter ou supprimer.
     */
	std::string modeString = params[2];

	/* Index du paramètre à traiter pour certains modes (ex : clé ou limite) */
	size_t paramIndex = 3;

	/* Indique si les modes doivent être ajoutés (true) ou supprimés (false) */
	bool adding = true;

    /**
     * Boucle sur chaque caractère de mode dans `modeString`.
     * Selon le caractère, un mode est activé ou désactivé sur le canal.
     */
	for (size_t i = 0; i < modeString.length(); ++i)
	{
		char modeChar = modeString[i];
		
		/* Si '+', commence à ajouter les modes suivants */
		if (modeChar == '+')
			adding = true;
		
		/* Si '-', commence à retirer les modes suivants */
		else if (modeChar == '-')
			adding = false;
		
		/* Modes "invitation seulement" ou "TOPIC" */
		else if (modeChar == 'i' || modeChar == 't')
		{
			if (adding)
				channel->setMode(modeChar);
			else
				channel->unsetMode(modeChar);
		}
		
		/* Mode "clé de canal" (mot de passe) */
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
		
		/* Mode "limite d'utilisateurs" */
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
		
		/* Mode "opérateur de canal" */
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
			
			/* Recherche du client par son pseudonyme dans la liste des clients */
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

    /**
     * Envoie une notification aux membres du canal indiquant les changements de mode.
     */
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
 * Gère la commande INVITE permettant à un client d'inviter un autre client
 * dans un canal spécifique, en fonction des permissions et de l'état du canal.
 * @param client : le client qui envoie la commande INVITE
 * @param params : vecteur contenant les arguments de la commande INVITE
 */
void Server::handleInviteCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le nombre d'arguments est suffisant pour traiter l'invitation.
     * Si insuffisant, envoie une erreur 461 (ERR_NEEDMOREPARAMS).
     */
	if (params.size() < THREE_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS INVITE :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le pseudonyme du client cible et le nom du canal à partir des arguments.
     * `params[1]` contient le pseudo cible, et `params[2]` le nom du canal.
     */
	std::string targetNick = params[1];
	std::string channelName = params[2];

    /**
     * Vérifie si le canal spécifié existe dans la liste des canaux du serveur.
     * Si le canal n'existe pas, envoie une erreur 403 (ERR_NOSUCHCHANNEL).
     */
	if (_channels.find(channelName) == _channels.end())
	{
		std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère un pointeur vers le canal existant pour faciliter l'accès à ses propriétés.
     */
	Channel *channel = _channels[channelName];

    /**
     * Vérifie si le client qui invite est lui-même membre du canal.
     * Si le client n'est pas membre, envoie une erreur 442 (ERR_NOTONCHANNEL).
     */
	if (!channel->hasClient(client))
	{
		std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Si le canal est en mode "invitation seulement" (mode 'i'),
     * vérifie que le client qui invite est un opérateur du canal.
     * Si non, envoie une erreur 482 (ERR_CHANOPRIVSNEEDED).
     */
	if (channel->hasMode('i') && !channel->isOperator(client))
	{
		std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Recherche le client cible par son pseudonyme parmi tous les clients connectés.
     * Si le client cible n'est pas trouvé, envoie une erreur 401 (ERR_NOSUCHNICK).
     */
	Client *targetClient = NULL;
	for (size_t i = 0; i < _clients.size(); ++i)
	{
		if (_clients[i]->getNickname() == targetNick)
		{
			targetClient = _clients[i];
			break;
		}
	}

    /**
     * Si le client cible est introuvable, retourne une erreur.
     */
	if (!targetClient)
	{
		std::string error = IRCCodes::ERR_NOSUCHNICK + targetNick + " :No such nick/channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Vérifie si le client cible est déjà membre du canal.
     * Si oui, envoie une erreur 443 (ERR_USERONCHANNEL).
     */
	if (channel->hasClient(targetClient))
	{
		std::string error = "443 ERR_USERONCHANNEL " + targetNick + " " + channelName + " :is already on channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Ajoute le client cible à la liste des invités du canal.
     * Cela permet au client invité de rejoindre le canal par la suite.
     */
	channel->inviteClient(targetClient);

    /**
     * Envoie un message d'invitation au client cible pour l'informer de l'invitation.
     * Le client cible voit un message indiquant qu'il a été invité à rejoindre le canal.
     */
	std::string inviteMsg = ":" + client->getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n";
	send(targetClient->getSocket(), inviteMsg.c_str(), inviteMsg.length(), 0);

    /**
     * Confirme au client qui a envoyé l'invitation que celle-ci a été envoyée avec succès.
     * Le client qui invite voit une réponse 341 confirmant l'envoi de l'invitation.
     */
	std::string reply = "341 " + client->getNickname() + " " + targetNick + " :" + channelName + "\r\n";
	send(client->getSocket(), reply.c_str(), reply.length(), 0);
}

/**
 * Gère la commande TOPIC pour définir ou consulter le sujet d'un canal.
 * La commande permet de définir un nouveau sujet ou de consulter le sujet actuel
 * selon les permissions et le mode de configuration du canal.
 * @param client : le client qui envoie la commande TOPIC
 * @param params : vecteur contenant les arguments de la commande TOPIC
 */
void Server::handleTopicCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le nombre d'arguments est suffisant pour la commande TOPIC.
     * Si insuffisant, envoie une erreur 461 (ERR_NEEDMOREPARAMS).
     */
	if (params.size() < TWO_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS TOPIC :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le nom du canal cible de la commande.
     * `params[1]` contient le nom du canal.
     */
	std::string channelName = params[1];

    /**
     * Vérifie si le canal spécifié existe dans la liste des canaux.
     * Si le canal n'existe pas, envoie une erreur 403 (ERR_NOSUCHCHANNEL).
     */
	if (_channels.find(channelName) == _channels.end())
	{
		std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère un pointeur vers le canal existant pour faciliter l'accès à ses propriétés.
     */
	Channel *channel = _channels[channelName];

    /**
     * Vérifie si le client est un membre du canal.
     * Si non, envoie une erreur 442 (ERR_NOTONCHANNEL) indiquant que le client n'est pas dans le canal.
     */
	if (!channel->hasClient(client))
	{
		std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		
		return;
	}

    /**
     * Si aucun sujet n'est fourni dans les arguments, renvoie le sujet actuel du canal.
     * Si un sujet est défini, envoie une réponse 332 avec le sujet actuel ;
     * sinon, envoie une réponse 331 indiquant que le canal n'a pas de sujet.
     */
	if (params.size() == TWO_ARGMNTS)
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

    /**
     * Si le canal est en mode 't', seuls les opérateurs peuvent définir le sujet.
     * Vérifie les permissions de l'opérateur et, si elles manquent, envoie une erreur 482.
     */
	if (channel->hasMode('t') && !channel->isOperator(client))
	{
		std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Construit le nouveau sujet en combinant les paramètres, car le sujet
     * peut inclure plusieurs mots. Supprime le caractère ':' s'il est présent.
     */
	std::string topic = params[2];
	if (topic[0] == ':')
		topic = topic.substr(1);
	for (size_t i = 3; i < params.size(); ++i)
	{
		topic += " " + params[i];
	}

    /**
     * Définit le nouveau sujet du canal avec le texte assemblé.
     */
	channel->setTopic(topic);

    /**
     * Notifie tous les membres du canal que le sujet a été modifié.
     * Envoie un message indiquant le changement de sujet pour chaque membre.
     */
	std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
	const std::vector<Client*> &channelClients = channel->getClients();
	for (size_t i = 0; i < channelClients.size(); ++i)
	{
		send(channelClients[i]->getSocket(), topicMsg.c_str(), topicMsg.length(), 0);
	}
}

/**
 * Gère la commande KICK pour expulser un membre d'un canal.
 * Un opérateur de canal peut utiliser cette commande pour retirer un membre spécifique du canal.
 * @param client : le client qui envoie la commande KICK
 * @param params : vecteur contenant les arguments de la commande KICK
 */
void Server::handleKickCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le nombre d'arguments est suffisant pour la commande KICK.
     * Si insuffisant, envoie une erreur 461 (ERR_NEEDMOREPARAMS).
     */
	if (params.size() < THREE_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS KICK :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le nom du canal cible et le pseudonyme du client à expulser.
     * `params[1]` contient le nom du canal et `params[2]` le pseudonyme de la cible.
     */
	std::string channelName = params[1];
	std::string targetNick = params[2];

    /**
     * Construit le commentaire pour l'expulsion si fourni. 
     * Par défaut, le commentaire contient le pseudonyme de l'expéditeur.
     */
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

    /**
     * Vérifie si le canal spécifié existe dans la liste des canaux.
     * Si le canal n'existe pas, envoie une erreur 403 (ERR_NOSUCHCHANNEL).
     */
	if (_channels.find(channelName) == _channels.end())
	{
		std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère un pointeur vers le canal pour faciliter l'accès à ses propriétés.
     */
	Channel *channel = _channels[channelName];

    /**
     * Vérifie si le client qui envoie la commande est membre du canal.
     * Si le client n'est pas dans le canal, envoie une erreur 442 (ERR_NOTONCHANNEL).
     */
	if (!channel->hasClient(client))
	{
		std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Vérifie si le client est un opérateur du canal, car seuls les opérateurs
     * sont autorisés à expulser d'autres membres.
     * Si le client n'est pas opérateur, envoie une erreur 482 (ERR_CHANOPRIVSNEEDED).
     */
	if (!channel->isOperator(client))
	{
		std::string error = "482 ERR_CHANOPRIVSNEEDED " + channelName + " :You're not channel operator\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Recherche le client cible par son pseudonyme parmi les clients connectés.
     * Si le client cible n'est pas trouvé, envoie une erreur 401 (ERR_NOSUCHNICK).
     */
	Client *targetClient = NULL;
	for (size_t i = 0; i < _clients.size(); ++i)
	{
		if (_clients[i]->getNickname() == targetNick)
		{
			targetClient = _clients[i];
			break;
		}
	}

    /**
     * Si le client cible est introuvable, retourne une erreur.
     */
	if (!targetClient)
	{
		std::string error = "401 ERR_NOSUCHNICK " + targetNick + " :No such nick/channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Vérifie si le client cible est membre du canal.
     * Si non, envoie une erreur 441 (ERR_USERNOTINCHANNEL) indiquant que le client cible n'est pas dans le canal.
     */
	if (!channel->hasClient(targetClient))
	{
		std::string error = "441 ERR_USERNOTINCHANNEL " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Notifie tous les membres du canal que le client cible est expulsé,
     * en envoyant un message indiquant l'auteur de l'expulsion et la raison.
     */
	std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName + " " + targetNick + " :" + comment + "\r\n";
	const std::vector<Client*> &channelClients = channel->getClients();
	for (size_t i = 0; i < channelClients.size(); ++i)
	{
		send(channelClients[i]->getSocket(), kickMsg.c_str(), kickMsg.length(), 0);
	}

    /**
     * Retire le client cible du canal et met à jour son état pour refléter son départ.
     */
	channel->removeClient(targetClient);
	targetClient->leaveChannel(channel);

    /**
     * Si le canal devient vide après l'expulsion, il est automatiquement supprimé du serveur.
     */
	if (channel->getClients().empty())
	{
		_channels.erase(channelName);
		delete channel;
	}
}

/**
 * Gère la commande CAP (Capabilities), utilisée par les clients pour négocier 
 * des fonctionnalités supplémentaires (capacités) avec le serveur IRC.
 * Cette fonction vérifie les sous-commandes et répond en fonction des capacités supportées.
 * @param client : le client qui envoie la commande CAP
 * @param params : vecteur contenant les arguments de la commande CAP
 */
void Server::handleCapCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le nombre d'arguments est suffisant pour traiter la commande CAP.
     * Si insuffisant, envoie une erreur 461 (ERR_NEEDMOREPARAMS) indiquant le manque d'arguments.
     */
	if (params.size() < TWO_ARGMNTS)
	{
		/* Si le client est enregistré, utilise son pseudonyme ; sinon, utilise "*" */
		std::string nick = client->isRegistered() ? client->getNickname() : "*";
		std::string error = ":" + _serverName + " 461 " + nick + " CAP :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère la sous-commande CAP fournie dans les arguments.
     * `params[1]` contient la sous-commande de la commande CAP, comme "LS" ou "END".
     */
	std::string subCommand = params[1];

    /**
     * Traite la sous-commande "LS" qui liste les capacités supportées par le serveur.
     * Si des capacités sont définies, elles seront listées ici.
     */
	if (subCommand == "LS")
	{
		/* Par défaut, les capacités supportées sont laissées vides pour cette implémentation */
		std::string capabilities = "";

        /**
         * Gère les capacités multi-lignes si le client le demande (version 302),
         * permettant d'envoyer de longues listes de capacités en plusieurs messages.
         */
		if (params.size() >= 3 && params[2] == "302")
		{
			/* Gérer les capacités multi-lignes si nécessaire */
		}

		/* Utilise le pseudonyme enregistré du client ou "*" s'il n'est pas encore enregistré */
		std::string nick = client->isRegistered() ? client->getNickname() : "*";

		/* Forme et envoie la réponse avec la liste des capacités supportées (ici vide) */
		std::string response = ":" + _serverName + " CAP " + nick + " LS :" + capabilities + "\r\n";
		send(client->getSocket(), response.c_str(), response.length(), 0);
	}

    /**
     * Gère la sous-commande "END", signifiant que le client termine la négociation des capacités.
     * Aucune action spécifique n'est requise pour cette implémentation de base.
     */	
	else if (subCommand == "END")
	{
		/* Rien à faire dans cette implémentation, car aucune capacité n'est supportée */
	}

    /**
     * Gère les sous-commandes inconnues ou non supportées.
     * Envoie une erreur 410 (ERR_INVALIDCAPCMD) pour indiquer une sous-commande non valide.
     */	
	else
	{
		/* Utilise le pseudonyme enregistré ou "*" pour identifier le client dans l'erreur */
		std::string nick = client->isRegistered() ? client->getNickname() : "*";
		std::string error = ":" + _serverName + " 410 " + nick + " " + subCommand + " :Invalid CAP subcommand\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
	}
}

/**
 * Envoie un message à un client via le descripteur de fichier donné, avec une limite de longueur.
 * Tronque les messages trop longs et ajoute une terminaison CRLF (Carriage Return Line Feed) avant envoi.
 * @param message : le message à envoyer
 * @param sender_fd : le descripteur de fichier du client destinataire
 * @return bool : retourne true si le message a été envoyé
 */
bool Server::send_message(const std::string &message, int sender_fd)
{
    /**
     * Crée une copie temporaire du message pour ajuster sa longueur si nécessaire.
     * Ceci garantit que l'original reste intact.
     */
	std::string tmp = message;

    /**
     * Si la taille du message dépasse 510 caractères, tronque le message.
     * Ajoute "\r\n" pour indiquer la fin du message (protocole IRC).
     * La limite de 510 caractères est imposée pour éviter des messages trop longs qui pourraient causer des erreurs.
     */
	if (tmp.size() > 510)
		tmp = tmp.substr(0, 510) + "\r\n";

    /**
     * Envoie le message (ou sa version tronquée) au client via le descripteur de fichier `sender_fd`.
     * La valeur 0 indique l'envoi sans options supplémentaires (voir `send()` pour plus de détails).
     */
	send(sender_fd, tmp.c_str(), tmp.size(), 0);

    /**
     * Affiche une confirmation dans la console du serveur pour indiquer que le message a été envoyé,
     * avec un code couleur jaune (ANSI) pour la visibilité.
     */
	std::cout << "\n\033[43mMessage sent: " << tmp << "\033[0m";

	/* Retourne true pour indiquer que le message a été envoyé avec succès. */
	return true;
}

std::string Server::formatPingPongMessage(const std::string& client_id, const std::string& command, const std::string& param)
{
    /* Si vous souhaitez conserver les codes de couleur, vous pouvez les inclure ici */
    std::string colored_client_id = "\033[43m" + client_id + "\033[0m";
    return colored_client_id + " " + command + " :" + param + "\r\n";
}

/**
 * Gère la commande PING/PONG pour vérifier ou répondre à la disponibilité d'un client.
 */
bool Server::handlePingPongCommand(Client* client, const std::string& args)
{
    /* Création de l'ID client */
    std::string client_id = client->getNickname() + "!" + client->getUsername() + "@" + _serverIp;

    /* Détermination de la commande (PING ou PONG) */
    std::string command = client->isRegistered() ? "PONG" : "PING";

    /* Paramètre à envoyer (utilise args si non vide) */
    std::string param = args.empty() ? "" : args;

    /* Formattage du message */
    std::string response = formatPingPongMessage(client_id, command, param);

    /**
     * Envoie le message PING ou PONG construit au client via `send_message`.
     * La réponse varie selon que le client est en cours d'enregistrement ou déjà enregistré.
     */
    send_message(response, client->getSocket());

	/* Retourne true pour indiquer que le message a bien été envoyé */
    return true;
}

/**
 * Recherche et retourne un client en fonction de son pseudonyme (nickname).
 * @param nickname : le pseudonyme du client à rechercher
 * @return Client* : pointeur vers l'objet Client correspondant au pseudonyme donné,
 *                   ou NULL si aucun client n'est trouvé.
 */
Client* Server::getClientByNickname(const std::string &nickname)
{
    /**
     * Parcourt la liste des clients connectés (_clients) pour trouver une correspondance.
     * `_clients` est un vecteur contenant les pointeurs vers tous les objets Client actifs.
     */
	for (size_t i = 0; i < _clients.size(); ++i)
	{
        /**
         * Vérifie si le pseudonyme du client courant correspond au pseudonyme recherché.
         * Utilise la fonction `getNickname()` de l'objet Client pour accéder à son pseudonyme.
         */
		if (_clients[i]->getNickname() == nickname)
			return _clients[i];
	}
	
    /**
     * Si aucun client n'est trouvé avec le pseudonyme donné, retourne NULL.
     * Cela signifie que le pseudonyme recherché n'est pas actuellement utilisé par un client actif.
     */
	return NULL;
}

/**
 * Traite une commande envoyée par un client, en fonction du message reçu.
 * Cette fonction interprète et route les commandes IRC standard (JOIN, NICK, PRIVMSG, etc.) vers les gestionnaires correspondants.
 * @param client : le client qui envoie la commande
 * @param message : le message contenant la commande et ses arguments
 */
void Server::processCommand(Client *client, const std::string &message)
{
    /**
     * Découpe le message en mots, séparant la commande des paramètres.
     * `split` retourne un vecteur (`tokens`) contenant la commande et ses arguments.
     */
	std::vector<std::string> tokens = split(message, " ");

	/* Si le message est vide, arrête le traitement */
	if (tokens.empty())
		return;

    /**
     * La première partie du message est la commande, en majuscules pour simplifier la comparaison.
     * Cela permet de traiter les commandes sans tenir compte de la casse.
     */
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

    /**
     * Si le client n'est pas encore enregistré et tente une autre commande, envoie une erreur 451.
     */
	else if (!client->isRegistered())
	{
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

    /**
     * Si la commande n'est pas reconnue, envoie une erreur 421 indiquant une commande inconnue.
     */
	else
	{
		std::string error = "421 ERR_UNKNOWNCOMMAND " + command + " :Unknown command\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
	}

    /**
     * Vérifie si le client est maintenant enregistré après l'exécution de la commande.
     * `registerClient` peut finaliser l'enregistrement si toutes les informations sont complètes.
     */
	registerClient(client);
}

/**
 * Vérifie si un client remplit toutes les conditions pour être enregistré sur le serveur IRC.
 * Si les conditions sont remplies, marque le client comme enregistré et envoie le message du jour (MOTD).
 * @param client : le client à vérifier et enregistrer
 */
void Server::registerClient(Client *client)
{
    /**
     * Vérifie que le client n'est pas déjà enregistré et qu'il a fourni les informations nécessaires :
     * - Le mot de passe (`hasSentPass`)
     * - Un pseudonyme (`hasSentNick`)
     * - Des informations utilisateur (`hasSentUser`)
     * Si ces trois conditions sont remplies, le client est enregistré.
     */
	if (!client->isRegistered() && client->hasSentPass() && client->hasSentNick() && client->hasSentUser())
	{
        /**
         * Marque le client comme enregistré en définissant son état interne à "registered".
         * Cela permet au serveur de traiter ses commandes IRC au-delà de l'authentification.
         */
		client->setRegistered(true);

        /**
         * Envoie le Message Of The Day (MOTD) au client enregistré, contenant des informations
         * générales ou des annonces concernant le serveur.
         */
		sendMotd(client);
	}
}

/**
 * Gère la commande PASS envoyée par un client pour s'authentifier avec un mot de passe.
 * Valide le mot de passe fourni, envoie des erreurs appropriées en cas d'échec,
 * et marque le client comme ayant envoyé un mot de passe valide.
 * @param client : le client qui envoie la commande PASS
 * @param params : vecteur contenant la commande et ses paramètres
 */
void Server::handlePassCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le client a déjà envoyé un mot de passe valide.
     * Si c'est le cas, envoie une erreur 462 indiquant qu'il est interdit de se réenregistrer.
     */
	if (client->hasSentPass())
	{
		std::string error = "462 ERR_ALREADYREGISTRED :You may not reregister\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Vérifie si suffisamment de paramètres ont été fournis avec la commande PASS.
     * Si non, envoie une erreur 461 indiquant un manque de paramètres.
     */
	if (params.size() < TWO_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS PASS :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le mot de passe fourni dans les paramètres.
     * `params[1]` correspond au deuxième élément (après "PASS").
     */
	std::string password = params[1];

    /**
     * Vérifie si le mot de passe fourni correspond au mot de passe attendu par le serveur.
     * Si non, envoie une erreur 464 indiquant un mot de passe incorrect et déconnecte le client.
     */
	if (password != _password)
	{
		std::string error = "464 ERR_PASSWDMISMATCH :Password incorrect\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		removeClient(client);
		return;
	}

    /**
     * Si toutes les vérifications passent, marque le client comme ayant envoyé un mot de passe valide.
     * Cela permet au processus d'enregistrement de continuer.
     */
	client->setSentPass(true);
}

/**
 * Vérifie si un pseudonyme (nickname) est valide selon les règles IRC.
 * Règles :
 * - Le pseudonyme ne doit pas être vide.
 * - La longueur maximale du pseudonyme est de 9 caractères.
 * - Le premier caractère doit être une lettre ou un caractère spécial autorisé.
 * - Les caractères restants doivent être alphanumériques ou des caractères spéciaux autorisés.
 * @param nickname : le pseudonyme à valider
 * @return bool : true si le pseudonyme est valide, false sinon
 */
bool Server::isValidNickname(const std::string &nickname)
{
    /**
     * Vérifie si le pseudonyme est vide ou dépasse la limite de 9 caractères.
     * Les pseudonymes trop longs ou absents sont immédiatement considérés comme invalides.
     */
	if (nickname.empty() || nickname.length() > 9)
		return false;

    /**
     * Liste des caractères spéciaux autorisés dans les pseudonymes IRC.
     * Ces caractères sont conformes aux standards IRC pour les pseudonymes.
     */
	std::string specialChars = "-[]\\`^{}|";

    /**
     * Vérifie que le premier caractère est valide :
     * - Il doit être une lettre (`std::isalpha`) ou
     * - Un caractère spécial autorisé contenu dans `specialChars`.
     */
	char firstChar = nickname[0];
	if (!std::isalpha(firstChar) && specialChars.find(firstChar) == std::string::npos)
		return false;

    /**
     * Parcourt les caractères restants du pseudonyme pour vérifier leur validité.
     * Chaque caractère doit être :
     * - Une lettre ou un chiffre (`std::isalnum`),
     * - Ou un caractère spécial autorisé.
     */
	for (size_t i = 1; i < nickname.length(); ++i)
	{
		char c = nickname[i];
		if (!std::isalnum(c) && specialChars.find(c) == std::string::npos)
			return false;
	}

    /**
     * Si toutes les vérifications passent, le pseudonyme est valide.
     */
	return true;
}

/**
 * Gère la commande NICK envoyée par un client pour définir ou changer son pseudonyme.
 * La fonction valide le pseudonyme, vérifie qu'il n'est pas utilisé, et met à jour le pseudonyme du client.
 * @param client : le client qui envoie la commande NICK
 * @param params : vecteur contenant la commande et ses paramètres
 */
void Server::handleNickCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si suffisamment de paramètres ont été fournis avec la commande NICK.
     * Si aucun pseudonyme n'est donné, envoie une erreur 431 indiquant "No nickname given".
     */
	if (params.size() < TWO_ARGMNTS)
	{
		std::string error = "431 ERR_NONICKNAMEGIVEN :No nickname given\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le nouveau pseudonyme proposé par le client.
     * `params[1]` contient le pseudonyme fourni après la commande NICK.
     */
	std::string newNickname = params[1];

    /**
     * Valide le pseudonyme en vérifiant qu'il respecte les règles IRC (longueur, caractères valides, etc.).
     * Si le pseudonyme est invalide, envoie une erreur 432 indiquant "Erroneous nickname".
     */
	if (!isValidNickname(newNickname))
	{
		std::string error = "432 ERR_ERRONEUSNICKNAME " + newNickname + " :Erroneous nickname\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Vérifie si le pseudonyme est déjà utilisé par un autre client enregistré sur le serveur.
     * Si le pseudonyme est en cours d'utilisation, envoie une erreur 433 indiquant "Nickname is already in use".
     */
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if ((*it)->getNickname() == newNickname && *it != client)
		{
			std::string error = "433 ERR_NICKNAMEINUSE " + newNickname + " :Nickname is already in use\r\n";
			send(client->getSocket(), error.c_str(), error.length(), 0);
			return;
		}
	}

    /**
     * Si le client est déjà enregistré et change de pseudonyme, informe les autres clients enregistrés du changement.
     * Le message NICK est diffusé aux autres clients pour indiquer la modification.
     */
	if (client->isRegistered())
	{
		std::string nickChangeMsg = ":" + client->getNickname() + " NICK :" + newNickname + "." + "\033[0m\r\n";
		for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (*it != client && (*it)->isRegistered())
				send((*it)->getSocket(), nickChangeMsg.c_str(), nickChangeMsg.length(), 0);
		}
	}

    /**
     * Met à jour le pseudonyme du client avec le nouveau pseudonyme validé.
     * Le pseudonyme est stocké dans l'objet Client et mis à jour pour les interactions futures.
     */
	client->setNickname(newNickname);
	std::cout << getBackgroundColorCode(client->getSocket()) 
			<< "\nClient " << client->getSocket() 
			<< " changed nickname to " << newNickname 
			<< "\033[0m\033[K" << std::endl;
			
    /**
     * Si le client n'est pas encore enregistré, marque l'état SentNick comme vrai.
     * Cela indique que le client a envoyé un pseudonyme valide et permet de continuer le processus d'enregistrement.
     */
	if (!client->isRegistered())
		client->setSentNick(true);
}

/**
 * Vérifie si un nom d'utilisateur (username) est valide selon des règles définies.
 * Règles :
 * - Le nom d'utilisateur ne doit pas être vide.
 * - La longueur maximale est de 10 caractères.
 * - Chaque caractère doit être alphanumérique ou un caractère spécial autorisé ('_' ou '-').
 * @param username : le nom d'utilisateur à valider
 * @return bool : true si le nom d'utilisateur est valide, false sinon
 */
bool Server::isValidUsername(const std::string &username)
{
    /**
     * Vérifie si le nom d'utilisateur est vide ou dépasse la limite de 10 caractères.
     * Si c'est le cas, retourne false pour indiquer un nom d'utilisateur invalide.
     */
	if (username.empty() || username.length() > 10) 
		return false;

    /**
     * Parcourt chaque caractère du nom d'utilisateur pour vérifier sa validité.
     * Chaque caractère doit être :
     * - Une lettre ou un chiffre (`std::isalnum`),
     * - Ou un caractère spécial autorisé ('_' ou '-').
     * Si un caractère invalide est détecté, retourne false.
     */
	for (size_t i = 0; i < username.length(); ++i)
	{
		char c = username[i];
		if (!std::isalnum(c) && c != '_' && c != '-')
			return false;
	}

    /**
     * Si toutes les vérifications passent, retourne true pour indiquer un nom d'utilisateur valide.
     */
	return true;
}

/**
 * Gère la commande USER envoyée par un client pour fournir ses informations utilisateur.
 * Valide et enregistre les informations fournies (username, hostname, servername, realname),
 * tout en vérifiant que le client n'est pas déjà enregistré.
 * @param client : le client qui envoie la commande USER
 * @param params : vecteur contenant la commande et ses paramètres
 */
void Server::handleUserCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le client a déjà envoyé une commande USER.
     * Si c'est le cas, envoie une erreur 462 indiquant qu'il est interdit de se réenregistrer.
     */
	if (client->hasSentUser())
	{
		std::string error = "462 ERR_ALREADYREGISTRED :You may not reregister\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Vérifie si suffisamment de paramètres ont été fournis avec la commande USER.
     * La commande USER nécessite au moins 4 paramètres après le mot-clé "USER".
     * Si ce n'est pas le cas, envoie une erreur 461 indiquant un manque de paramètres.
     */
	if (params.size() < FIVE_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS USER :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère les paramètres requis de la commande USER :
     * - username : Identifiant unique de l'utilisateur.
     * - hostname : Nom d'hôte de la machine utilisée par l'utilisateur.
     * - servername : Nom du serveur auquel l'utilisateur est connecté.
     * - realname : Nom complet ou description de l'utilisateur.
     */
	std::string username = params[1];
	std::string hostname = params[2];
	std::string servername = params[3];
	std::string realname = params[4];

    /**
     * Si le realname commence par ':', supprime ce préfixe et concatène les
     * paramètres restants pour former le nom complet. Cela permet de gérer
     * les realnames contenant des espaces.
     */
	if (realname[0] == ':')
	{
		realname = realname.substr(1);
		for (size_t i = 5; i < params.size(); ++i)
		{
			realname += " " + params[i];
		}
	}

    /**
     * Valide le username pour s'assurer qu'il respecte les règles définies (longueur, caractères valides, etc.).
     * Si le username est invalide, envoie une erreur 432 indiquant un "Erroneous username".
     */
	if (!isValidUsername(username))
	{
		std::string error = "432 ERR_ERRONEUSNICKNAME " + username + " :Erroneous username\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Si le hostname n'est pas fourni ou est vide, utilise une valeur par défaut.
     * Cela garantit que chaque client a un hostname, même s'il n'en a pas spécifié.
     */
	if (hostname.empty())
	{
		/* Valeur par défaut si le nom d'hôte n'est pas fourni */
		hostname = "unknown";
	}

    /**
     * Met à jour les informations utilisateur du client dans son objet :
     * - username : Identifiant unique de l'utilisateur.
     * - hostname : Nom d'hôte (valeur par défaut utilisée si non fourni).
     * - servername : Nom du serveur auquel l'utilisateur est connecté.
     * - realname : Nom complet ou description de l'utilisateur.
     */
	client->setUsername(username);
	client->setHostname(hostname);
	client->setServername(servername);
	client->setRealname(realname);

    /**
     * Marque le client comme ayant envoyé une commande USER valide.
     * Cela permet au processus d'enregistrement de continuer.
     */
	client->setSentUser(true);
}

/**
 * Envoie les messages d'accueil et le MOTD (Message of the Day) à un client nouvellement enregistré.
 * Ces messages sont standardisés par le protocole IRC et fournissent des informations sur le serveur.
 * @param client : le client à qui envoyer les messages
 */
void Server::sendMotd(Client *client)
{
    /**
     * Récupère les informations du client :
     * - nick : Pseudonyme du client.
     * - username : Identifiant fourni par le client lors de la commande USER.
     */
	std::string nick = client->getNickname();
	std::string username = client->getUsername();

    /**
     * Envoie le message d'accueil 001 (RPL_WELCOME) au client.
     * Ce message souhaite la bienvenue et affiche des informations de connexion.
     */
	std::string welcome = ":" + _serverName + " 001 " + nick + " :Welcome to the Internet Relay Network " + client->getNickname() + "!" + client->getRealname() + "@" + getServerIp() + "\r\n";
	send(client->getSocket(), welcome.c_str(), welcome.length(), 0);

    /**
     * Envoie le message 002 (RPL_YOURHOST), qui informe le client de l'hôte du serveur
     * et de la version du logiciel qu'il exécute.
     */
	std::string yourHost = ":" + _serverName + " 002 " + nick + " :Your host is " + _serverName + ", running version 1.0\r\n";
	send(client->getSocket(), yourHost.c_str(), yourHost.length(), 0);

    /**
     * Envoie le message 003 (RPL_CREATED), indiquant la date de création du serveur.
     * Ici, une date fictive est utilisée, mais cela pourrait être dynamique.
     */
	std::string created = ":" + _serverName + " 003 " + nick + " :This server was created at some point in the past\r\n";
	send(client->getSocket(), created.c_str(), created.length(), 0);

    /**
     * Envoie le message 004 (RPL_MYINFO), qui fournit des informations
     * supplémentaires sur le serveur, telles que les options et modes activés.
     */
	std::string myInfo = ":" + _serverName + " 004 " + nick + " " + _serverName + " 1.0 o o\r\n";
	send(client->getSocket(), myInfo.c_str(), myInfo.length(), 0);

    /**
     * Envoie le message 375 (RPL_MOTDSTART), indiquant le début du Message of the Day.
     */
	std::string motdStart = ":" + _serverName + " 375 " + nick + " :- " + _serverName + " Message of the Day - \r\n";
	send(client->getSocket(), motdStart.c_str(), motdStart.length(), 0);

    /**
     * Envoie le message 372 (RPL_MOTD), contenant le contenu principal du Message of the Day.
     * Ce message peut être étendu pour inclure plusieurs lignes.
     */
	std::string motd = ":" + _serverName + " 372 " + nick + " :- Welcome to our IRC server!\r\n";
	send(client->getSocket(), motd.c_str(), motd.length(), 0);

    /**
     * Envoie le message 376 (RPL_ENDOFMOTD), indiquant la fin du Message of the Day.
     */
	std::string motdEnd = ":" + _serverName + " 376 " + nick + " :End of /MOTD command.\r\n";
	send(client->getSocket(), motdEnd.c_str(), motdEnd.length(), 0);
}

/**
 * Supprime un client du serveur en fermant son socket, en le retirant des structures internes,
 * et en le marquant pour une suppression différée.
 * @param client : le client à supprimer
 */
void Server::removeClient(Client *client)
{
    /**
     * Ferme le socket associé au client pour libérer les ressources réseau.
     * Cela met fin à la connexion entre le serveur et ce client.
     */
	close(client->getSocket());

    /**
     * Supprime le descripteur du client du _masterSet utilisé pour surveiller les sockets avec `select`.
     * FD_CLR (File Descriptor Clear) retire le socket du set.
     */
	FD_CLR(client->getSocket(), &_masterSet);

    /**
     * Supprime le client de la liste des clients (_clients).
     * - std::remove réorganise la liste en déplaçant le client à la fin.
     * - erase supprime effectivement le client de la liste.
     */
	_clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());

    /**
     * Ajoute le client à la liste _clientsToRemove pour une suppression différée.
     * Cela permet de différer la destruction du client jusqu'à ce que toutes les opérations
     * en cours soient terminées, évitant ainsi des comportements indéterminés.
     */
	_clientsToRemove.push_back(client);
}

/**
 * Divise une chaîne de caractères (`str`) en sous-chaînes (tokens) en utilisant un délimiteur (`delim`).
 * Renvoie un vecteur contenant les sous-chaînes résultantes.
 * @param str : La chaîne de caractères à diviser.
 * @param delim : Le délimiteur utilisé pour séparer la chaîne.
 * @return std::vector<std::string> : Les sous-chaînes divisées.
 */
std::vector<std::string> Server::split(const std::string &str, const std::string &delim)
{
    /**
     * Contiendra les sous-chaînes résultantes.
     */
	std::vector<std::string> tokens;

    /**
     * `prev` : Position de début d'une sous-chaîne à extraire.
     * `pos` : Position de l'occurrence du délimiteur.
     */
	size_t prev = 0, pos = 0;
	do
	{
        /**
         * Trouve la position du prochain délimiteur.
         * Si aucun délimiteur n'est trouvé, `find` retourne `std::string::npos`.
         */
		pos = str.find(delim, prev);

        /**
         * Si aucun délimiteur n'est trouvé, positionne `pos` à la fin de la chaîne.
         */
		if (pos == std::string::npos)
			pos = str.length();

        /**
         * Extrait une sous-chaîne allant de `prev` à `pos`.
         */
		std::string token = str.substr(prev, pos - prev);

        /**
         * Si la sous-chaîne n'est pas vide, l'ajoute au vecteur de résultats.
         */
		if (!token.empty())
			tokens.push_back(token);

        /**
         * Déplace `prev` juste après le délimiteur pour préparer la prochaine itération.
         */
		prev = pos + delim.length();
	}
	/* Continue tant qu'il reste des caractères à traiter. */
	while (pos < str.length() && prev < str.length());

    /**
     * Retourne le vecteur contenant les sous-chaînes divisées.
     */
	return tokens;
}

/**
 * Envoie la liste des utilisateurs d'un canal (`NAMES` command reply) au client.
 * Comprend les préfixes pour les opérateurs ('@') et les utilisateurs avec voix ('+').
 * Termine par un message signalant la fin de la liste.
 * @param client : Le client demandant la liste des noms.
 * @param channel : Le canal pour lequel les noms sont listés.
 */
void Server::sendNamesReply(Client *client, Channel *channel)
{
    /**
     * Initialise une chaîne pour contenir la liste des pseudonymes.
     * Cette chaîne inclura des préfixes pour les rôles spécifiques.
     */
	std::string nickList;

    /**
     * Récupère la liste des clients connectés à ce canal.
     */
	const std::vector<Client*> &clients = channel->getClients();

    /**
     * Parcourt chaque client du canal pour construire la liste des pseudonymes.
     */
	for (size_t i = 0; i < clients.size(); ++i)
	{
        /**
         * Si le client est opérateur du canal, ajoute '@' devant son pseudonyme.
         */
		if (channel->isOperator(clients[i]))
			nickList += "@";

        /**
         * Ajoute le pseudonyme du client suivi d'un espace.
         */
		nickList += clients[i]->getNickname() + " ";
	}

    /**
     * RPL_NAMREPLY (353) :
     * Envoie une réponse avec la liste des noms des utilisateurs connectés au canal.
     */
	std::string namesReply = ":" + _serverName + " 353 " + client->getNickname() + " = " +
		channel->getName() + " :" + nickList + "\r\n";
	send(client->getSocket(), namesReply.c_str(), namesReply.length(), 0);

    /**
     * RPL_ENDOFNAMES (366) :
     * Informe que la liste des noms est complète.
     */
	std::string endOfNames = ":" + _serverName + " 366 " + client->getNickname() + " " +
		channel->getName() + " :End of /NAMES list.\r\n";
	send(client->getSocket(), endOfNames.c_str(), endOfNames.length(), 0);
}

/**
 * Gère la commande JOIN envoyée par un client pour rejoindre un canal.
 * Si le canal n'existe pas, il est créé. Le client doit respecter les règles
 * de mode du canal (`+i`, `+k`, `+l`) pour y accéder.
 * @param client : Le client envoyant la commande JOIN.
 * @param params : Les paramètres de la commande JOIN, comprenant le nom du canal
 *                 et, éventuellement, une clé.
 */
void Server::handleJoinCommand(Client *client, const std::vector<std::string> &params)
{
    /** 
     * Vérifie si les paramètres de la commande sont suffisants (au moins un nom de canal).
     * Renvoie une erreur sinon.
     */
	if (params.size() < TWO_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS JOIN :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Récupère le nom du canal et, si présent, la clé pour y accéder.
     */
	std::string channelName = params[1];
	std::string key = "";
	if (params.size() >= 3)
		key = params[2];

    /**
     * Vérifie que le nom du canal est valide (commence par '#' ou '&').
     * Renvoie une erreur si le nom est incorrect.
     */
	if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
	{
		std::string error = "476 ERR_BADCHANMASK " + channelName + " :Bad Channel Mask\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Initialise un pointeur vers le canal. 
     * Si le canal n'existe pas, il sera créé.
     */
	Channel *channel = NULL;

    /** 
     * Crée un nouveau canal si celui-ci n'existe pas.
     * Le client qui crée le canal devient automatiquement opérateur.
     */
	if (_channels.find(channelName) == _channels.end())
	{
		channel = new Channel(channelName);
		_channels[channelName] = channel;
		/* Le premier client à rejoindre le canal devient opérateur */
		channel->addOperator(client);
	}
	else
	{
        /**
         * Le canal existe déjà. Vérifie les règles d'accès au canal :
         * - Mode `+i` : Invitation requise.
         * - Mode `+k` : Clé (mot de passe) requise.
         * - Mode `+l` : Limite d'utilisateurs atteinte.
         */
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

    /**
     * Ajoute le client au canal, supprime les invitations (si existantes)
     * et informe les autres clients du canal du nouvel arrivant.
     */
	channel->addClient(client);
	client->joinChannel(channel);
	channel->removeInvitation(client);

	/* Notifier les autres clients du canal */
	std::string joinMsg = ":" + client->getNickname() + "!" + client->getRealname() + "@" + getServerIp() + " JOIN :" + channelName + "\r\n";
	std::cout << "\nCmd Join send by " << getBackgroundColorCode(client->getSocket()) << joinMsg << "\033[0m\033[K";
	const std::vector<Client*> &channelClients = channel->getClients();
	for (size_t i = 0; i < channelClients.size(); ++i)
	{
		send(channelClients[i]->getSocket(), joinMsg.c_str(), joinMsg.length(), 0);
	}

    /**
     * Envoie le sujet du canal (RPL_TOPIC ou RPL_NOTOPIC) au client.
     */
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

    /**
     * Envoie la liste des noms des membres du canal (commande NAMES).
     */
	sendNamesReply(client, channel);
}

/**
 * Gère la commande PART envoyée par un client pour quitter un canal.
 * Notifie les autres membres du canal du départ du client. Supprime le canal
 * si aucun client n’y reste après le départ.
 * @param client : Le client envoyant la commande PART.
 * @param params : Les paramètres de la commande PART, comprenant le nom du canal.
 */
void Server::handlePartCommand(Client *client, const std::vector<std::string> &params)
{
    /** 
     * Vérifie si le client a fourni suffisamment de paramètres (au moins un nom de canal).
     * Renvoie une erreur si les paramètres sont insuffisants.
     */
	if (params.size() < TWO_ARGMNTS)
	{
		std::string error = "461 ERR_NEEDMOREPARAMS PART :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /** 
     * Récupère le nom du canal depuis les paramètres.
     */
	std::string channelName = params[1];

    /**
     * Vérifie que le canal existe dans la liste des canaux du serveur.
     * Renvoie une erreur si le canal n'existe pas.
     */
	if (_channels.find(channelName) == _channels.end())
	{
		std::string error = "403 ERR_NOSUCHCHANNEL " + channelName + " :No such channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /** 
     * Récupère le canal correspondant.
     */
	Channel *channel = _channels[channelName];

    /**
     * Vérifie que le client est bien membre du canal.
     * Renvoie une erreur s’il n’en est pas membre.
     */
	if (!channel->hasClient(client))
	{
		std::string error = "442 ERR_NOTONCHANNEL " + channelName + " :You're not on that channel\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /**
     * Prépare un message de notification indiquant que le client a quitté le canal.
     */
	std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + getServerIp() + " PART " + channelName + "\r\n";
	std::cout << "\033[0m\nCmd PART send by " << getBackgroundColorCode(client->getSocket()) << ":" << partMsg << "\033[0m\033[K";

	
    /**
     * Envoie le message à tous les membres du canal.
     */	
	const std::vector<Client*> &channelClients = channel->getClients();
	for (size_t i = 0; i < channelClients.size(); ++i)
	{
		send(channelClients[i]->getSocket(), partMsg.c_str(), partMsg.length(), 0);
	}

    /**
     * Retire le client du canal et met à jour ses informations.
     */
	channel->removeClient(client);
	client->leaveChannel(channel);

    /**
     * Si le canal est vide après le départ du client, le supprime de la liste des canaux.
     */
	if (channel->getClients().empty())
	{
		_channels.erase(channelName);
		delete channel;
	}
}

/**
 * Gère la commande PRIVMSG envoyée par un client pour transmettre un message.
 * La commande peut cibler un canal ou un utilisateur spécifique.
 * @param client : Le client qui envoie le message.
 * @param params : Les paramètres de la commande PRIVMSG.
 */
void Server::handlePrivmsgCommand(Client *client, const std::vector<std::string> &params)
{
    /**
     * Vérifie si le client a fourni suffisamment de paramètres (au moins une cible et un message).
     * Renvoie une erreur si les paramètres sont insuffisants.
     */
	if (params.size() < THREE_ARGMNTS)
	{
		std::string error = ":" + _serverName + " 461 " + client->getNickname() + " PRIVMSG :Not enough parameters\r\n";
		send(client->getSocket(), error.c_str(), error.length(), 0);
		return;
	}

    /** 
     * Récupère la cible (canal ou utilisateur) et le message depuis les paramètres.
     */
	std::string target = params[1];
	std::string message = params[2];

    /**
     * Si le message commence par ':', combine les paramètres restants pour former le message complet.
     */
	if (message[0] == ':')
	{
		message = message.substr(1);
		for (size_t i = 3; i < params.size(); ++i)
		{
			message += " " + params[i];
		}
	}

    /**
     * Vérifie si le message est un message CTCP (Client-to-Client Protocol).
     * Les messages CTCP commencent et se terminent par le caractère '\x01'.
     */
	bool isCTCP = false;
	if (!message.empty() && message[0] == '\x01' && message[message.size() - 1] == '\x01')
	{
		isCTCP = true;
		std::string ctcpMessage = message.substr(1, message.size() - 2);
		std::vector<std::string> ctcpParams = split(ctcpMessage, " ");
		std::string ctcpCommand = ctcpParams[0];

        /**
         * Gère la commande CTCP PING, en répondant au client avec un message PING.
         */
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

    /**
     * Prépare le message complet avec le format :[expéditeur] PRIVMSG [cible] :[message].
     */
	std::string fullMsg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";

    /**
     * Si la cible est un canal (nom commençant par '#' ou '&'), envoie le message à tous les membres du canal.
     */
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

        /**
         * Vérifie si le client est membre du canal.
         * Renvoie une erreur si le client n'est pas autorisé à envoyer un message au canal.
         */
		if (!channel->hasClient(client))
		{
			std::string error = ":" + _serverName + " 404 " + client->getNickname() + " " + target + " :Cannot send to channel\r\n";
			send(client->getSocket(), error.c_str(), error.length(), 0);
			return;
		}

        /**
         * Envoie le message à tous les membres du canal, sauf à l'expéditeur.
         */
		const std::vector<Client*> &channelClients = channel->getClients();
		for (size_t i = 0; i < channelClients.size(); ++i)
		{
			if (channelClients[i] != client)
				send(channelClients[i]->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
		}
	}
	
	else
	{
        /**
         * Si la cible est un utilisateur, cherche le client correspondant au pseudonyme cible.
         */
		Client *targetClient = NULL;
		for (size_t i = 0; i < _clients.size(); ++i)
		{
			if (_clients[i]->getNickname() == target)
			{
				targetClient = _clients[i];
				break;
			}
		}

        /**
         * Vérifie si le client cible existe. Renvoie une erreur si le pseudonyme est invalide.
         */
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
			/**
			 * Envoie le message directement au client cible.
			 * Si le message est un CTCP, le message est envoyé uniquement au destinataire.
			 */
			send(targetClient->getSocket(), fullMsg.c_str(), fullMsg.length(), 0);
		}
	}
}

/**
 * Accède au master set de descripteurs de fichiers.
 * Le master set contient tous les descripteurs actuellement surveillés par le serveur,
 * incluant le socket d'écoute et les sockets des clients connectés.
 * @return Référence à _masterSet, la structure contenant les descripteurs surveillés.
 */
fd_set& Server::getMasterSet()
{
    /**
     * Renvoie une référence à _masterSet pour permettre son utilisation ou modification
     * à l'extérieur de la classe Server.
     */
	return _masterSet;
}

/**
 * Accède à la valeur maximale des descripteurs de fichiers surveillés par le serveur.
 * Cette valeur est nécessaire pour la fonction `select()`, qui parcourt tous les
 * descripteurs de fichiers actifs jusqu'à ce nombre.
 * @return La valeur maximale des descripteurs de fichiers (_fdMax).
 */
int Server::getFdMax() const
{
    /**
     * Retourne _fdMax, le plus grand descripteur de fichier utilisé dans le serveur.
     * _fdMax est mis à jour chaque fois qu'un nouveau descripteur est ajouté.
     */
	return _fdMax;
}

/**
 * Récupère le nom du serveur IRC.
 * @return Une référence constante à la chaîne contenant le nom du serveur (_serverName).
 */
const std::string& Server::getServerName() const
{
    /**
     * Retourne la variable membre _serverName.
     * _serverName contient l'identifiant unique ou le nom du serveur utilisé
     * pour l'affichage et les messages protocolaires.
     */
	return _serverName;
}

/**
 * Met à jour le descripteur de fichier maximal suivi par le serveur.
 * @param fd Descripteur de fichier à comparer avec _fdMax.
 */
void Server::setFdMax(int fd)
{
    /**
     * Si le nouveau descripteur (fd) est supérieur au _fdMax actuel,
     * met à jour _fdMax avec la valeur de fd.
     * _fdMax représente le plus grand descripteur de fichier que le serveur doit surveiller.
     */
	if (fd > _fdMax)
		_fdMax = fd;
}

/**
 * Retourne l'adresse IP du serveur.
 * @return Une référence à la chaîne contenant l'adresse IP (_serverIp).
 */
std::string & Server::getServerIp()
{
    /**
     * Renvoie une référence à la variable membre _serverIp,
     * qui contient l'adresse IP actuelle du serveur.
     */
	return _serverIp;
}