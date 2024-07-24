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

		// check the connection message from the client
		if (client->receive(connectionPacket) != sf::Socket::Done)
		{
			std::cerr << "Error: Could not processes connection packet from client" << std::endl;
			continue;
		}

		ConnectionMessage connectionMessage = parseConnectionPacket(connectionPacket);

		if (!connectionMessage.isCorrect) {
			continue;
		}

		std::unordered_set<std::string> subscriberSet;
		std::unordered_set<std::string> publisherSet;

		// if the topic ids don't relate to any topics, don't add client
		for (int i = 0; i < connectionMessage.publisherTo.size(); ++i) {
			if (topicMap.find(connectionMessage.publisherTo[i]) == topicMap.end()) {
				continue;
			}

			publisherSet.insert(connectionMessage.publisherTo[i]);
		}

		for (int i = 0; i < connectionMessage.subscriberTo.size(); ++i) {
			if (topicMap.find(connectionMessage.subscriberTo[i]) == topicMap.end()) {
				continue;
			}

			subscriberSet.insert(connectionMessage.subscriberTo[i]);
		}

		ConnectedClient connectedClient;
		connectedClient.clientSocket = client;
		connectedClient.subscriberTo = subscriberSet;
		connectedClient.publisherTo = publisherSet;
		connectedClients.insert({ connectionMessage.clientId, std::move(connectedClient)});
		std::cout << "Client connected" << std::endl;
		std::thread(&Server::serverClientThread, this, connectionMessage.clientId).detach();
	}
}

void Server::manageNonEmptyTopic(std::string topicId) {
	while (!topicMap[topicId].messages.empty()) {
		auto message = topicMap[topicId].messages.front();
		topicMap[topicId].messages.pop();
		// process message here, send to all connected clients
		for (auto client : connectedClients) {
			if (client.second.subscriberTo.find(topicId) != client.second.subscriberTo.end()) {
				
				sf::Packet subscriberMessage;
				subscriberMessage << message;

				if (client.second.clientSocket->send(subscriberMessage) != sf::Socket::Done) {
					// delete client here
					std::cout << "Error sending message to client" << std::endl;
				}
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
				topicsBeingProcessed.find(topic.first) == topicsBeingProcessed.end()){
				topicsBeingProcessed.insert(topic.first);
				std::thread(&Server::manageNonEmptyTopic, this, topic.first).detach();
			}
		}
	}


}

// message structure
// clientid|toTopic1:toTopic2...|content
Message Server::parseMessage(sf::Packet& message) {

	Message wrongMessage;
	wrongMessage.isCorrect = false;

	Message parsed;
	std::string data;
	int dataPointer = 0;
	std::string clientId = "";
	std::string content = "";
	std::string topic = "";
	std::vector<std::string> toTopics;


	if (!(message >> data))
	{
		return wrongMessage;
	}

	// collect the client id
	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|') {
			dataPointer++;
			break;
		}
		else {
			clientId.push_back(data[dataPointer]);
			dataPointer++;
		}
	}


	if (clientId.size() != CLIENT_ID_SIZE) {

		return wrongMessage;
	}

	// collect all topics
	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|') {
			if (topic.size() == TOPIC_ID_SIZE) {
				toTopics.push_back(topic);
			}

			dataPointer++;

			break;
		}
		else if (data[dataPointer] == ':') {
			if (topic.size() != TOPIC_ID_SIZE) {

				parsed.isCorrect = false;
				return parsed;
			}

			toTopics.push_back(topic);
			topic = "";
		}
		else {
			topic.push_back(data[dataPointer]);
			dataPointer++;
		}
	}

	while (dataPointer < data.size()) {
		content += data[dataPointer];
		dataPointer++;
	}

	if (content.size() == 0) {
		return wrongMessage;
	}

	// finally, verify the topic ids are correct
	for (int i = 0; i < toTopics.size(); ++i) {
		if (topicMap.find(toTopics[i]) == topicMap.end()) {
			return wrongMessage;
		}
	}

	parsed.clientId = clientId;
	parsed.content = content;
	parsed.toTopics = toTopics;
	parsed.isCorrect = true;
	return parsed;
}

void Server::createTopic(std::string topicId, int maxAllowedConnections) {
	Topic topic;
	topic.maxAllowedConnections = maxAllowedConnections;
	topic.topicId = topicId;
	topicMap.insert({ topicId, topic });
}

void Server::serverClientThread(std::string clientId) {
	while (serverIsRunning) {
		auto client = connectedClients[clientId];

		sf::Packet packet;

		if (client.clientSocket->receive(packet) != sf::Socket::Done)
		{
			break;
		}
		
		Message message = parseMessage(packet);

		if (!message.isCorrect) {
			break;
		}

		for (int i = 0; i < message.toTopics.size(); ++i) {
			topicMap[message.toTopics[i]].messages.push(message.content);
		}
	}
}

ConnectionMessage Server::parseConnectionPacket(sf::Packet& connectionPacket) {
	ConnectionMessage connectionMessage;
	std::string data;

	if (!(connectionPacket >> data))
	{
		connectionMessage.isCorrect = false;
		return connectionMessage;
	}


	std::string clientId = "";
	std::vector<std::string> publisherTo;
	std::vector<std::string> subscriberTo;
	
	int dataPointer = 0;

	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|') {
			dataPointer++;
			break;
		}
		else {
			clientId.push_back(data[dataPointer]);
			dataPointer++;
		}
	}

	if (clientId.size() != CLIENT_ID_SIZE) {    // client id is exactly 4 characters
		connectionMessage.isCorrect = false;

		return connectionMessage;
	}

	std::string publishTo = "";

	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|'){
			if (publishTo.size() == TOPIC_ID_SIZE) {
				publisherTo.push_back(publishTo);
			}

			dataPointer++;

			break;
		}
		else if (data[dataPointer] == ':') {

			if (publishTo.size() != TOPIC_ID_SIZE) {
				connectionMessage.isCorrect = false;
				return connectionMessage;
			}

			publisherTo.push_back(publishTo);
			publishTo = "";
			dataPointer++;
		}
		else {
			publishTo.push_back(data[dataPointer]);
			dataPointer++;
		}


	}

	std::string subscribeTo = "";

	while (dataPointer < data.size()) {
		if (data[dataPointer] == '|') {
			break;
		}
		else if (data[dataPointer] == ':') {

			if (subscribeTo.size() != TOPIC_ID_SIZE) {
				connectionMessage.isCorrect = false;
				return connectionMessage;
			}

			subscriberTo.push_back(subscribeTo);
			subscribeTo = "";
			dataPointer++;
		}
		else {
			subscribeTo.push_back(data[dataPointer]);
			dataPointer++;
		}
	}

	if (subscribeTo.size() == TOPIC_ID_SIZE) {
		subscriberTo.push_back(subscribeTo);
	}

	connectionMessage.isCorrect = true;
	connectionMessage.clientId = clientId;
	connectionMessage.publisherTo = publisherTo;
	connectionMessage.subscriberTo = subscriberTo;

	return connectionMessage;
}