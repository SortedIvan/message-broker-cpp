#pragma once

#include "SFML/Network.hpp"
#include "../src/topic/topic.hpp"
#include "../src/message/message.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

/*
    @param listener: Represents a TCP socket object that is configured to listen for new clients
    The listener is the doorway to the server and how a client establishes a connection

    @param topic: maps a topic id to a topic object
*/
class Server {
private:
    sf::TcpListener listener;
    sf::IpAddress ip;
    unsigned short port;
    bool serverIsRunning;
    std::vector<sf::TcpSocket> connectedClients;
    std::unordered_map<std::string, Topic> topics;
public:
    Server(std::string _ip, unsigned int _port);
    void serverLoop();
    void startServer();
    void serverClientThread();
    ConnectionMessage parseConnectionPacket(sf::Packet& connectionPacket);
};