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

    const std::string& getName() const;

    void addClient(Client* client);
    void removeClient(Client* client);
    bool hasClient(Client* client) const;
    bool isEmpty() const;

    void broadcast(const std::string& message, Client* sender);

    // Operators and modes
    bool isOperator(Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);

    void setTopic(const std::string& topic);
    const std::string& getTopic() const;

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

    void addInvitedClient(Client* client);
    bool isInvited(Client* client) const;

    Client* getClientByNick(const std::string& nickname) const;

    std::string getModes() const;

private:
    std::string _name;
    std::set<Client*> _clients;
    std::set<Client*> _operators;
    std::string _topic;

    // Modes and settings
    bool _inviteOnly;
    bool _topicRestricted;
    std::string _key;
    int _userLimit;
    std::set<Client*> _invitedClients;
};

#endif // CHANNEL_HPP
