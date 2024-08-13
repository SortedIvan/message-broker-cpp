#include "server.hpp"
#include "../src/message/message_serializer.hpp"

Server::Server(std::string _ip, unsigned int _port) {
	ip = _ip;
	port = _port;
	serverIsRunning = false;
}

void Server::serverLoop() {
	serverIsRunning = true;
	
	if (listener.listen(port, ip) != sf::Socket::Done) {
		std::cerr << "Listener was not configured properly" << std::endl;
		return;
	}

	while (serverIsRunning) {
		auto client = std::make_shared<sf::TcpSocket>();

		if (listener.accept(*client) != sf::Socket::Done) {
			std::cerr << "Error: Could not accept new connection" << std::endl;
			continue;
		}

		std::thread(&Server::serverClientThread, this, std::move(client)).detach();
	}
}

/*
	@dev represents a thread function that is responsible for managing receiving messages from each client
	TODO: add sending a message back to client to let them know if they connected succesfully
*/
void Server::serverClientThread(std::shared_ptr<sf::TcpSocket> clientSocket) {

	std::cout << "Client is connecting" << std::endl;

	// first, attempt to connect with the client before adding him to the connected clients
	sf::Packet connectionPacket;

	if (clientSocket->receive(connectionPacket) != sf::Socket::Done) {
		std::cerr << "Error parsing the connection packet for the connecting client" << std::endl;
	}

	Message connectionMessage;
	bool connectionParseResult = parsePacketIntoMessage(connectionPacket, connectionMessage);

	if (!connectionParseResult) {
		std::cerr << "Error parsing client connection message" << std::endl;
		return;
	}

	if (connectionMessage.messageActionType != Connect) {
		std::cerr << "Message not correct type" << std::endl;
		return;
	}


	ConnectedClient connectedClient;
	bool clientConnectionResult = connectClient(connectionMessage, connectedClient);

	if (!clientConnectionResult) {
		std::cerr << "Issue with connecting client" << std::endl;
		return;
	}

	std::string clientId = connectionMessage.sender;
	
	if (clientId.size() != CLIENT_ID_SIZE) {
		std::cerr << "Incorrect client id size" << std::endl;
		return;
	}
	
	connectedClient.clientId = clientId;
	connectedClient.clientSocket = std::move(clientSocket);
	connectedClients.insert({ clientId, std::move(connectedClient)});

	while (serverIsRunning) {
		sf::Packet packet;

		connectedClient = connectedClients[clientId];

		if (connectedClient.clientSocket->receive(packet) != sf::Socket::Done)
		{
			std::cerr << "Error with message receiving from client" << std::endl;
			connectedClients.erase(clientId);
			break;
		}

		Message message;
		bool messageParseResult = parsePacketIntoMessage(packet, message);

		if (!messageParseResult) {
			std::cerr << "Client message could not be parsed correctly" << std::endl;
			connectedClients.erase(clientId);
			return;
		}

		/*
			@dev process functionality
			Manage any needed actions embedded in the message
			& handle pushing messages into the appropriate topic
		*/
		bool processingResult = processMessage(message);

		if (!processingResult) {
			std::cerr << "There was an error processing a message" << std::endl;
			connectedClients.erase(clientId);
			return;
		}

	}

	connectedClients.erase(clientId);
}

bool Server::processMessage(Message& message) {
	bool processResult = false;

	switch (message.messageActionType) {
		case None:
			return false;
		case Connect:
			return false;
		case SimpleMessage:
			processResult = processSimpleMessage(message);
			break;
	}

	return processResult;
}

/*
	@dev sample function for a client action
	Simple message comes in the form of a string, does not need any further processing, just gets sent into the topic
*/
bool Server::processSimpleMessage(Message& message) {

	for (auto connectedClient : connectedClients) {
		std::cout << connectedClient.second.publisherTo.size() << std::endl;
	}

	for (std::string topic : connectedClients[message.sender].publisherTo) {
		if (topicMap.find(topic) == topicMap.end()) {
			// topic got deleted externally/by another thread, continue;
			continue;
		}
		topicMap[topic].messages.push(message);
	}

	return true;
}

bool Server::connectClient(Message& message, ConnectedClient& client) {
	try {
		ConnectionData data = deserialize<ConnectionData>(message.actionData);

		if (data.publisherTo.empty() && data.subscriberTo.empty()) {
			return false;
		}

		client.subscriberTo = std::unordered_set<std::string>(data.subscriberTo.begin(), data.subscriberTo.end());
		client.publisherTo = std::unordered_set<std::string>(data.publisherTo.begin(), data.publisherTo.end());
		return true;
	}
	catch (std::exception& ex) {
		return false;
	}
	return false;
}


void Server::processMessagesFromNonEmptyTopic(std::string topicId) {
	while (!topicMap[topicId].messages.empty()) {
		auto message = topicMap[topicId].messages.front();
		topicMap[topicId].messages.pop();

		// process message here, send to all connected clients
		for (auto client : connectedClients) {
			if (client.second.subscriberTo.find(topicId) != client.second.subscriberTo.end()) {
				sf::Packet subscriberMessage;
				std::string messageData = EMPTY_STR;
				// serialize message into a string

				try {
					messageData = serialize<Message>(message);
					subscriberMessage << messageData;

					if (client.second.clientSocket->send(subscriberMessage) != sf::Socket::Done) {
						// delete client here
						std::cout << "Error sending message to client" << std::endl;
					}
				}
				catch (const std::exception& ex) {
					std::cerr << ex.what() << std::endl;
					// optionally, remove the client that sent the broken message
				}
			}
		}
	}

	topicsBeingProcessed.erase(topicId);
}

/*
	@dev uses a task thread to process a non empty topic
	@todo make sure this uses a thread pool | make it an event based system, not constant looping
*/ 
void Server::processNonEmptyTopicThreadCreator() {
	while (serverIsRunning) {
		for (auto& topic : topicMap) {

			if (!topic.second.messages.empty() &&
				topicsBeingProcessed.find(topic.first) == topicsBeingProcessed.end())
			{
				topicsBeingProcessed.insert(topic.first);
				std::thread(&Server::processMessagesFromNonEmptyTopic, this, topic.first).detach();
			}
		}
	}
}


bool Server::parsePacketIntoMessage(sf::Packet& packet, Message& message) {
	std::string data;
	int dataPointer = 0;
	MessageActionType actionType = None;

	if (!(packet >> data)) return false;
	if (data.size() == 0) return false;

	Message serializedMessage;

	try {
		serializedMessage = deserialize<Message>(data);
		message = serializedMessage;
		return true;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}
	return false;
}

// TODO: Add processing of additional headers
Header Server::processHeader(std::string headerContent) {
	Header header;
	header.content = headerContent;
	return header;
}

void Server::createTopic(std::string topicId, int maxAllowedConnections) {
	Topic topic;
	topic.maxAllowedConnections = maxAllowedConnections;
	topic.topicId = topicId;
	topicMap.insert({ topicId, topic });
}


