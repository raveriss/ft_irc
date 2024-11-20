/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:21:24 by raveriss          #+#    #+#             */
/*   Updated: 2024/11/19 00:14:18 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Inclusions pour le bot, les clients et les canaux */
#include "../incs/Bot.hpp"
#include "../incs/Client.hpp"
#include "../incs/Channel.hpp"

/* For std::transform */
#include <algorithm>

/**
 * Constructeur
 */
Bot::Bot()
{
    /* Initialize with some default forbidden words */
    _forbiddenWords.insert("salade");
    _forbiddenWords.insert("tomate");
    _forbiddenWords.insert("oignon");
}

/**
 * Destructeur
 */
Bot::~Bot()
{
    
    /* Vider les conteneurs */
    _forbiddenWords.clear();

    /* Vider les avertissements */
    _warnings.clear();
}

/**
 * Ajoute un mot interdit
 */
void Bot::addForbiddenWord(const std::string &word)
{
    _forbiddenWords.insert(word);
}

/**
 * Retire un mot interdit
 */
void Bot::removeForbiddenWord(const std::string &word)
{
    _forbiddenWords.erase(word);
}

/**
 * Vérifie si un mot est interdit
 */
bool Bot::isForbiddenWord(const std::string &word) const
{
    return _forbiddenWords.find(word) != _forbiddenWords.end();
}

/**
 * Traite un message
 */
void Bot::handleMessage(Client *client, Channel *channel, const std::string &message)
{
    /* Convertir le message en minuscule */
    std::string lowerMessage = message;
    std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);

    /* Remplacer tous les caractères non alphanumériques par des espaces */
    for (size_t i = 0; i < lowerMessage.size(); ++i)
    {
        if (!isalnum(lowerMessage[i]) && !isspace(lowerMessage[i]))
        {
            lowerMessage[i] = ' ';
        }
    }

    /* Découper le message en mots distincts */
    std::istringstream iss(lowerMessage);
    std::string word;

    while (iss >> word)
    {
        /* Si le mot figure dans la liste des mots interdits */
        if (!word.empty() && _forbiddenWords.find(word) != _forbiddenWords.end())
        {
            if (_warnings[client] == 0)
            {
                sendWarning(client, channel);
                _warnings[client]++;
            }
            else
            {
                kickClient(client, channel);
                _warnings.erase(client);
            }
            return;
        }
    }
}





/**
 * Envoie un avertissement à un client
 */
void Bot::sendWarning(Client *client, Channel *channel)
{
    std::string warningMessage = "WARNING: Inappropriate language detected. Further violations will result in a kick.";
    std::string message = ":" + client->getNickname() + " PRIVMSG " + channel->getName() + " :" + warningMessage + "\r\n";

    const std::vector<Client*> &clients = channel->getClients();
    for (std::vector<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
        send((*it)->getSocket(), message.c_str(), message.length(), 0);
    }
}


/**
 * Expulse un client du canal
 */
void Bot::kickClient(Client *client, Channel *channel)
{
    /* Vérifier si le client est dans le canal */
    if (!channel->hasClient(client))
        return;

    /* Remplacez par le pseudo de votre bot */
    std::string botNickname = "Bot";

    /* Formater le message de kick selon le protocole IRC */
    std::string kickMessage = ":" + botNickname + " KICK " + channel->getName() + " " + client->getNickname() + " :You have been kicked for inappropriate language.\r\n";

    /* Envoyer le message de kick à tous les clients du canal */
    const std::vector<Client*> &channelClients = channel->getClients();
    for (std::vector<Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
    {
        send((*it)->getSocket(), kickMessage.c_str(), kickMessage.length(), 0);
    }

    /* Retirer le client du canal */
    channel->removeClient(client);
    client->leaveChannel(channel);
}
