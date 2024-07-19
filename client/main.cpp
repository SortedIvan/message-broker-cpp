#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include <iostream>

int main() {
	sf::TcpSocket socket;

	sf::IpAddress ip("127.0.0.1");
	unsigned short port = 54000;

	sf::Socket::Status status = socket.connect(ip, port);
	if (status != sf::Socket::Done)
	{
		std::cout << "ASDASD" << std::endl;

		// error...
	}

	return 0;
}