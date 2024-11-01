/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:25:54 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/01 01:34:18 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

/* For std::string */
#include <string>

/* For close() */
#include <unistd.h>

/* For std::set */
#include <set>

/* For time_t */
#include <ctime>

/* Déclaration anticipée de Channel */
class Channel;

/**
 * class Client
 */
class Client
{
    public:

        /* Constructeur */
        Client(int socket);

        /* Destructeur */
        ~Client();

        /* Retourne le socket du client */
        int getSocket() const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                              ENREGISTREMENT                               */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Retourne true si le client est enregistré, false sinon */
        bool isRegistered() const;

        /* Définit l'état enregistré du client */
        void setRegistered(bool status);

        /* Retourne true si le client a envoyé la commande PASS, false sinon */
        bool hasSentPass() const;

        /* Définit l'état sentPass du client */
        void setSentPass(bool status);

        /* Retourne true si le client a envoyé la commande NICK, false sinon */
        bool hasSentNick() const;

        /* Définit l'état sentNick du client */
        void setSentNick(bool status);

        /* Retourne true si le client a envoyé la commande USER, false sinon */
        bool hasSentUser() const;

        /* Définit l'état sentUser du client */
        void setSentUser(bool status);


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                  STATUT                                   */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Vérifier si le client est "away" */
        bool isAway() const;

        /* Définit l'état "away" du client */
        void setAway(bool status);
        
        /* Vérifier si le client est opérateur */
        bool isOperator() const;

        /* Définit l'état opérateur du client */
        void setOperator(bool status);

        bool isSuspended() const;

        /* Méthode pour mettre à jour l'heure de dernière activité */
        void updateLastActivity();


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                   IDENTITÉ                                */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Retourne le pseudonyme du client */
        const std::string &getNickname() const;

        /* Définit le pseudonyme du client */
        void setNickname(const std::string &nickname);

        /* Retourne le nom d'utilisateur du client */
        const std::string &getUsername() const;

        /* Définit le nom d'utilisateur du client */
        void setUsername(const std::string &username);

        /* Retourne le nom d'hôte du client */
        const std::string &getHostname() const;

        /* Définit le nom d'hôte du client */
        void setHostname(const std::string &hostname);

        /* Retourne le nom du serveur du client */
        const std::string &getServername() const;

        /* Définit le nom du serveur du client */
        void setServername(const std::string &servername);

        /* Retourne le nom réel du client */
        const std::string &getRealname() const;

        /* Définit le nom réel du client */
        void setRealname(const std::string &realname);


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                  CHANNEL                                  */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute un canal à la liste */
        void joinChannel(Channel *channel);

        /* Retire un canal de la liste des canaux du client */
        void leaveChannel(Channel *channel);

        /* Retourne true si le client est dans le canal, false sinon */
        bool isInChannel(const std::string &channelName) const;

        /* Retourne la liste des canaux du client */
        const std::set<Channel*> &getChannels() const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                  PONGTIME                                 */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Définit le temps du dernier pong */
        void setLastPongTime(time_t time);

        /* Retourne le temps du dernier pong */
        time_t getLastPongTime() const;

        /* Getter pour le tampon de messages partiels */
        std::string & getMessageBuffer();

        /* Setter pour le PING status */
        void setPingStatus(bool status);

        /* Indique si le client a reçu un PING */
        bool isPingReceived() const;


    private:

        /* Socket du client */
        int _socket;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                           CLIENT IDENTIFICATION                           */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Pseudonyme */
        std::string _nickname;

        /* Username */
        std::string _username;

        /* Hostname */
        std::string _hostname;

        /* Server name */
        std::string _servername;

        /* Real name */
        std::string _realname;

        /* Tampon pour stocker les messages partiels */
        std::string _messageBuffer;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                            CLIENT REGISTRATION STATES                     */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Indique si le client est complètement enregistré sur le serveur IRC */
        bool _registered;

        /* Indique si le client a envoyé la commande PASS avec succès */
        bool _sentPass;

        /* Indique si le client a envoyé la commande NICK avec succès */
        bool _sentNick;

        /* Indique si le client a envoyé la commande USER avec succès */
        bool _sentUser;

        /* Ajout du membre pour l'état "away" */
        bool _isAway;

        /* Indique si le client est opérateur */
        bool _isOperator;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                              CLIENT CHANNELS                              */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Liste des canaux du client */
        std::set<Channel*> _channels;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                             CLIENT PING STATUS                            */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Temps du dernier pong */
        time_t _lastPongTime;

        /* Temps de la dernière activité */
        time_t _lastActivityTime;

        /* Indique si le client a reçu un PING */
        bool pingReceived;


};

#endif /* CLIENT_HPP */
