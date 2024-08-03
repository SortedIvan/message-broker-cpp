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

		std::thread(&Server::serverClientThread, this, client).detach();
	}
}

void Server::manageNonEmptyTopic(std::string topicId) {
	while (!topicMap[topicId].messages.empty()) {
		auto message = topicMap[topicId].messages.front();
		topicMap[topicId].messages.pop();

		if (message.content.empty()) {
			continue;
		}

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
Message Server::parseMessage(sf::Packet& packet) {
	Message message;
	Message wrongMessage;
	wrongMessage.isCorrect = false;
	std::string data;
	int dataPointer = 0;
	MessageActionType actionType = None;

	if (!(packet >> data)) return wrongMessage;
	if (data.size() == 0) return wrongMessage;

	// first, process the action_id
	std::string action_id_str = EMPTY_STR;

	while (dataPointer < data.size()) {
		if (data[dataPointer] == ':') {
			dataPointer++;
			break;
		}

		action_id_str.push_back(data[dataPointer]);
		dataPointer++;
	}

	if (action_id_str.size() > MESSAGE_ACTION_ID_SIZE) return wrongMessage;
	
	try {
		actionType = (MessageActionType)std::stoi(action_id_str);
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
		return wrongMessage;
	}

	std::string messageContent = EMPTY_STR;

	while (dataPointer < data.size()) {
		messageContent += data[dataPointer];
		dataPointer++;
	}

	nlohmann::json jsonContent;

	bool result = processMessageContent(jsonContent, actionType, messageContent);

	if (!result) {
		return wrongMessage;
	}

	return message;
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
			result = parseConnectMessage(jsonContent, messageContent);
			break;
		case Disconnect: // Disconnecting has no content attached to it
			return true;
			break;
		case SimpleMessage:
			result = parseSimpleMessage(jsonContent, messageContent);
			break;
	}

	return result;
}

/*
	connect message should contain the information about client and the topics he subscribes to
	structure: {<client_id>|<topic_pub_1>-<topic_pub_2>...|<topic_sub_1>-<topic_sub_2>..}
*/ 
bool Server::parseConnectMessage(nlohmann::json& jsonContent, std::string messageContent) {
	int contentPointer = 0;
	std::string clientId = EMPTY_STR;
	std::vector<std::string> subscribeTo;
	std::vector<std::string> publishTo;

	while (contentPointer < messageContent.size()) {
		if (messageContent[contentPointer] == '|') {
			contentPointer++;
			break;
		}

		clientId += messageContent[contentPointer];
		contentPointer++;
	}

	if (clientId.size() != CLIENT_ID_SIZE) {
		return false;
	}

	std::string pb = EMPTY_STR;
	
	while (contentPointer < messageContent.size()) {
		if (messageContent[contentPointer] == '-') {
			publishTo.push_back(pb);
			contentPointer++;
			pb = EMPTY_STR;
		}
		else if (messageContent[contentPointer] == '|') {
			contentPointer++;
			
			if (pb.size() > 0) {
				publishTo.push_back(pb);
			}

			break;
		}
		else {
			pb += messageContent[contentPointer];
			contentPointer++;
		}
	}

	std::string sb = EMPTY_STR;

	while (contentPointer < messageContent.size()) {
		if (messageContent[contentPointer] == '-') {
			subscribeTo.push_back(sb);
			contentPointer++;
			sb = EMPTY_STR;
		}
		else {
			sb += messageContent[contentPointer];
			contentPointer++;
		}
	}

	if (sb.size() > 0) {
		publishTo.push_back(sb);
	}

	if (subscribeTo.size() == 0 && publishTo.size() == 0) {
		return false;
	}

	jsonContent["client_id"] = clientId;
	jsonContent["publish_to"] = publishTo;
	jsonContent["subscribe_to"] = subscribeTo;
	return true;
}

/*
	Boilerplate method for a message that contains content
	@structure {"content":<content>}
*/
bool Server::parseSimpleMessage(nlohmann::json& jsonContent, std::string messageContent) {
	int contentPointer = 0;
	std::string contentStr = EMPTY_STR;

	while (contentPointer < messageContent.size()) {
		
		if (messageContent[contentPointer] == ':') {
			contentPointer++;
			break;
		}

		contentStr += messageContent[contentPointer];
		contentPointer++;
	}

	if (contentStr.size() == 0) {
		return false;
	}

	jsonContent["content"] = contentStr;
	return true;
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
void Server::serverClientThread(std::shared_ptr<sf::TcpSocket> client) {

	sf::Packet connectionPacket; // has a specific structure, should not connect if it doesn't match

	// check the connection message from the client
	if (client->receive(connectionPacket) != sf::Socket::Done)
	{
		std::cerr << "Error: Could not processes connection packet from client" << std::endl;
		return;
	}

	Message message = parseMessage(connectionPacket);

	if (!message.isCorrect) {
		return;
	}

	if (message.messageActionType != Connect) {
		return;
	}

	ConnectedClient connectedClient = connectClient(message);
	std::string clientId = message.content["client_id"];
	connectedClients.insert({clientId ,std::move(connectedClient) });

	while (serverIsRunning) {
		auto client = connectedClients[clientId];

		sf::Packet packet;
		
		if (client.clientSocket->receive(packet) != sf::Socket::Done)
		{
			std::cout << "Error with message receiving from client" << std::endl;

			break;
		}
		
		Message message = parseMessage(packet);

		if (!message.isCorrect) {
			break;
		}
		
		// process message here

		bool result = processMessage(message, clientId);
	}
}

bool Server::processMessage(Message& message, const std::string& clientId) {
	switch (message.messageActionType) {
		case None:
			return false;
		case Connect:
			return false;
		case Disconnect:
			return false;
		case SimpleMessage:
			return processSimpleMessage(message, clientId);
	}
}

//@structure {"content":<content>}
// TODO: Figure out a way to actually send messages
// Entire schema will have to change
bool Server::processSimpleMessage(Message& message, const std::string& clientId) {
	for (std::string pub : connectedClients[clientId].publisherTo) {
		//topicMap[pub].messages.push()
	}
}

ConnectedClient Server::connectClient(Message& message) {
	std::unordered_set<std::string> subscriberSet;
	std::unordered_set<std::string> publisherSet;
	std::vector<std::string> publishTo = message.content["publish_to"];
	std::vector<std::string> subscribeTo = message.content["subscribe_to"];

	// if the topic ids don't relate to any topics, don't add client
	for (int i = 0; i < publishTo.size(); ++i) {
		if (topicMap.find(publishTo[i]) == topicMap.end()) {
			continue;
		}

		publisherSet.insert(publishTo[i]);
	}

	for (int i = 0; i < subscribeTo.size(); ++i) {
		if (topicMap.find(subscribeTo[i]) == topicMap.end()) {
			continue;
		}

		subscriberSet.insert(subscribeTo[i]);
	}

	ConnectedClient connectedClient;
	connectedClient.subscriberTo = subscriberSet;
	connectedClient.publisherTo = publisherSet;

	return connectedClient;
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