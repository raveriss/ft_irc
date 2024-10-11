/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: raveriss <raveriss@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/10 12:36:38 by raveriss          #+#    #+#             */
/*   Updated: 2024/10/11 16:07:00 by raveriss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include <iostream>

/* For std::atoi */
#include <cstdlib>
#include "server.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return EXIT_FAILURE;
    }

    Server ircServer(argv[1], argv[2]);
    ircServer.run();

    return EXIT_SUCCESS;
}


/* main.cpp */