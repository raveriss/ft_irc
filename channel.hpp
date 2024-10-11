// channel.hpp

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>
#include "client.hpp"

class Channel {
public:
    Channel(const std::string& name);
    ~Channel();

    // Accesseurs de base
    const std::string& getName() const;
    const std::string& getTopic() const;
    std::string getModes() const;

    // Gestion des clients
    void addClient(Client* client);
    void removeClient(Client* client);
    bool hasClient(Client* client) const;
    bool isEmpty() const;
    int getClientCount() const; // Ajout de cette méthode
    const std::set<Client*>& getClients() const; // Ajout de cette méthode

    // Diffusion de messages
    // void broadcast(const std::string& message, Client* sender);

    // Gestion des opérateurs
    bool isOperator(Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);

    // Gestion du topic
    void setTopic(const std::string& topic);

    // Modes et paramètres
    void setInviteOnly(bool value);
    bool isInviteOnly() const;

    void setTopicRestriction(bool value);
    bool isTopicRestricted() const;

    void setKey(const std::string& key);
    void removeKey();
    bool hasKey() const;
    const std::string& getKey() const;

    void setUserLimit(int limit);
    void removeUserLimit();
    bool hasUserLimit() const;
    int getUserLimit() const;

    // Gestion des invitations
    void addInvitedClient(Client* client);
    bool isInvited(Client* client) const;

    // Recherche de client
    Client* getClientByNick(const std::string& nickname) const;

private:
    std::string _name;
    std::set<Client*> _clients;
    std::set<Client*> _operators;
    std::string _topic;

    // Modes et paramètres
    bool _inviteOnly;
    bool _topicRestricted;
    std::string _key;
    int _userLimit;
    std::set<Client*> _invitedClients;
    Server* _server;
};

#endif // CHANNEL_HPP
