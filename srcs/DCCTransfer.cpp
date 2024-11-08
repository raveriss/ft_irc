/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DCCTransfer.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 21:16:05 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/31 19:26:11 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Inclusions pour les transferts DCC, le serveur, et les clients */
#include "../incs/DCCTransfer.hpp"
#include "../incs/Server.hpp"
#include "../incs/Client.hpp"

/**
 * Constructeur
 */
DCCTransfer::DCCTransfer(Server* server, Client* sender, Client* receiver, const std::string& filename, uint32_t filesize)
    : _server(server), _sender(sender), _receiver(receiver), _filename(filename), _filesize(filesize),
      _listenSocket(-1), _senderSocket(-1), _receiverSocket(-1), _bytesTransferred(0), _state(WAITING_FOR_CONNECTIONS)
{
}

/**
 * Destructeur
 */
DCCTransfer::~DCCTransfer()
{
    if (_listenSocket >= 0) {
        close(_listenSocket);
        std::cout << "Socket d'écoute de DCCTransfer fermé." << std::endl;
    }
    if (_senderSocket >= 0) {
        close(_senderSocket);
        std::cout << "Socket d'envoi de DCCTransfer fermé." << std::endl;
    }
    if (_receiverSocket >= 0) {
        close(_receiverSocket);
        std::cout << "Socket de réception de DCCTransfer fermé." << std::endl;
    }
}

/**
 * Démarre le transfert
 */
