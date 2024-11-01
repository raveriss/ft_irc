
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "../incs/Server.hpp"

/* Déclaration de l'instance du serveur */
Server* serverInstance = NULL;

/* Gestionnaire de signaux */
void handleSignal(int signal) {
    const char* signalName;
    if (signal == SIGINT)
        signalName = "SIGINT (ctrl + c)";
    else if (signal == SIGTSTP)
        signalName = "SIGTSTP (ctrl + z)";
    else
        signalName = "Unknown";

    std::cout << "\nSignal " << signalName << " reçu, fermeture du serveur..." << std::endl;

    if (serverInstance) {
        serverInstance->shutdown();
        serverInstance = NULL;
    }
    exit(0);
}

/**
 * Main function
 */
int main(int argc, char **argv) {
    /* Vérification des arguments */
    if (argc != 3) {
        std::cerr << "Usage: ./micro_irc <port> <password>" << std::endl;
        return EXIT_FAILURE;
    }

    /* Conversion du port en entier */
    char *endptr;
    long port = std::strtol(argv[1], &endptr, 10);

    /* Vérification de la validité du port */
    if (*endptr != '\0' || port <= 0 || port > UINT16_MAX) {
        std::cerr << "Port invalide. Veuillez spécifier un port entre 1 et 65535." << std::endl;
        return EXIT_FAILURE;
    }

    /* Récupération du mot de passe */
    std::string password(argv[2]);

    /* Configuration des gestionnaires de signaux */
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTSTP, &sa, NULL) == -1) {
        std::cerr << "Erreur lors de la configuration des signaux." << std::endl;
        return EXIT_FAILURE;
    }

    /* Tente de créer et démarrer le serveur IRC avec le port et le mot de passe fournis */
    try {
        std::cout << "IRC, port \"" << port << "\", pass \"" << password << "\"." << std::endl << std::endl;
        Server server(static_cast<unsigned short>(port), password);
        serverInstance = &server;
        server.run();
    }

    /* Gestion des exceptions */
    catch (const std::exception &e) {
        std::cerr << "Erreur du serveur : " << e.what() << std::endl;
        if (serverInstance) {
            /* Ensuring memory release on error */
            delete serverInstance;
            serverInstance = NULL;
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
