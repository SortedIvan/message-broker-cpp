#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "server/server.hpp"
#include "msgpack.hpp"
#include <iostream>
#include <sstream>

int main() {
    Server server("127.0.0.1", 54000);
    server.createTopic("topic1", 10);
    server.createTopic("topic2", 2);
    server.createTopic("topic3", 1);
    std::thread(&Server::processNonEmptyTopicThreadCreator, std::ref(server)).detach();
    server.serverLoop();
	return 0;
}