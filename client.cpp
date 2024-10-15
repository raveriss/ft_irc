#include "client.hpp"

Client::Client()
: fd(-1), state(PASS_REQUIRED)
{}


Client::Client(int fd) : fd(fd), state(PASS_REQUIRED) {}

Client::~Client() {
    close(fd);
}

int Client::getFd() const {
    return fd;
}

void Client::setNickname(const std::string& nick) {
    nickname = nick;
}

void Client::setUsername(const std::string& user) {
    username = user;
}

std::string Client::getNickname() const {
    return nickname;
}

std::string Client::getUsername() const {
    return username;
}

RegistrationState Client::getState() const {
    return state;
}

void Client::setState(RegistrationState newState) {
    state = newState;
}
