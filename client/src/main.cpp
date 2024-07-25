#include <iostream>
#include <thread>
#include "SFML/Network.hpp"
#include "client/client.hpp"

#define CLIENT_ID_SIZE 4

Client initializeClient();
void initializePublisherSubscriberArrays(Client& client);
void receiver(Client& client, bool& isConnected);
void processCommand(const std::string& command, Client& client, bool& isConnected);
void sendMessage(Client& client, bool& isConnected);
void showAllReceived(Client& client);
void showAllSent(Client& client);
bool establishConnection(const Client& client);

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

		std::cin >> command;

		processCommand(command, client, isConnected);
	}

	return EXIT_SUCCESS;
}

void sendMessage(Client& client, bool& isConnected) {
	sf::Packet message;
	std::string data = "";
	std::cout << "Input message data: ";
	std::cin >> data;

	if (data.size() == 0) {
		return;
	}

	message << data;

	std::cout << "Message is being sent to queue" << std::endl;
	if (client.clientSocket->send(message) != sf::Socket::Done) {
		std::cerr << "Problem with appending message to queue, breaking connection" << std::endl;
		isConnected = false;
		return;
	}

	std::cout << "Message appended to queue succesfully" << std::endl;
	client.messageSentHistory.push_back(data);
}

void showAllReceived(Client& client) {
	for (int i = 0; i < client.messageReceivedHistory.size(); ++i) {
		std::cout << client.messageReceivedHistory[i] << std::endl;
	}
}

void showAllSent(Client& client) {
	for (int i = 0; i < client.messageSentHistory.size(); ++i) {
		std::cout << client.messageSentHistory[i] << std::endl;
	}
}

void processCommand(const std::string& command, Client& client, bool& isConnected) {
	try {
		if (command.size() != 1
			|| std::stoi(command) > 9 || std::stoi(command) < 1) {
			std::cout << "Command incorrect" << std::endl;
			return;
		}

		switch (std::stoi(command)) {
			case 1:
				sendMessage(client, isConnected);
				break;
			case 2:
				showAllSent(client);
				break;
			case 3:
				showAllReceived(client);
				break;
		}
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
}

void receiver(Client& client, bool& isConnected) {
	while (isConnected) {
		sf::Packet incomingMessage;
		if (client.clientSocket->receive(incomingMessage) != sf::Socket::Done) {
			std::cerr << "Issue with receiving a message, terminating client connection" << std::endl;
			isConnected = false;
			break;
		}

		std::string data;
		incomingMessage >> data;
		client.messageReceivedHistory.push_back(data);
		std::cout << "Message received: " << data << std::endl;
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
		else {
			clientInitialized = true;
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