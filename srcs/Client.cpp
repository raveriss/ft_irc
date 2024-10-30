/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:06:36 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/29 00:26:12 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Inclusions pour les clients et les canaux */
#include "../incs/Client.hpp"
#include "../incs/Channel.hpp"

/* Définit la durée d'inactivité maximale (par exemple 300 secondes) */
const time_t INACTIVITY_TIMEOUT = 300;

/**
 * Constructor
 */
Client::Client(int socket)
    : _socket(socket), _registered(false), _sentPass(false), _sentNick(false), 
        _sentUser(false), _isAway(false), _isOperator(false), _lastPongTime(0),
        _lastActivityTime(time(NULL)), pingReceived(false)
{
    /* Initialiser le temps de la dernière activité */
    _lastActivityTime = time(NULL);
}

/**
 * Destructor
 */
Client::~Client()
{
    close(_socket);
    for (std::set<Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        (*it)->removeClient(this);
    }
    _channels.clear();
}

/**
 * @return the socket of the client
 */
int Client::getSocket() const
{
    return _socket;
}

/**
 * @return true if the client is registered, false otherwise
 */
bool Client::isRegistered() const
{
    return _registered;
}

/**
 * Set the registered status of the client
 */
void Client::setRegistered(bool status)
{
    _registered = status;
}

/**
 * @return true if the client has sent the PASS command, false otherwise
 */
bool Client::hasSentPass() const
{
    return _sentPass;
}

/**
 * Set the sentPass status of the client
 */
void Client::setSentPass(bool status)
{
    _sentPass = status;
}

/**
 * @return the message buffer of the client
 */
std::string& Client::getMessageBuffer()
{ 
    return _messageBuffer; 
}

/**
 * @return true if the client has sent the NICK command, false otherwise
 */
bool Client::hasSentNick() const
{
    return _sentNick;
}

/**
 * Set the sentNick status of the client
 */
void Client::setSentNick(bool status)
{
    _sentNick = status;
}

/**
 * @return true if the client has sent the USER command, false otherwise
 */
bool Client::hasSentUser() const
{
    return _sentUser;
}

/**
 * Set the sentUser status of the client
 */
void Client::setSentUser(bool status)
{
    _sentUser = status;
}

/**
 * @return the nickname of the client
 */
const std::string &Client::getNickname() const
{
    return _nickname;
}

/**
 * Set the nickname of the client
 */
void Client::setNickname(const std::string &nickname)
{
    _nickname = nickname;
}

/**
 * @return the username of the client
 */
const std::string &Client::getUsername() const
{
    return _username;
}

/**
 * Set the username of the client
 */
void Client::setUsername(const std::string &username)
{
    _username = username;
}

/**
 * @return the hostname of the client
 */
const std::string &Client::getHostname() const
{
    return _hostname;
}

/**
 * Set the hostname of the client
 */
void Client::setHostname(const std::string &hostname)
{
    _hostname = hostname;
}

/**
 * @return the servername of the client
 */
const std::string &Client::getServername() const
{
    return _servername;
}

/**
 * Set the servername of the client
 */
void Client::setServername(const std::string &servername)
{
    _servername = servername;
}

/**
 * @return the realname of the client
 */
const std::string &Client::getRealname() const
{
    return _realname;
}

/**
 * Set the realname of the client
 */
void Client::setRealname(const std::string &realname)
{
    _realname = realname;
}

/**
 * Add a channel to the list of channels of the client
 */
void Client::joinChannel(Channel *channel)
{
    _channels.insert(channel);
}

/**
 * Remove a channel from the list of channels of the client
 */
void Client::leaveChannel(Channel *channel)
{
    _channels.erase(channel);
}

/**
 * @return true if the client is in the channel, false otherwise
 */
bool Client::isInChannel(const std::string &channelName) const
{
    for (std::set<Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        if ((*it)->getName() == channelName)
        {
            return true;
        }
    }
    return false;
}

/**
 * @return the list of channels of the client
 */
const std::set<Channel*> &Client::getChannels() const
{
    return _channels;
}

/**
 * Set the time of the last PONG received from the client
 */
void Client::setLastPongTime(time_t time)
{
    _lastPongTime = time;
}

/**
 * @return the time of the last PONG received from the client
 */
time_t Client::getLastPongTime() const
{
    return _lastPongTime;
}

/**
 * @return true if the client is away, false otherwise
 */
bool Client::isAway() const {
    return _isAway;
}

/**
 * Set the away status of the client
 */
void Client::setAway(bool status) {
    _isAway = status;
}

/**
 * @return true if the client is an operator, false otherwise
 */
bool Client::isOperator() const {
    return _isOperator;
}

/**
 * Set the operator status of the client
 */
void Client::setOperator(bool status) {
    _isOperator = status;
}

/**
 * @return true if the client is suspended, false otherwise
 */
bool Client::isSuspended() const {
    time_t currentTime = time(NULL);
    return (currentTime - _lastActivityTime) > INACTIVITY_TIMEOUT;
}

/**
 * Update the time of the last activity
 */
void Client::updateLastActivity() {
    
    /* Mettre à jour l'heure de la dernière activité */
    _lastActivityTime = time(NULL);
}

/**
 * Set the ping status of the client
 */
void Client::setPingStatus(bool status)
{
    pingReceived = status;
}

/**
 * @return true if the client has received a PING, false otherwise
 */
bool Client::isPingReceived() const
{
    return pingReceived;
}


