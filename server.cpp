#include "server.hpp"

Server::Server(int port, const std::string &password)
: _port(port), _password(password)
{
    std::cout << "Server is starting on port: " << this->_port << std::endl; // Log clair
    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0) {
        std::cerr << "Error creating socket\n";
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(this->_port);  // Utilisation de this->port

    if (bind(_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket\n";
        exit(EXIT_FAILURE);
    }

    if (listen(_server_fd, MAX_CLIENTS) < 0) {
        std::cerr << "Error listening on socket\n";
        exit(EXIT_FAILURE);
    }

    fcntl(_server_fd, F_SETFL, O_NONBLOCK);
    FD_ZERO(&master_fds);
    FD_SET(_server_fd, &master_fds);
}

Server::~Server() {
    stop();
}

void Server::run() {
    while (true) {
        read_fds = master_fds;
        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            std::cerr << "Error on select\n";
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == _server_fd) {
                    handleNewConnection();
                } else {
                    handleClientMessage(i);
                }
            }
        }
    }
}

void Server::handleNewConnection() {
    int client_fd = accept(_server_fd, NULL, NULL);
    if (client_fd < 0) {
        std::cerr << "Error accepting new connection\n";
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    // Ajouter le client à la liste des descripteurs de fichiers
    FD_SET(client_fd, &master_fds);
    client_fds.insert(client_fd);
    clients[client_fd] = "";  // Le client n'a pas encore été authentifié
    std::cout << "Client connected, waiting for password...\n";  // Pas encore authentifié
}


void Server::handleClientMessage(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    
    if (bytes_received <= 0) {
        removeClient(client_fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message = buffer;

    // Si le client n'a pas encore envoyé le mot de passe
    if (clients[client_fd].empty()) {
        // Vérifier si la commande est une commande PASS
        if (message.substr(0, 5) == "PASS ") {
            std::string clientPassword = message.substr(5);
            clientPassword.erase(std::remove(clientPassword.begin(), clientPassword.end(), '\n'), clientPassword.end());
            clientPassword.erase(std::remove(clientPassword.begin(), clientPassword.end(), '\r'), clientPassword.end());

            if (clientPassword != _password) {
                sendMessage(client_fd, "Error: Incorrect password.\n");
                removeClient(client_fd);
            } else {
                sendMessage(client_fd, "Password accepted. You are now connected.\n");
                clients[client_fd] = "authenticated";  // Marquer le client comme authentifié
            }
        } else {
            sendMessage(client_fd, "Please send PASS <password> to authenticate.\n");
        }
    } else {
        // Gérer les autres messages des clients authentifiés
        if (message == "/quit\n") {
            removeClient(client_fd);
        } else {
            for (std::set<int>::iterator it = client_fds.begin(); it != client_fds.end(); ++it) {
                if (*it != client_fd) {
                    send(*it, message.c_str(), message.size(), 0);
                }
            }
        }
    }
}

void Server::removeClient(int client_fd) {
    close(client_fd);
    FD_CLR(client_fd, &master_fds);
    client_fds.erase(client_fd);
    clients.erase(client_fd);
    std::cout << "Client disconnected\n";
}

void Server::stop() {
    close(_server_fd);
}

std::string Server::receivePasswordFromClient(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string password = std::string(buffer);
        // Remove any trailing newline or carriage return characters
        password.erase(std::remove(password.begin(), password.end(), '\n'), password.end());
        password.erase(std::remove(password.begin(), password.end(), '\r'), password.end());
        return password;
    }
    return "";
}


void Server::sendMessage(int client_fd, const std::string &message) {
    send(client_fd, message.c_str(), message.size(), 0);
}