bool DCCTransfer::start()
{
    /* Création du socket d'écoute */
    _listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSocket < 0)
    {
        perror("socket");
        return false;
    }

    /* Configuration du socket */
    int opt = 1;
    if (setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(_listenSocket);
        return false;
    }

    /* Lier le socket à une adresse et un port disponibles */
    sockaddr_in addr;

    /* Initialiser la structure à zéro */
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Lier le socket à un port aléatoire */
    addr.sin_port = 0;

    if (bind(_listenSocket, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
        perror("bind");
        close(_listenSocket);
        return false;
    }

    /* Récupérer le numéro de port assigné */
    socklen_t addrLen = sizeof(addr);
    if (getsockname(_listenSocket, (sockaddr*)&addr, &addrLen) == -1)
    {
        perror("getsockname");
        close(_listenSocket);
        return false;
    }

    /* Convertir le port en ordre hôte */
    _listenPort = ntohs(addr.sin_port);

    if (listen(_listenSocket, 2) < 0)
    {
        perror("listen");
        close(_listenSocket);
        return false;
    }

    /* Mettre le socket en mode non-bloquant */
    if (fcntl(_listenSocket, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl");
        close(_listenSocket);
        return false;
    }

    /* Pour vérification, afficher le port assigné */
    std::cout << "DCC Transfer listening on port: " << _listenPort << std::endl;

    return true;
}

/**
 * Traite les transferts
 */
void DCCTransfer::process(fd_set& readSet, fd_set& writeSet)
{
    if (_state == WAITING_FOR_CONNECTIONS)
    {
        if (FD_ISSET(_listenSocket, &readSet))
        {
            int newSocket = accept(_listenSocket, NULL, NULL);
            if (newSocket >= 0)
            {
                // Mettre le socket en mode non-bloquant
                if (fcntl(newSocket, F_SETFL, O_NONBLOCK) < 0)
                {
                    perror("fcntl");
                    close(newSocket);
                    _state = ERROR;
                    return;
                }

                // Identifier le client (expéditeur ou destinataire)
                if (_senderSocket < 0 && newSocket == _sender->getSocket())
                {
                    _senderSocket = newSocket;
                }
                else if (_receiverSocket < 0 && newSocket == _receiver->getSocket())
                {
                    _receiverSocket = newSocket;
                }
                else
                {
                    if (_senderSocket < 0)
                    {
                        _senderSocket = newSocket;
                    }
                    else if (_receiverSocket < 0)
                    {
                        _receiverSocket = newSocket;
                    }
                    else
                    {
                        close(newSocket);
                        return;
                    }
                }

                // Ajouter le socket au set maître du serveur
                FD_SET(newSocket, &_server->getMasterSet());
                if (newSocket > _server->getFdMax())
                {
                    _server->setFdMax(newSocket);
                }
            }
        }

        if (_senderSocket >= 0 && _receiverSocket >= 0)
        {
            _state = TRANSFERRING;
        }
    }
    else if (_state == TRANSFERRING)
    {
        if (FD_ISSET(_senderSocket, &readSet))
        {
            char buffer[4096];
            ssize_t bytesRead = recv(_senderSocket, buffer, sizeof(buffer), 0);
            if (bytesRead > 0)
            {
                _buffer.insert(_buffer.end(), buffer, buffer + bytesRead);
                std::cout << "Données lues depuis l'expéditeur : " << bytesRead << " octets" << std::endl;
            }
            else if (bytesRead == 0)
            {
                // L'expéditeur a fermé la connexion
                _state = COMPLETED;
            }
            else if (bytesRead < 0 && errno != EWOULDBLOCK)
            {
                perror("recv");
                _state = ERROR;
            }
        }

        if (FD_ISSET(_receiverSocket, &writeSet) && !_buffer.empty())
        {
            ssize_t bytesSent = send(_receiverSocket, &_buffer[0], _buffer.size(), 0);
            if (bytesSent > 0)
            {
                _buffer.erase(_buffer.begin(), _buffer.begin() + bytesSent);
                _bytesTransferred += bytesSent;
                std::cout << "Données envoyées au récepteur : " << bytesSent << " octets, Total transféré : " << _bytesTransferred << " / " << _filesize << std::endl;
            }
            else if (bytesSent < 0 && errno != EWOULDBLOCK)
            {
                perror("send");
                _state = ERROR;
            }
        }

        // Vérifier si le transfert est complet
        if (_bytesTransferred >= _filesize)
        {
            _state = COMPLETED;
        }
    }

    if (_state == COMPLETED)
    {
        std::string completeMsg = ":" + _server->getServerName() + " NOTICE " + _sender->getNickname() + " :Transfert terminé\r\n";
        send(_sender->getSocket(), completeMsg.c_str(), completeMsg.length(), 0);

        completeMsg = ":" + _server->getServerName() + " NOTICE " + _receiver->getNickname() + " :Transfert terminé\r\n";
        send(_receiver->getSocket(), completeMsg.c_str(), completeMsg.length(), 0);

        // Fermer les sockets et nettoyer
        close(_listenSocket);
        close(_senderSocket);
        close(_receiverSocket);
        _listenSocket = -1;
        _senderSocket = -1;
        _receiverSocket = -1;

        _state = FINISHED;

        // Retirer le transfert du gestionnaire
        _server->getDCCManager().removeTransfer(this);
    }
    else if (_state == ERROR)
    {
        std::string errorMsg = ":" + _server->getServerName() + " NOTICE " + _sender->getNickname() + " :Erreur lors du transfert DCC\r\n";
        send(_sender->getSocket(), errorMsg.c_str(), errorMsg.length(), 0);

        errorMsg = ":" + _server->getServerName() + " NOTICE " + _receiver->getNickname() + " :Erreur lors du transfert DCC\r\n";
        send(_receiver->getSocket(), errorMsg.c_str(), errorMsg.length(), 0);

        // Fermer les sockets et nettoyer
        close(_listenSocket);
        close(_senderSocket);
        close(_receiverSocket);
        _listenSocket = -1;
        _senderSocket = -1;
        _receiverSocket = -1;

        _state = FINISHED;

        // Retirer le transfert du gestionnaire
        _server->getDCCManager().removeTransfer(this);
    }
}


/**
 * Vérifie si le transfert est complet
 */
bool DCCTransfer::isComplete() const
{
    return _state == COMPLETED || _state == FINISHED;
}

/**
 * Vérifie si une erreur s'est produite
 */
bool DCCTransfer::hasError() const
{
    return _state == ERROR;
}

/**
 * Retourne le port d'écoute
 */
uint16_t DCCTransfer::getListenPort() const
{
    return _listenPort;
}

/**
 * Ajoute les sockets au set
 */
void DCCTransfer::addSocketsToSet(fd_set& readSet, fd_set& writeSet, int& maxFd)
{
    if (_state == WAITING_FOR_CONNECTIONS)
    {
        FD_SET(_listenSocket, &readSet);
        if (_listenSocket > maxFd)
            maxFd = _listenSocket;
    }
    else if (_state == TRANSFERRING)
    {
        FD_SET(_senderSocket, &readSet);
        if (_senderSocket > maxFd)
            maxFd = _senderSocket;

        if (!_buffer.empty())
        {
            FD_SET(_receiverSocket, &writeSet);
            if (_receiverSocket > maxFd)
                maxFd = _receiverSocket;
        }
    }
}
