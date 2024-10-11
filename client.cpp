// client.cpp

#include "client.hpp"
#include <unistd.h>

Client::Client(int fd)
: _fd(fd), _authenticated(false), _registered(false), _isOperator(false),
      _messageCount(0), _lastMessageTime(time(NULL)) {
}

Client::~Client() {
    close(_fd);
}

int Client::getFd() const {
    return _fd;
}

const std::string& Client::getNickname() const {
    return _nickname;
}

const std::string& Client::getUsername() const {
    return _username;
}

const std::string& Client::getRealname() const { // Implémentation de getRealname()
    return _realname;
}

bool Client::isAuthenticated() const {
    return _authenticated;
}

bool Client::isRegistered() const {
    return _registered;
}

bool Client::isOperator() const {
    return _isOperator;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}

void Client::setRealname(const std::string& realname) { // Implémentation de setRealname()
    _realname = realname;
}

void Client::setPassword(const std::string& password) {
    _password = password;
}

bool Client::isPasswordValid(const std::string& serverPassword) const {
    return _password == serverPassword;
}

void Client::authenticate() {
    _authenticated = true;
}

void Client::setOperator(bool value) {
    _isOperator = value;
}

void Client::appendBuffer(const std::string& data) {
    _buffer += data;
}

std::string Client::getNextCommand() {
    std::string::size_type pos = _buffer.find("\r\n");
    if (pos != std::string::npos) {
        std::string command = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 2);
        return command;
    }
    return "";
}

bool Client::hasNickname() const {
    return !_nickname.empty();
}

bool Client::hasUsername() const {
    return !_username.empty();
}

bool Client::hasRealname() const { // Implémentation de hasRealname()
    return !_realname.empty();
}

bool Client::hasPassword() const {
    return !_password.empty();
}

// Flood protection methods

void Client::incrementMessageCount() {
    _messageCount++;
}

void Client::resetMessageCount() {
    _messageCount = 0;
}

int Client::getMessageCount() const {
    return _messageCount;
}

time_t Client::getLastMessageTime() const {
    return _lastMessageTime;
}

void Client::setLastMessageTime(time_t time) {
    _lastMessageTime = time;
}
