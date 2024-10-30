/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DCCManager.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:25:59 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/24 23:26:00 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DCCMANAGER_HPP
#define DCCMANAGER_HPP

/* For std::vector */
#include <vector>

/* For fd_set */
#include <sys/select.h>

/* For std::remove */
#include <algorithm>

/* For fd_set */
#include <sys/select.h>

/* Déclaration anticipée de DCCTransfer */
class DCCTransfer;

/**
 * class DCCManager
 */
class DCCManager
{
    public:

        /* Constructeur */
        DCCManager();

        /* Destructeur */
        ~DCCManager();


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                              GESTION DES TRANSFERTS                       */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute un transfert à la liste */
        void addTransfer(DCCTransfer* transfer);

        /* Retire un transfert de la liste */
        void removeTransfer(DCCTransfer* transfer);

        /* Traite les transferts */
        void processTransfers(fd_set& readSet, fd_set& writeSet);


        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                            GESTION DES SOCKETS                            */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Ajoute les sockets à l'ensemble */
        void addSocketsToSet(fd_set& readSet, fd_set& writeSet, int& maxFd);

    private:

        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
        /*                           DONNÉES INTERNES DES TRANSFERTS                 */
        /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

        /* Liste des transferts */
        std::vector<DCCTransfer*> _transfers;
};

#endif /* DCCMANAGER_HPP */
