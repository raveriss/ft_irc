/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:21:21 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/31 19:45:49 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Inclusions pour les canaux et les clients */
#include "../incs/Channel.hpp"
#include "../incs/Client.hpp"

/**
 * Constructor
 */
Channel::Channel(const std::string &name)
: _name(name), _userLimit(0), _hasTopic(false)
{}

/**
 * Destructor
 */
Channel::~Channel()
{
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        (*it)->leaveChannel(this);
    }
    _clients.clear();
}

/**
 * @return the name of the channel
 */
const std::string &Channel::getName() const
{
    return _name;
}

/**
 * Add a client to the channel
 */
void Channel::addClient(Client *client)
{
    if (!hasClient(client))
    {
        _clients.push_back(client);
    }
}

/**
 * Remove a client from the channel
 */
void Channel::removeClient(Client *client)
{
    std::cout << "Tentative de suppression du client du canal..." << std::endl;

    // Suppression du client de la liste des clients
    _clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());
    std::cout << "Client supprimé du vecteur _clients" << std::endl;

    // Suppression du client de la liste des opérateurs
    if (_operators.erase(client) > 0) {
        std::cout << "Client supprimé de _operators" << std::endl;
    }

    // Suppression du client de la liste des invités
    if (_invitedClients.erase(client) > 0) {
        std::cout << "Client supprimé de _invitedClients" << std::endl;
    }
}

/**
 * @return true if the client is in the channel, false otherwise
 */
bool Channel::hasClient(Client *client) const
{
    return std::find(_clients.begin(), _clients.end(), client) != _clients.end();
}

/**
 * @return the list of clients in the channel
 */
const std::vector<Client*> &Channel::getClients() const
{
    return _clients;
}

/**
 * Set a mode to the channel
 */
void Channel::setMode(char mode)
{
    _modes.insert(mode);
}

/**
 * Remove a mode from the channel
 */
void Channel::unsetMode(char mode)
{
    _modes.erase(mode);
}

/**
 * @return true if the channel has the mode, false otherwise
 */
bool Channel::hasMode(char mode) const
{
    return _modes.find(mode) != _modes.end();
}

/**
 * Set the key of the channel
 */
void Channel::setKey(const std::string &key)
{
    _key = key;
    setMode('k');
}

/**
 * Remove the key of the channel
 */
void Channel::unsetKey()
{
    _key.clear();
    unsetMode('k');
}

/**
 * @return true if the key is correct, false otherwise
 */
bool Channel::checkKey(const std::string &key) const
{
    return _key == key;
}

/**
 * @return true if the channel has a key, false otherwise
 */
bool Channel::hasKey() const
{
    return hasMode('k');
}

/* Gestion de la limite d'utilisateurs */
void Channel::setUserLimit(int limit)
{
    _userLimit = limit;
    setMode('l');
}

/**
 * Remove the user limit of the channel
 */
void Channel::unsetUserLimit()
{
    _userLimit = 0;
    unsetMode('l');
}

/**
 * @return the user limit of the channel
 */
int Channel::getUserLimit() const
{
    return _userLimit;
}

/**
 * @return true if the channel is full, false otherwise
 */
bool Channel::isFull() const
{
    return hasMode('l') && static_cast<int>(_clients.size()) >= _userLimit;
}

/**
 * Add a client to the operators list
 */
void Channel::addOperator(Client *client)
{
    _operators.insert(client);
}

/**
 * Remove a client from the operators list
 */
void Channel::removeOperator(Client *client)
{
    _operators.erase(client);
}

/**
 * @return true if the client is an operator of the channel, false otherwise
 */
bool Channel::isOperator(Client *client) const
{
    return _operators.find(client) != _operators.end();
}

/**
 * Invite a client to the channel
 */
void Channel::inviteClient(Client *client)
{
    _invitedClients.insert(client);
}

/**
 * @return true if the client is invited to the channel, false otherwise
 */
bool Channel::isInvited(Client *client) const
{
    return _invitedClients.find(client) != _invitedClients.end();
}

/**
 * Remove an invitation for a client
 */
void Channel::removeInvitation(Client *client)
{
    _invitedClients.erase(client);
}

/**
 * Set the topic of the channel
 */
void Channel::setTopic(const std::string &topic)
{
    _topic = topic;
    _hasTopic = true;
}

/**
 * @return the topic of the channel
 */
const std::string &Channel::getTopic() const
{
    return _topic;
}

/**
 * @return true if the channel has a topic, false otherwise
 */
bool Channel::hasTopic() const
{
    return _hasTopic;
}

/**
 * Return true if the client has voice, false otherwise
 */
bool Channel::hasVoice(const Client *client) const {
    return _voicedClients.find(const_cast<Client*>(client)) != _voicedClients.end();
}
