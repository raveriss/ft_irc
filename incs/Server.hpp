/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 21:11:37 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/14 03:16:19 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

/* Inclusions pour les clients, les canaux et le bot */
#include "Bot.hpp"
#include "Client.hpp"

/* For std::vector */
#include <vector>

/* For std::string */
#include <string>

/* For memset() */
#include <cstring>

/* For getnameinfo, NI_MAXHOST */
#include <netdb.h>

/* For inet_ntoa() */
#include <arpa/inet.h>

/* For perror() */
#include <cstdio>

/* For std::istringstream */
#include <sstream>

/* For SIGINT */
#include <csignal>

/* For exit() */
#include <cstdlib>

/* Inclusion pour les appels système de POSIX (par ex., close) */
#include <unistd.h>

/* Pour manipuler les options de fichier, comme O_NONBLOCK */
#include <fcntl.h>

/* Inclusion pour les opérations d’entrée/sortie standard */
#include <iostream>

/* Pour utiliser errno */
#include <cerrno>

/* Pour utiliser strerror */
#include <cstring>

/* For struct ifaddrs */
#include <ifaddrs.h>

/* For inet_ntoa() */
#include <arpa/inet.h>

/* For UINT16_MAX */
#include <stdint.h>

/* For MAX_UINT16_BITS */ 
#define MAX_UINT16_BITS UINT16_MAX

/* Définit le protocole d'adresse comme IPv4 */
#define IPV4 AF_INET

/* Définit le protocole de transport comme TCP */
#define TCP SOCK_STREAM

/* Définit le protocole par défaut */
#define DEFLT_PROT 0

/* Niveau d'option du socket */
#define GLOB_SOCK_OPT SOL_SOCKET

/* Option pour réutiliser l'adresse */
#define REUSE_ADDR SO_REUSEADDR

/* Activer l'option */
#define OPT_ON 1

/* Ecoute maximale */
#define ALL_ADDR INADDR_ANY

/* Taille maximale de la file d'attente */
#define MAX_CONNEXIONS SOMAXCONN

/* Code de retour pour une opération échouée */
#define FAILURE -1

/* Nombre de paramètres requis pour la commande */
#define TWO_ARGMNTS 2
#define THREE_ARGMNTS 3
#define FIVE_ARGMNTS 5

/* Signal d'interruption déclenché par Ctrl + C */
#define CTRL_C SIGINT

/* Signal de suspension temporaire déclenché par Ctrl + Z */
#define CTRL_Z SIGTSTP

/* Index de l'argument pour le port dans argv */
#define PORT_ARG_INDEX 1



#define PING(client_id, param) (client_id + "\033[43m PING :" + param + "\033[0m\r\n")
#define PONG(client_id, param) (client_id + "\033[43m PONG :" + param + "\033[0m\r\n")


/* Déclaration anticipée de Client */
class Client;

/* Déclaration anticipée de Channel */
class Channel;

/**
 * class Server
 */
class Server
{
    public:
    
        /*  Ajoutez ceci dans la section publique ou privée */
        static Server* instance;

        /* Constructeur */
        Server(unsigned short port, const std::string &password);

        /* Destructeur */
        ~Server();


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                 GESTION DU SERVEUR                        */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Méthode pour lancer le serveur */
        void run();

        /* Retourne la valeur maximale de fd */
        int getFdMax() const;

        /* Retourne l'ensemble maître des fd */
        fd_set& getMasterSet();
    
        /* Retourne le nom du serveur */
        const std::string& getServerName() const;



        /* Définit la valeur maximale de fd */
        void setFdMax(int fd);

        std::string & getServerIp();


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                             INITIALISATION ET UTILITAIRES                 */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Initialise le serveur */
        void init();

         /* Méthodes pour gérer les commandes */
        void processCommand(Client *client, const std::string &message);

        /* Méthodes utilitaires */
        void removeClient(Client *client);

        /* Authentification et enregistrement */
        void registerClient(Client *client);

        /* Envoie le MOTD au client */
        void sendMotd(Client *client);

        /* Divise une chaîne en fonction d'un délimiteur */
        std::vector<std::string> split(const std::string &str, const std::string &delim);

        /* Retourne un client par son pseudonyme */
        Client* getClientByNickname(const std::string &nickname);

        /* Retourne un canal à partir d'un message */
        Channel* getChannelFromMessage(const std::string &message);

        /* Vérifie la validité d'un pseudonyme */
        bool isValidNickname(const std::string &nickname);

        /* Vérifie la validité d'un nom d'utilisateur */
        bool isValidUsername(const std::string &username);


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                              GESTION DES COMMANDES                        */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        
        /* Gère une nouvelle connexion */
        void handleNewConnection();

        /* Gère les messages des clients */
        void handleClientMessage(Client *client);

        /* Commandes IRC standard */
        void handlePassCommand(Client *client, const std::vector<std::string> &params);
        void handleNickCommand(Client *client, const std::vector<std::string> &params);
        void handleUserCommand(Client *client, const std::vector<std::string> &params);
        void handleJoinCommand(Client *client, const std::vector<std::string> &params);
        void handlePartCommand(Client *client, const std::vector<std::string> &params);
        void handlePrivmsgCommand(Client *client, const std::vector<std::string> &params);
        void handleModeCommand(Client *client, const std::vector<std::string> &params);
        void handleInviteCommand(Client *client, const std::vector<std::string> &params);
        void handleTopicCommand(Client *client, const std::vector<std::string> &params);
        void handleKickCommand(Client *client, const std::vector<std::string> &params);
        void handleCapCommand(Client *client, const std::vector<std::string> &params);
        bool handlePingPongCommand(Client *client, const std::string &args);






        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                         GESTION DES RÉPONSES ET MAINTENANCE              */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Envoie la réponse NAMES à un client pour un canal donné */
        void sendNamesReply(Client *client, Channel *channel);



        /* Gestion des signaux */
        static void handleSignal(int signal);

        /* Ferme tous les clients et le socket d'écoute */
        void shutdown();


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                GESTION DU PING                           */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Intervalle entre chaque envoi de PING en secondes */
        static const int PING_INTERVAL = 60;
        static const int PING_RESPONSE_DELAY = 15;

        /* Envoie un message PING à tous les clients */
        bool send_message(const std::string &message, int sender_fd);

        std::string getBackgroundColorCode(int socket);

        void printClientInfo(int newSocket, const std::string& host);



        /* Vérifie les réponses aux PING */

    private:

        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                 DONNÉES INTERNES                          */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Port d'écoute du serveur */
        unsigned short _port;

        /* Mot de passe requis pour se connecter */
        std::string _password;

        /* Nom du serveur */
        std::string _serverName;

        /* Socket d'écoute */
        int _listenSocket;

        /* Ensemble maître des fd */
        fd_set _masterSet;

        /* Valeur maximale de fd */
        int _fdMax;

        /* Liste des canaux du serveur */
        std::map<std::string, Channel*> _channels;

        /* Liste des clients connectés */
        std::vector<Client*> _clients;

        /* Bot associé au serveur */
        Bot _bot;



        /* Adresse IP du serveur */
        std::string _serverIp;

        std::vector<Client*> _clientsToRemove;


};

#endif /* SERVER_HPP */
