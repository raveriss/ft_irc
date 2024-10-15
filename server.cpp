#include "server.hpp"

Server::Server(int port, const std::string &password)
: port(port), _password(password)
{
    std::cout << "Server is starting on port: " << this->port << std::endl; // Log clair
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating socket\n";
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options\n";
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(this->port);  // Utilisation de this->port

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket\n";
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        std::cerr << "Error listening on socket\n";
        exit(EXIT_FAILURE);
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
}

Server::~Server() {
    stop();
}

std::string Server::parseCommand(const std::string &message) {
    // Analyse la première partie du message pour extraire la commande
    std::istringstream iss(message);
    std::string command;
    iss >> command;
    return command;
}

std::string Server::extractPassword(const std::string &message) {
    // Analyse et retourne le paramètre de la commande PASS
    std::istringstream iss(message);
    std::string command, password;
    iss >> command >> password;
    return password;
}


std::string Server::receiveMessage(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        return "";
    }
    buffer[bytes_received] = '\0';
    return std::string(buffer);
}


std::string Server::extractNickname(const std::string &message) {
    std::istringstream iss(message);
    std::string command, nickname;
    iss >> command >> nickname;
    return nickname;
}

bool Server::isNicknameInUse(const std::string &nickname) {
    for (std::map<int, Client>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second.getNickname() == nickname) {
            return true;
        }
    }
    return false;
}


std::string Server::extractUsername(const std::string &message) {
    std::istringstream iss(message);
    std::string command, user, mode, unused, realname;
    iss >> command >> user >> mode >> unused;
    std::getline(iss, realname);  // Récupère le realname, qui peut contenir des espaces
    return user;
}


void Server::handleClientMessage(int client_fd) {
    // Vérifier si le client existe déjà dans la map avant de l'utiliser
    std::map<int, Client>::iterator it = clients.find(client_fd);
    if (it == clients.end()) {
        std::cerr << "Client not found for FD: " << client_fd << std::endl;
        return;
    }

    // Récupérer le client existant à partir de la map
    Client& client = it->second;

    // Recevoir le message du client
    std::string message = receiveMessage(client_fd);
    
    // Extraire la commande du message reçu
    std::string command = parseCommand(message);

    if (command == "PASS") {
        if (client.getState() != PASS_REQUIRED) {
            sendMessage(client_fd, "462 :You may not reregister\r\n");  // ERR_ALREADYREGISTRED
            return;
        }

        std::string password = extractPassword(message);
        if (password != this->_password) {
            sendMessage(client_fd, "464 :Password incorrect\r\n");  // ERR_PASSWDMISMATCH
            removeClient(client_fd);  // Déconnecter le client si le mot de passe est incorrect
        } else {
            client.setState(NICK_REQUIRED);
        }
    } 
    else if (command == "NICK") {
        if (client.getState() == PASS_REQUIRED) {
            sendMessage(client_fd, "451 :You have not registered\r\n");  // ERR_NOTREGISTERED
            return;
        }

        std::string nickname = extractNickname(message);
        if (isNicknameInUse(nickname)) {
            sendMessage(client_fd, "433 :Nickname is already in use\r\n");  // ERR_NICKNAMEINUSE
        } else {
            client.setNickname(nickname);
            client.setState(USER_REQUIRED);
        }
    } 
    else if (command == "USER") {
        if (client.getState() != USER_REQUIRED) {
            sendMessage(client_fd, "451 :You have not registered\r\n");  // ERR_NOTREGISTERED
            return;
        }

        std::string username = extractUsername(message);
        client.setUsername(username);
        client.setState(REGISTERED);

        // Envoie le message de bienvenue
        sendMessage(client_fd, ":server 001 " + client.getNickname() + " :Welcome to the IRC network\r\n");
    }
}




void Server::run() {
    while (true) {
        read_fds = master_fds;

        // Appel à select
        int activity = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) {
                continue;  // Signal interrompu, continue la boucle
            }
            std::cerr << "Error on select: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) {
                    handleNewConnection();
                } else {
                    handleClientMessage(i);
                }
            }
        }
    }
}


void Server::handleNewConnection() {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        std::cerr << "Error accepting new connection" << std::endl;
        return;
    }

    // Créer un nouvel objet Client avec le descripteur de fichier client_fd
    Client newClient(client_fd);

    // Ajouter le nouveau client dans la map clients
    clients[client_fd] = newClient;

    // Ajouter le descripteur du client au set de descripteurs surveillés
    FD_SET(client_fd, &master_fds);

    std::cout << "New client connected on FD: " << client_fd << std::endl;
}




void Server::removeClient(int client_fd) {
    if (clients.find(client_fd) != clients.end()) {
        close(client_fd);
        FD_CLR(client_fd, &master_fds);

        // Supprimer le client de la map
        clients.erase(client_fd);

        std::cout << "Client disconnected from FD: " << client_fd << std::endl;
    } else {
        std::cerr << "Attempted to remove a non-existent client with FD: " << client_fd << std::endl;
    }
}



void Server::stop() {
    close(server_fd);
}

// Helper function to receive password from client
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


// Helper function to send messages to client
void Server::sendMessage(int client_fd, const std::string &message) {
    send(client_fd, message.c_str(), message.size(), 0);
}

