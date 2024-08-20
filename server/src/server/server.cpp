#include "server.hpp"
#include "../src/message/message_serializer.hpp"

Server::Server(std::string _ip, unsigned int _port, const std::vector<std::pair<std::string, int>>& topics) {
	ip = _ip;
	port = _port;
	serverIsRunning = false;
	topicProcessPool.initialize(4);

	for (auto topic : topics) {
		createTopic(topic.first, topic.second);
	}

	topicProcessingThread = std::thread(&Server::processNonEmptyTopicThreadAssignee, this);
}

Server::~Server() {
	serverIsRunning = false;

	// signal to all threads that server has shut down

	for (int i = 0; i < serverClientThreads.size(); ++i) {
		serverClientThreads[i].join();
	}

	sendServerShutdownMessageToClients("Server object has been destroyed");
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

		std::thread t(&Server::serverClientThread, this, client);
		threadFinished.insert({ t.get_id(), false });
		serverClientThreads.push_back(std::move(t));
	}
	
	for (int i = 0; i < serverClientThreads.size(); ++i) {
		serverClientThreads[i].join();
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
		threadFinished[std::this_thread::get_id()] = true;
		return;
	}

	if (connectionMessage.messageActionType != Connect) {
		std::cerr << "Message not correct type" << std::endl;
		threadFinished[std::this_thread::get_id()] = true;
		return;
	}

	ConnectedClient connectedClient;
	bool clientConnectionResult = connectClient(connectionMessage, connectedClient);

	if (!clientConnectionResult) {
		std::cerr << "Issue with connecting client" << std::endl;
		threadFinished[std::this_thread::get_id()] = true;
		return;
	}

	std::string clientId = connectionMessage.sender;
	
	if (clientId.size() < CLIENT_ID_SIZE) {
		std::cerr << "Incorrect client id size" << std::endl;
		threadFinished[std::this_thread::get_id()] = true;
		return;
	}
	
	connectedClient.clientId = clientId;
	connectedClient.clientSocket = std::move(clientSocket);
	connectedClients.insert({ clientId, std::move(connectedClient)});

	while (serverIsRunning) {
		sf::Packet packet;

		if (connectedClients.find(clientId) == connectedClients.end()) {
			break;
		}

		connectedClient = connectedClients[clientId];

		if (connectedClient.clientSocket->receive(packet) != sf::Socket::Done)
		{
			std::cerr << "Error with message receiving from client" << std::endl;
			break;
		}

		Message message;
		bool messageParseResult = parsePacketIntoMessage(packet, message);

		if (!messageParseResult) {
			std::cerr << "Client message could not be parsed correctly" << std::endl;
			break;
		}

		/*
			@dev process functionality
			Manage any needed actions embedded in the message
			& handle pushing messages into the appropriate topic
		*/
		bool processingResult = processMessage(message);

		if (!processingResult) {
			std::cerr << "There was an error processing a message" << std::endl;
			break;
		}

		removeFinishedOrDeadThreads();
	}

	threadFinished[std::this_thread::get_id()] = true;
	connectedClients.erase(clientId);
}

bool Server::processMessage(Message& message) {
	bool processResult = false;

	switch (message.messageActionType) {
		case None:
			return false;
		case Connect:
			return false;
		case Disconnect:
			return handleClientDisconnect(message);
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

		if (client.publisherTo.find("server") != client.publisherTo.end()) {
			// client is attempting to be a publisher to server topic
			client.publisherTo.erase("server");
		}

		client.subscriberTo.insert("server"); // connect client to the global server topic
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

				if (client.first == message.sender) {
					continue; // Don't send message back to the sender
				}

				sf::Packet subscriberMessage;
				std::string messageData = EMPTY_STR;

				try {
					messageData = serialize<Message>(message);
					subscriberMessage << messageData;

					if (client.second.clientSocket->send(subscriberMessage) != sf::Socket::Done) {
						// delete client here
						std::cout << "Error sending message to client" << std::endl;
					}
					std::cout << "Sent" << std::endl;
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
	@dev uses a thread pool to assign non-empty queues as a task
*/ 
void Server::processNonEmptyTopicThreadAssignee() {
	while (serverIsRunning) {
		for (auto& topic : topicMap) {

			if (!topic.second.messages.empty() &&
				topicsBeingProcessed.find(topic.first) == topicsBeingProcessed.end())
			{
				topicsBeingProcessed.insert(topic.first);
				topicProcessPool.addTask(&Server::processMessagesFromNonEmptyTopic,this, topic.first);
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

	if (topicId.size() < TOPIC_ID_SIZE) {
		return;
	}

	Topic topic;
	topic.maxAllowedConnections = maxAllowedConnections;
	topic.topicId = topicId;
	topicMap.insert({ topicId, topic });
}

bool Server::handleClientDisconnect(Message& message) {
	if (connectedClients.find(message.sender) == connectedClients.end()) {
		return true;
	}

	std::unordered_set<std::string> subscriberTo = connectedClients[message.sender].subscriberTo;
	std::unordered_set<std::string> publisherTo = connectedClients[message.sender].publisherTo;

	// make sure that any left-over messages are processed
	for (std::string topic : subscriberTo) {
		while (topicsBeingProcessed.find(topic) != topicsBeingProcessed.end()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
	}
	
	message.actionData = "Client " + message.sender + " has disconnected";

	// first, remove the client from the connectedClients map
	connectedClients.erase(message.sender);

	// Only then after, send the disconnect message (done to avoid race condition)
	for (std::string topic : subscriberTo) {
		topicMap[topic].messages.push(message);
	}

	for (std::string topic : publisherTo) {
		topicMap[topic].messages.push(message);
	}

	std::cout << "Client { " << message.sender << "} has disconnected" << std::endl;

	return true;
}

void Server::removeFinishedOrDeadThreads() {
	std::unordered_map<std::thread::id, bool>::iterator it;
	std::vector<std::thread::id> markedForDeletion;

	for (it = threadFinished.begin(); it != threadFinished.end(); it++)
	{
		if (it->second) {
			for (int i = 0; i < serverClientThreads.size(); ++i) {
				if (serverClientThreads[i].get_id() == it->first) {
					serverClientThreads[i].join();
					serverClientThreads.erase(serverClientThreads.begin() + i);
					markedForDeletion.push_back(serverClientThreads[i].get_id());
				}
			}
		}
	}

	for (int i = 0; i < markedForDeletion.size(); ++i) {
		threadFinished.erase(markedForDeletion[i]);
	}
}

void Server::sendServerShutdownMessageToClients(std::string reason) {
	Message disconnectMessage;
	disconnectMessage.sender = "server";
	disconnectMessage.messageActionType = ServerHasShutdown;
	disconnectMessage.actionData = reason;
	topicMap["server"].messages.push(disconnectMessage);
}