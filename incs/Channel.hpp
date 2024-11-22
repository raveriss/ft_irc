/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:25:46 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/16 00:25:47 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

/* For std::string */
#include <string>

/* For std::vector */
#include <vector>

/* For std::set */
#include <set>

/* For std::map */
#include <map>

/* For std::find */
#include <algorithm>

/* For td::cout */
#include <iostream>

/* Déclaration anticipée de Client */
class Client;

/**
 * class Channel
 */
class Channel
{
    public:

        /* Constructeur */
        Channel(const std::string &name);

        /* Destructeur */
        ~Channel();

        /* Retourne le nom du canal */
        const std::string &getName() const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                   CLIENT                                  */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute un client au canal */
        void addClient(Client *client);

        /* Retire un client du canal */
        void removeClient(Client *client);

        /* Retourne true si le client est dans le canal, false sinon */
        bool hasClient(Client *client) const;

        /* Retourne la liste des clients du canal */
        const std::vector<Client*> &getClients() const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                    MODE                                   */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute un mode au canal */
        void setMode(char mode);

        /* Retire un mode du canal */
        void unsetMode(char mode);

        /* Retourne true si le canal a le mode, false sinon */
        bool hasMode(char mode) const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                    KEY                                    */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Définit la clé du canal */
        void setKey(const std::string &key);

        /* Retire la clé du canal */
        void unsetKey();

        /* Retourne true si la clé est correcte, false sinon */
        bool checkKey(const std::string &key) const;

        /* Retourne true si le canal a une clé, false sinon */
        bool hasKey() const;

        const std::string &getKey() const;



        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                   LIMIT                                   */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Définit la limite d'utilisateurs du canal */
        void setUserLimit(int limit);

        /* Retire la limite d'utilisateurs du canal */
        void unsetUserLimit();

        /* Retourne la limite d'utilisateurs du canal */
        int getUserLimit() const;

        /* Retourne true si le canal est plein, false sinon */
        bool isFull() const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                  OPERATOR                                 */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute un client à la liste des opérateurs */
        void addOperator(Client *client);

        /* Retire un client de la liste des opérateurs */
        void removeOperator(Client *client);

        /* Retourne true si le client est opérateur du canal, false sinon */
        bool isOperator(Client *client) const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                   INVITE                                  */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Invite un client dans le canal */
        void inviteClient(Client *client);

        /* Retourne true si le client est invité dans le canal, false sinon */
        bool isInvited(Client *client) const;

        /* Retire l'invitation d'un client */
        void removeInvitation(Client *client);


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                   TOPIC                                   */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Définit le sujet du canal */
        void setTopic(const std::string &topic);

        /* Retourne le sujet du canal */
        const std::string &getTopic() const;

        /* Retourne true si un sujet est défini, false sinon */
        bool hasTopic() const;

        /* Vérifie si le client a le statut de voix */
        bool hasVoice(const Client *client) const;
        

    private:

        /* Nom du canal */
        std::string _name;

        /* Liste des clients du canal */
        std::vector<Client*> _clients;

        /* Modes du canal */
        std::set<char> _modes;

        /* Clé du canal (mode 'k') */
        std::string _key;

        /* Limite d'utilisateurs (mode 'l') */
        int _userLimit;

        /* Liste des opérateurs */
        std::set<Client*> _operators;

        /* Liste des clients invités */
        std::set<Client*> _invitedClients;

        /* Sujet du canal */
        std::string _topic;

        /* true si un sujet est défini, false sinon */
        bool _hasTopic;

        /* Ensemble des clients avec le statut de voix */
        std::set<Client*> _voicedClients;

};

#endif /* CHANNEL_HPP */

