#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "server/server.hpp"
#include <iostream>

int main() {
    Server server("127.0.0.1", 54000);
    server.serverLoop();

    //sf::TcpListener listener;
    //sf::IpAddress ip("127.0.0.1");
    //unsigned short port = 54000;


    //std::cout << "Server is listening on " << ip.toString() << ":" << port << std::endl;



    //std::cout << "New client connected: " << client.getRemoteAddress().toString() << std::endl;

	return 0;
}