// channel.cpp

#include "channel.hpp"
#include <algorithm>

Channel::Channel(const std::string& name)
    : _name(name), _inviteOnly(false), _topicRestricted(false), _userLimit(0) {
}

Channel::~Channel() {
}

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}

void Channel::addClient(Client* client) {
    _clients.insert(client);
    // Si le canal vient d'être créé, le premier client est automatiquement opérateur
    if (_clients.size() == 1) {
        addOperator(client);
    }
}

void Channel::removeClient(Client* client) {
    _clients.erase(client);
    _operators.erase(client); // Retirer le client des opérateurs s'il en faisait partie
    _invitedClients.erase(client); // Retirer le client de la liste des invités
}

bool Channel::hasClient(Client* client) const {
    return _clients.find(client) != _clients.end();
}

bool Channel::isEmpty() const {
    return _clients.empty();
}

int Channel::getClientCount() const {
    return _clients.size();
}

const std::set<Client*>& Channel::getClients() const {
    return _clients;
}

// void Channel::broadcast(const std::string& message, Client* sender) {
//     for (std::set<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
//         if (*it != sender) {
//             // Fonction pour envoyer un message au client
//             // Vous devrez implémenter sendToClient dans votre classe Server ou ailleurs
//             // sendToClient(*it, message);
//         }
//     }
// }

bool Channel::isOperator(Client* client) const {
    return _operators.find(client) != _operators.end();
}

void Channel::addOperator(Client* client) {
    _operators.insert(client);
}

void Channel::removeOperator(Client* client) {
    _operators.erase(client);
}

void Channel::setInviteOnly(bool value) {
    _inviteOnly = value;
}

bool Channel::isInviteOnly() const {
    return _inviteOnly;
}

void Channel::setTopicRestriction(bool value) {
    _topicRestricted = value;
}

bool Channel::isTopicRestricted() const {
    return _topicRestricted;
}

void Channel::setKey(const std::string& key) {
    _key = key;
}

void Channel::removeKey() {
    _key.clear();
}

bool Channel::hasKey() const {
    return !_key.empty();
}

const std::string& Channel::getKey() const {
    return _key;
}

void Channel::setUserLimit(int limit) {
    _userLimit = limit;
}

void Channel::removeUserLimit() {
    _userLimit = 0;
}

bool Channel::hasUserLimit() const {
    return _userLimit > 0;
}

int Channel::getUserLimit() const {
    return _userLimit;
}

void Channel::addInvitedClient(Client* client) {
    _invitedClients.insert(client);
}

bool Channel::isInvited(Client* client) const {
    return _invitedClients.find(client) != _invitedClients.end();
}

Client* Channel::getClientByNick(const std::string& nickname) const {
    for (std::set<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if ((*it)->getNickname() == nickname) {
            return *it;
        }
    }
    return NULL;
}

std::string Channel::getModes() const {
    std::string modes = "+";
    if (_inviteOnly)
        modes += "i";
    if (_topicRestricted)
        modes += "t";
    if (hasKey())
        modes += "k";
    if (hasUserLimit())
        modes += "l";
    return modes;
}
