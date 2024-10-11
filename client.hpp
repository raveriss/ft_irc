// client.hpp

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>

class Client {
public:
    Client(int fd);
    ~Client();

    int getFd() const;
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getRealname() const; // Ajout de getRealname()

    bool isAuthenticated() const;
    bool isRegistered() const;
    bool isOperator() const;

    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setRealname(const std::string& realname); // Ajout de setRealname()

    void setPassword(const std::string& password);
    bool isPasswordValid(const std::string& serverPassword) const;

    void authenticate();
    void setOperator(bool value);

    void appendBuffer(const std::string& data);
    std::string getNextCommand();

    bool hasNickname() const;
    bool hasUsername() const;
    bool hasRealname() const; // Ajout de hasRealname()
    bool hasPassword() const;

    // Flood protection
    void incrementMessageCount();
    void resetMessageCount();
    int getMessageCount() const;
    time_t getLastMessageTime() const;
    void setLastMessageTime(time_t time);

private:
    int _fd;
    std::string _nickname;
    std::string _username;
    std::string _realname; // Ajout de _realname
    std::string _password;
    bool _authenticated;
    bool _registered;
    bool _isOperator;
    std::string _buffer;

    // Flood protection
    int _messageCount;
    time_t _lastMessageTime;
};

#endif // CLIENT_HPP
