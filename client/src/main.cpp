#include <iostream>
#include <thread>
#include "SFML/Network.hpp"
#include "client/client.hpp"

#define CLIENT_ID_SIZE 4

Client initializeClient();
void initializePublisherSubscriberArrays(Client& client);
void receiver(const Client& client, bool& isConnected);
void processCommand(const std::string& command);
void sendMessage();
void showAllReceived();
void showAllSent();

int main() {
	Client client;
	client = initializeClient();
	initializePublisherSubscriberArrays(client);
	bool isConnected = establishConnection(client);
	
	if (!isConnected) {
		return EXIT_FAILURE;
	}

	auto clientRef = std::ref(client);

	std::thread(receiver, clientRef, std::ref(isConnected)).detach();

	while (isConnected) {
		std::string command = "";
		std::cout << "Please choose an option: " << std::endl;
		std::cout << "1) - Send a message" << std::endl; 
		std::cout << "2) - View all sent" << std::endl;
		std::cout << "3) - View all received" << std::endl;

		processCommand(command);
	}

	return EXIT_SUCCESS;
}

void sendMessage() {

}

void showAllReceived() {

}

void showAllSent() {

}

void processCommand(const std::string& command) {
	try {
		if (command.size() != 1
			|| std::stoi(command) > 9 || std::stoi(command) < 1) {
			std::cout << "Command incorrect" << std::endl;
			return;
		}

		switch (std::stoi(command)) {
			case 1:
				sendMessage();
				break;
			case 2:
				showAllSent();
				break;
			case 3:
				showAllReceived();
				break;
		}
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
}

void receiver(const Client& client, bool& isConnected) {
	while (isConnected) {

	}
}

bool establishConnection(const Client& client) {
	std::string connectionMessage = client.clientId + "|";

	for (int i = 0; i < client.publisherTo.size(); ++i) {
		connectionMessage += client.publisherTo[i];

		if (i != client.publisherTo.size() - 1) {
			connectionMessage += ":";
		}
	}

	connectionMessage += "|";

	for (int i = 0; i < client.subscriberTo.size(); ++i) {
		connectionMessage += client.subscriberTo[i];

		if (i != client.subscriberTo.size() - 1) {
			connectionMessage += ":";
		}
	}

	sf::Packet connectionPacket;
	connectionPacket << connectionMessage;

	if (client.clientSocket->send(connectionPacket) != sf::Socket::Done) {
		std::cerr << "There was an error connecting the client. Please try again" << std::endl;
		return false;
	}

	return true;
}

void initializePublisherSubscriberArrays(Client& client) {

	std::vector<std::string> publisherTo;
	std::vector<std::string> subscriberTo;

	while (true) {
		std::string publishTo = "";
		std::cout << "To cancel, enter *" << std::endl;
		std::cout << "Please enter topic to publish to: ";
		std::cin >> publishTo;

		if (publishTo.find("*")) {
			break;
		}
		else {
			publisherTo.push_back(publishTo);
		}
	}

	while (true) {
		std::string subscribeTo = "";
		std::cout << "To cancel, enter *" << std::endl;
		std::cout << "Please enter topic to subscribe to to: ";
		std::cin >> subscribeTo;

		if (subscribeTo.find("*")) {
			break;
		}
		else {
			subscriberTo.push_back(subscribeTo);
		}
	}

	client.publisherTo = publisherTo;
	client.subscriberTo = subscriberTo;
}


Client initializeClient() {

	bool clientInitialized = false;
	bool clientIdCorrect = false;
	bool clientIpCorrect = false;

	Client client;
	std::string clientIp = "";
	std::string clientId = "";

	while (!clientInitialized) {

		if (!clientIdCorrect) {
			std::cout << "Enter client id: ";
			std::cin >> clientId;

			if (clientId.size() != CLIENT_ID_SIZE) {
				std::cout << "Wrong size client id, please enter the correct size: " << CLIENT_ID_SIZE << std::endl;
				clientId = "";
				clientIdCorrect = false;
				continue;
			}

			clientIdCorrect = true;
		}

		if (!clientIpCorrect) {
			std::cout << "Enter client ip: ";
			std::cin >> clientIp;
			clientIpCorrect = true;
		}
	}

	auto socket = std::make_shared<sf::TcpSocket>();
	sf::IpAddress ip(clientIp);
	unsigned short port = 54000;

	sf::Socket::Status status = socket->connect(ip, port);
	if (status != sf::Socket::Done)
	{
		std::cerr << "Error connecting client" << std::endl;
		return client;
	}

	client.clientInitialized = true;
	client.clientId = clientId;
	client.clientSocket = socket;

	return client;
}