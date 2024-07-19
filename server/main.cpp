#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include <iostream>

int main() {
    sf::TcpListener listener;

    sf::IpAddress ip("127.0.0.1");
    unsigned short port = 54000;

    // Bind the listener to the specified IP address and port
    if (listener.listen(port, ip) != sf::Socket::Done) {
        std::cerr << "Error: Could not bind to " << ip.toString() << ":" << port << std::endl;
        return 1;
    }

    std::cout << "Server is listening on " << ip.toString() << ":" << port << std::endl;

    // Accepting new connections
    sf::TcpSocket client;
    if (listener.accept(client) != sf::Socket::Done) {
        std::cerr << "Error: Could not accept new connection" << std::endl;
        return 1;
    }

    std::cout << "New client connected: " << client.getRemoteAddress().toString() << std::endl;

	return 0;
}