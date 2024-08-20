#include <iostream>
#include <thread>
#include <unordered_set>
#include <chrono>
#include "SFML/Network.hpp"
#include "message/message.hpp"
#include "message/message_serializer.hpp"
#include "client/client.hpp"

void initializePublisherSubscriberArrays(Client& client);
void receiver(Client& client);
void processCommand(const std::string& command, Client& client);

int main() {

	std::string newClientId = "";
	std::cout << "Enter client id: ";
	std::cin >> newClientId;
	std::cout << std::endl;

	Client client(newClientId, "127.0.0.1", 54000);
	if (!client.tryConnect()) {
		std::cerr << "Error connecting client" << std::endl;
		return EXIT_FAILURE;
	}

	initializePublisherSubscriberArrays(client);
	
	bool isConnected = client.tryEstablishClientIdentity();

	if (!isConnected) {
		return EXIT_FAILURE;
	}

	auto clientRef = std::ref(client);

	std::thread receiverThread(receiver, clientRef);
	// to do: make the commands be a seperate thread

	while (client.clientIsConnected) {
		std::string command = "";
		std::cout << "Please choose an option: " << std::endl;
		std::cout << "1) - Send a message" << std::endl; 
		std::cout << "2) - View all sent" << std::endl;
		std::cout << "3) - View all received" << std::endl;
		std::cout << "4) - Disconnect" << std::endl;

		std::cin >> command;
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline character
		processCommand(command, client);
	}

	receiverThread.join();
	return EXIT_SUCCESS;
}

void processCommand(const std::string& command, Client& client) {
	try {
		if (command.size() != 1
			|| std::stoi(command) > 9 || std::stoi(command) < 1) {
			std::cout << "Command incorrect" << std::endl;
			return;
		}

		switch (std::stoi(command)) {
			case 1:
				client.sendMessage();
				break;
			case 2:
				client.showAllSent();
				break;
			case 3:
				client.showAllReceived();
				break;
			case 4: 
				bool disconnectAttempt = client.signalDisconnect();
				if (disconnectAttempt) {}
				return;
		}
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
}

void receiver(Client& client) {
	while (client.clientIsConnected) {
		sf::Packet incomingMessage;
		sf::Socket::Status status = client.clientSocket->receive(incomingMessage);

		if (status == sf::Socket::Done) {
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
				client.processMessage(message);
			}
			catch (const std::exception& ex) {
				std::cerr << ex.what() << std::endl;
			}		
		}
		else if (status == sf::Socket::NotReady) {
			// Do something with not ready
		}
		else if (status == sf::Socket::Disconnected) {
			std::cout << "Disconnected from the server." << std::endl;
			client.clientIsConnected = false;
			break;
		}
		else {
			std::cerr << "Error occurred during receiving." << std::endl;
		}
	}
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
