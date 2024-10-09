
# ft_irc - École 42 | Paris

<div align="center">
  <img src="https://img.shields.io/badge/language-C++98-blue" alt="C++98">
  <img src="https://img.shields.io/badge/standard-IRC-1f425f" alt="IRC Protocol">
  <img src="https://img.shields.io/badge/socket-TCP/IP-blue" alt="TCP/IP">
  <img src="https://img.shields.io/badge/school-42-green" alt="42">
  <img src="https://img.shields.io/badge/42-Paris-blue" alt="42 Paris">
</div>

###
<div align="center">
  <img src="https://raw.githubusercontent.com/ayogun/42-project-badges/refs/heads/main/badges/ft_irce.png?raw=true" alt="Badge du projet push_swap">
</div>

## Description

Le projet **ft_irc** consiste à développer un serveur IRC en respectant le standard IRC tout en utilisant le langage **C++98**. Le serveur doit gérer plusieurs connexions simultanées et fonctionner en mode non bloquant pour permettre une communication fluide entre les clients.

Vous devez tester votre serveur à l'aide d'un vrai client IRC pour garantir la compatibilité avec le protocole standard.

## Objectifs

- Reproduire le fonctionnement d'un serveur **IRC** en C++98.
- Gérer plusieurs clients simultanément sans blocage.
- Implémenter les principales commandes IRC comme `NICK`, `USER`, `JOIN`, `PRIVMSG`, etc.
- Fournir un système d'authentification via un mot de passe à la connexion.
- Supporter les opérateurs de canal et les commandes associées comme `KICK`, `INVITE`, `TOPIC`, et `MODE`.

## Fonctionnalités

- **Communication TCP/IP** : Utilisation des sockets pour la communication entre le serveur et les clients via le protocole **TCP/IP** (v4 ou v6).
- **Gestion des canaux** : Le serveur doit permettre la création et la gestion de canaux IRC, avec la possibilité d'envoyer des messages privés ou dans des canaux.
- **Non-bloquant** : Toutes les opérations d'entrée/sortie (E/S) sont non bloquantes afin de ne jamais empêcher la gestion simultanée des connexions multiples.
- **Sécurité** : Utilisation d'un mot de passe pour sécuriser l'accès au serveur. Chaque client doit fournir le bon mot de passe pour pouvoir se connecter.
- **Support multi-clients** : Le serveur peut gérer plusieurs connexions simultanément sans aucun forking ou thread supplémentaire.

## Structure du Projet

- **Fichiers rendus** : 
  - `Makefile`
  - Fichiers sources C++ (.cpp, .hpp)
  - Un fichier de configuration optionnel
- **Arguments** :
  - `<port>` : Le numéro de port sur lequel le serveur écoutera.
  - `<password>` : Le mot de passe que les clients devront fournir pour se connecter.

Exemple de commande pour lancer le serveur :
```bash
./ircserv <port> <password>
```

## Compilation

Utilisez le **Makefile** pour compiler le projet avec les options de compilation requises. Le projet doit se compiler en utilisant C++98 avec les flags `-Wall -Wextra -Werror`.

Commandes disponibles :
```bash
make        # Compile le projet
make clean  # Supprime les fichiers objets
make fclean # Supprime les fichiers objets et les binaires
make re     # Recompile le projet
```

## Tests

Utilisez un client IRC tel que **nc** (netcat) ou un client IRC de référence pour tester la connectivité et les fonctionnalités du serveur.

Exemple de connexion avec **nc** :
```bash
nc 127.0.0.1 <port>
```

### Exemple de commandes IRC supportées

- **NICK** : Changer le pseudonyme du client
- **USER** : S'authentifier sur le serveur
- **JOIN** : Rejoindre un canal
- **PRIVMSG** : Envoyer un message privé ou un message dans un canal
- **KICK** : Éjecter un client d'un canal (opérateurs uniquement)
- **INVITE** : Inviter un client à rejoindre un canal
- **TOPIC** : Changer ou afficher le sujet d'un canal
- **MODE** : Modifier les permissions d'un canal (opérateurs uniquement)

## Bonus

- **Bot IRC** : Possibilité d'ajouter un petit bot qui interagit avec les utilisateurs.
- **Transfert de fichiers** : Support de l'envoi de fichiers entre clients.

## Contributeurs

- raveriss - Développeur principal

## Ressources Utilisées

- [Documentation officielle de l'IRC](https://tools.ietf.org/html/rfc2812)
- [Documentation des sockets en C++](https://www.boost.org/doc/libs/1_76_0/doc/html/boost_asio.html)
- [Tutoriel sur les sockets Unix](https://beej.us/guide/bgnet/)
