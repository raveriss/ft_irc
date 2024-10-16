#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <unistd.h>


class Client {
private:
    int fd;
    std::string nickname;
    std::string username;

public:
    Client(int fd);
    ~Client();

    int getFd() const;
    void setNickname(const std::string& nick);
    void setUsername(const std::string& user);
    std::string getNickname() const;
    std::string getUsername() const;
};

#endif
