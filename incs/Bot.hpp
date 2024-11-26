/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:25:40 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/16 00:41:03 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
#define BOT_HPP

/* For std::string */
#include <string>

/* For std::set */
#include <vector>

/* For std::map */
#include <map>

/* For std::set */
#include <set>

/* For std::istringstream */
#include <sstream>


/* For send() */
#include <sys/socket.h>

/* Déclaration anticipée de Client */
class Client;

/* Déclaration anticipée de Channel */
class Channel;

/**
 * class Bot
 */
class Bot
{
    public:

        /* Constructeur */
        Bot();

        /* Destructeur */
        ~Bot();

        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                   MESSAGES                                */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Traite un message */
        void handleMessage(Client *client, Channel *channel, const std::string &message);


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                 SANCTIONS                                 */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute un avertissement */
        void sendWarning(Client *client, Channel *channel);

        /* Exclut un client */
        void kickClient(Client *client, Channel *channel);

    private:
    
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                              DONNÉES INTERNES                             */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Mots interdits */
        std::set<std::string> _forbiddenWords;

        /* Avertissements */
        std::map<Client*, int> _warnings;
};

#endif /* BOT_HPP */