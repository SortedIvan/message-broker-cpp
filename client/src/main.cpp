#include <iostream>
#include <thread>
#include <unordered_set>
#include "SFML/Network.hpp"
#include "message/message.hpp"
#include "message/message_serializer.hpp"
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
void printMessage(const Message& message);
bool processMessage(Message& message);


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
		std::cout << "4) - Disconnect" << std::endl;

		std::cin >> command;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline character
		processCommand(command, client, isConnected);
	}

	return EXIT_SUCCESS;
}

void signalDisconnect(Client& client) {
	sf::Packet packet;

}

/*
	Sends a simple message into the message queue
*/
void sendMessage(Client& client, bool& isConnected) {
	sf::Packet messagePacket;
	std::string data = "";
	std::cout << "Input message data: ";
	std::getline(std::cin, data); // Read full line including spaces
	std::cout << std::endl;

	if (data.empty()) {
		std::cout << "No message entered." << std::endl;
		return;
	}

	Message message;
	message.sender = client.clientId;
	message.messageActionType = SimpleMessage;
	message.actionData = data;

	std::string serializedMessage = serialize<Message>(message);

	messagePacket << serializedMessage;
	
	if (client.clientSocket->send(messagePacket) != sf::Socket::Done) {
		std::cerr << "Problem with appending message to queue, breaking connection" << std::endl;
		isConnected = false;
		return;
	}

	std::cout << "Message appended to queue successfully" << std::endl;
	client.messageSentHistory.push_back(message);
}

void showAllReceived(Client& client) {
	for (int i = 0; i < client.messageReceivedHistory.size(); ++i) {
		printMessage(client.messageReceivedHistory[i]);
	}
}

void printMessage(const Message& message) {
	std::cout << "---------- Message ------------" << std::endl;
	std::cout << "Sender: " << message.sender << std::endl;
	std::cout << "Message data" << message.actionData << std::endl;
}

void showAllSent(Client& client) {
	for (int i = 0; i < client.messageSentHistory.size(); ++i) {
		printMessage(client.messageReceivedHistory[i]);
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
		if (!(incomingMessage >> data)) {
			std::cerr << "Error with parsing incoming message data" << std::endl;
			continue;
		}

		if (data.empty()) {
			std::cerr << "Incoming message data is empty" << std::endl;
			continue;
		}

		Message message;

		try {
			message = deserialize<Message>(data);
			processMessage(message);
		}
		catch (const std::exception& ex) {
			std::cerr << ex.what() << std::endl;
		}
	}
}

bool processMessage(Message& message) {
	switch (message.messageActionType) {
		case None:
			return false;
		case SimpleMessage:
			std::cout << "Client: " << message.sender << " has sent {" << message.actionData << "}" << std::endl;
			return true;
		case Connect:
			return true;
		case Disconnect:
			return true;
	}
}

bool establishConnection(const Client& client) {
	ConnectionData connectionData;
	connectionData.publisherTo = std::vector<std::string>(client.publisherTo.begin(), client.publisherTo.end());
	connectionData.subscriberTo = std::vector<std::string>(client.subscriberTo.begin(), client.subscriberTo.end());
	std::string serializedData = serialize<ConnectionData>(connectionData);

	if (serializedData.empty()) {
		return false;
	}

	Message connectionMessage;
	connectionMessage.sender = client.clientId;
	connectionMessage.messageActionType =(int)Connect;
	connectionMessage.actionData = serializedData;

	std::string serializedMessage = serialize<Message>(connectionMessage);

	sf::Packet connectionPacket;
	connectionPacket << serializedMessage;

	if (client.clientSocket->send(connectionPacket) != sf::Socket::Done) {
		std::cerr << "There was an error connecting the client. Please try again" << std::endl;
		return false;
	}

	std::cout << serializedMessage << std::endl;

	return true;
}

void initializePublisherSubscriberArrays(Client& client) {
	std::unordered_set<std::string> publisherTo;
	std::unordered_set<std::string> subscriberTo;

	while (true) {
		std::string publishTo = "";
		std::cout << "To cancel, enter *" << std::endl;
		std::cout << "Please enter topic to publish to: ";
		std::cin >> publishTo;

		if (publishTo.find('*') != std::string::npos) {
			break;
		}
		else {
			if (publisherTo.find(publishTo) != publisherTo.end()) {
				std::cout << "Topic already added!" << std::endl;
			}
			else {
				publisherTo.insert(publishTo);
			}
		}
	}

	while (true) {
		std::string subscribeTo = "";
		std::cout << "To cancel, enter *" << std::endl;
		std::cout << "Please enter topic to subscribe to to: ";
		std::cin >> subscribeTo;

		if (subscribeTo.find('*') != std::string::npos) {
			break;
		}
		else {
			if (subscriberTo.find(subscribeTo) != subscriberTo.end()) {
				std::cout << "Topic already added!" << std::endl;
			}
			else {
				subscriberTo.insert(subscribeTo);
			}
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