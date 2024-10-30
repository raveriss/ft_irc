/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DCCTransfer.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:26:05 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/24 23:26:06 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DCCTRANSFER_HPP
#define DCCTRANSFER_HPP

/* For uint32_t */
typedef unsigned int uint32_t;

/* For uint16_t */
typedef unsigned short uint16_t;

/* For std::vector  */
#include <vector>

/* For fcntl() */
#include <fcntl.h>

/* For std::cout */
#include <iostream>

/* For errno, EWOULDBLOCK */
#include <errno.h>

/* For fd_set */
#include <algorithm>

/* Déclaration anticipée de Server */
class Server;

/* Déclaration anticipée de Client */
class Client;

/**
 * class DCCTransfer
 */
class DCCTransfer
{
    public:

        /* Constructeur */
        DCCTransfer(Server* server, Client* sender, Client* receiver, const std::string& filename, uint32_t filesize);

        /* Destructeur */
        ~DCCTransfer();


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                GESTION DU TRANSFERT                       */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        bool start();
        void process(fd_set& readSet, fd_set& writeSet);
        bool isComplete() const;
        bool hasError() const;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                GESTION DES SOCKETS                        */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Retourne le port d'écoute */
        uint16_t getListenPort() const;

        /* Ajoute les sockets à l'ensemble */
        void addSocketsToSet(fd_set& readSet, fd_set& writeSet, int& maxFd);

    private:

        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                               DONNÉES INTERNES                            */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Pointeur vers le serveur */
        Server* _server;

        /* Pointeur vers le client émetteur */
        Client* _sender;

        /* Pointeur vers le client récepteur */
        Client* _receiver;

        /* Nom du fichier à transférer */
        std::string _filename;

        /* Taille du fichier à transférer */
        uint32_t _filesize;

        /* Socket d'écoute */
        int _listenSocket;

        /* Socket du client émetteur */
        int _senderSocket;

        /* Socket du client récepteur */
        int _receiverSocket;

        /* Port d'écoute */
        uint16_t _listenPort;

        /* Nombre d'octets transférés */
        uint32_t _bytesTransferred;

        /* Tampon de données */
        std::vector<char> _buffer;


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                                ÉTAT DU TRANSFERT                          */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* États du transfert */
        enum State
        {
            WAITING_FOR_CONNECTIONS,
            TRANSFERRING,
            COMPLETED,
            ERROR,
            FINISHED
        } _state;
};

#endif /* DCCTRANSFER_HPP */
