#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <unistd.h>

enum RegistrationState {
    PASS_REQUIRED,
    NICK_REQUIRED,
    USER_REQUIRED,
    REGISTERED
};

class Client {
private:
    int fd;
    std::string nickname;
    std::string username;
    RegistrationState state;  // Ajout de l'état d'enregistrement


public:
    Client();
    Client(int fd);
    ~Client();

    int getFd() const;
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    std::string getNickname() const;
    std::string getUsername() const;

    RegistrationState getState() const;  // Obtenir l'état
    void setState(RegistrationState newState);  // Modifier l'état
};

#endif
