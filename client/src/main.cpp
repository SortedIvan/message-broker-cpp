#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include <iostream>
#include <thread>
#include <Windows.h>


int main() {
	sf::TcpSocket socket;

	sf::IpAddress ip("127.0.0.1");
	unsigned short port = 54000;

	sf::Socket::Status status = socket.connect(ip, port);
	if (status != sf::Socket::Done)
	{
		std::cout << "ASDASD" << std::endl;
	}

	sf::Packet connectionMessage;
	sf::Packet testMessage;
	
	std::string connectionContent = "1234|topic1|topic1";
	std::string testSendMessage = "1234|topic1|Hello, world!";
	std::string clientConnectionMessage;

	connectionMessage << connectionContent;

	std::cout << connectionMessage.getDataSize();

	if (socket.send(connectionMessage) != sf::Socket::Done)
	{
		// error...
	}

	// clientid|toTopic1:toTopic2...|content

	testMessage << testSendMessage;

	if (socket.send(testMessage) != sf::Socket::Done) 
	{
		
	}
	

	sf::Packet messageReceived;

	if (socket.receive(testMessage) != sf::Socket::Done)
	{

	}

	std::string data;
	messageReceived >> data;
	std::cout << data << std::endl;
	return 0;
}