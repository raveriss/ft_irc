/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DCCManager.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 23:06:52 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/24 23:07:02 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Inclusions pour les transferts DCC */
#include "../incs/DCCManager.hpp"
#include "../incs/DCCTransfer.hpp"

/**
 * Constructeur
 */
DCCManager::DCCManager()
{}

/**
 * Destructeur
 */
DCCManager::~DCCManager()
{
    /* Clean up remaining transfers */
    for (std::vector<DCCTransfer*>::iterator it = _transfers.begin(); it != _transfers.end(); ++it)
    {
        delete *it;
    }
    _transfers.clear();
}

/**
 * Ajoute un transfert à la liste
 */
void DCCManager::addTransfer(DCCTransfer* transfer)
{
    _transfers.push_back(transfer);
}

/**
 * Retire un transfert de la liste
 */
void DCCManager::removeTransfer(DCCTransfer* transfer)
{
    _transfers.erase(std::remove(_transfers.begin(), _transfers.end(), transfer), _transfers.end());
    delete transfer;
}

/**
 * Traite les transferts
 */
void DCCManager::processTransfers(fd_set& readSet, fd_set& writeSet)
{
    for (std::vector<DCCTransfer*>::iterator it = _transfers.begin(); it != _transfers.end();)
    {
        DCCTransfer* transfer = *it;
        transfer->process(readSet, writeSet);

        if (transfer->isComplete() || transfer->hasError())
        {
            it = _transfers.erase(it);
            delete transfer;
        }
        else
        {
            ++it;
        }
    }
}

/**
 * Ajoute les sockets des transferts aux ensembles
 */
void DCCManager::addSocketsToSet(fd_set& readSet, fd_set& writeSet, int& maxFd)
{
    for (size_t i = 0; i < _transfers.size(); ++i)
    {
        _transfers[i]->addSocketsToSet(readSet, writeSet, maxFd);
    }
}
