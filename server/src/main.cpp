#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "server/server.hpp"
#include "msgpack.hpp"
#include <sstream>
#include <iostream>

int main() {
    std::vector<std::pair<std::string, int>> topics = 
    {
        {SERVER_TOPIC, 1000},   // topic that only serves to communicate global server messages
        {"topic1", 10},   
        {"topic2", 2},
        {"topic3", 1}
    };

    Server server("127.0.0.1", 54000, topics);
    server.serverLoop();
    server.termination();
    return EXIT_SUCCESS;
}