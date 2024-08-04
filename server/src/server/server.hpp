#pragma once

#include "SFML/Network.hpp"
#include "../src/topic/topic.hpp"
#include "../src/client/client.hpp"
#include "../src/message/message_serializer.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <set>
#include <unordered_set>
#include <utility>

#define CLIENT_ID_SIZE 4
#define MESSAGE_ACTION_ID_SIZE 4 // there can be at most 9999 actions
#define TOPIC_ID_SIZE 6
#define EMPTY_STR ""

/*
    @param listener: Represents a TCP socket object that is configured to listen for new clients
    The listener is the doorway to the server and how a client establishes a connection

    @param topic: maps a topic id to a topic object
*/
class Server {
private:
    MessageSerializer messageSerializer;
    sf::TcpListener listener;
    sf::IpAddress ip;
    unsigned short port;
    bool serverIsRunning;
    std::unordered_map<std::string, ConnectedClient> connectedClients;
    std::unordered_map<std::string, Topic> topicMap;  
    std::set<std::string> topicsBeingProcessed;
    std::vector<std::thread> processPool;
public:
    Server(std::string _ip, unsigned int _port);
    void serverLoop();
    void startServer();
    void serverClientThread(std::string clientId);
    void createTopic(std::string topicId, int maxAllowedConnections);
    void manageNonEmptyTopic(std::string topicId);
    
    // Processing
    Header processHeader(std::string headerContent);
    bool processMessageContent(nlohmann::json& content,MessageActionType actionType,std::string messageContent);
    void messageProcessing();
    void processMessage(Message& message);
    ConnectedClient connectClient(Message& message);

    // Parsing
    bool Server::parseMessage(sf::Packet& packet, Message& message);
};