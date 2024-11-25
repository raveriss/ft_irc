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
 * Constructeur du bot, il definit les mots interdits
 */
Bot::Bot()
{
    /* Initialize with some default forbidden words */
    _forbiddenWords.insert("salade");
    _forbiddenWords.insert("tomate");
    _forbiddenWords.insert("oignon");
}

/**
 * Destructeur du bot, il vide les conteneurs contenant les mots interdits
 * et les clients qui ont recus des avertissements.
 */
Bot::~Bot()
{
    _forbiddenWords.clear();
    _warnings.clear();
}

/**
 * Convertit le message en minuscule, remplace les caracteres non alphanumeriques
 * par des espaces et decoupe le message en mots distincts.
 * Puis il verifie le message du client mot par mot, si un mot interdit est trouve et que
 * le client n'a pas encore recu d'avertissement, il envoie un avertissement.
 * Si le client a deja recu un avertissement, il est expulse du canal.
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
 * Definit le message d'avertissement en cas d'utilisation de mots interdits.
 * Cherche tous les clients presents dans le canal et envoie le message d'avertissement
 * et a qui il s'adresse.
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
 * Verifie que le client problematique est bien dans le canal.
 * Il definit le message de kick et l'envoie a tous les clients du canal.
 * Il retire le client problematique du canal, des operateurs et des invites
 * avec removeClient.
 * Puis il fait sortir le client du canal avec leaveChannel.
 */
void Bot::kickClient(Client *client, Channel *channel)
{
    /* Vérifier si le client est dans le canal */
    if (!channel->hasClient(client))
        return;

    /* Remplacez par le pseudo de votre bot */
    std::string botNickname = "Bot";
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
