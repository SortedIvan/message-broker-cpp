#include "server.hpp"

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

		sf::Packet connectionPacket; // has a specific structure, should not connect player if it doesn't match
		Message message;
		bool parseResult = parseMessage(connectionPacket, message);

		if (!parseResult) {
			return;
		}

		if (message.messageActionType != Connect) {
			continue;
		}

		//ConnectedClient connectedClient = connectClient(message);

		//std::string client_id = message.content["client_id"];

		//connectedClients.insert({client_id , std::move(connectedClient)});
		//std::cout << "Client connected" << std::endl;
		//std::thread(&Server::serverClientThread, this, client_id).detach();
	}
}

void Server::manageNonEmptyTopic(std::string topicId) {
	while (!topicMap[topicId].messages.empty()) {
		//auto message = topicMap[topicId].messages.front();
		//topicMap[topicId].messages.pop();

		//if (message.content.empty()) {
		//	continue;
		//}

		// process message here, send to all connected clients
		for (auto client : connectedClients) {
			if (client.second.subscriberTo.find(topicId) != client.second.subscriberTo.end()) {
				sf::Packet subscriberMessage;
				//subscriberMessage << message.content;

				//if (client.second.clientSocket->send(subscriberMessage) != sf::Socket::Done) {
				//	// delete client here
				//	std::cout << "Error sending message to client" << std::endl;
				//}
			}
		}
	}

	topicsBeingProcessed.erase(topicId);
}

// TODO: Make this utilize a thread pool
void Server::messageProcessing() {
	while (serverIsRunning) {
		for (auto& topic : topicMap) {

			if (!topic.second.messages.empty() &&
				topicsBeingProcessed.find(topic.first) == topicsBeingProcessed.end())
			{
				topicsBeingProcessed.insert(topic.first);
				std::thread(&Server::manageNonEmptyTopic, this, topic.first).detach();
			}
		}
	}
}


// The structure of any message should be
// {<message_action_id>:<message_content>:<additional_header_1>:<additional_header_2>:...:<additional_header_n>}
bool Server::parseMessage(sf::Packet& packet, Message& message) {
	std::string data;
	int dataPointer = 0;
	MessageActionType actionType = None;

	if (!(packet >> data)) return false;
	if (data.size() == 0) return false;

	Message serializedMessage;

	try {
		serializedMessage = messageSerializer.deserialize(packet);
		return true;
	}
	catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}
	return false;
}

/*
	Creates the structure of the json based on the message type
*/
bool Server::processMessageContent(nlohmann::json& jsonContent, MessageActionType actionType,std::string messageContent) {
	bool result = false;

	switch (actionType) {
		case None:
			return false;
		case Connect:
			//result = parseConnectMessage(jsonContent, messageContent);
			break;
		case Disconnect: // Disconnecting has no content attached to it
			return true;
			break;
		case SimpleMessage:
			//result = parseSimpleMessage(jsonContent, messageContent);
			break;
	}

	return result;
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

/*
	@dev represents a thread function that is responsible for managing receiving messages from each client
	TODO: rework to a more extendable system
*/
void Server::serverClientThread(std::string clientId) {
	while (serverIsRunning) {
		auto client = connectedClients[clientId];

		sf::Packet packet;
		
		if (client.clientSocket->receive(packet) != sf::Socket::Done)
		{
			std::cout << "Error with message receiving from client" << std::endl;

			break;
		}
		
		Message message;
		parseMessage(packet, message);
		
		// process message here


	}
}

void Server::processMessage(Message& message) {
	switch (message.messageActionType) {
		case None:
			return;
		case Connect:
			connectClient(message);
	}
}

// 
ConnectedClient Server::connectClient(Message& message) {
	std::unordered_set<std::string> subscriberSet;
	std::unordered_set<std::string> publisherSet;

	ConnectedClient connectedClient;

	return connectedClient;
}
