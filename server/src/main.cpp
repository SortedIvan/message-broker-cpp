#include "SFML/Graphics.hpp"
#include "SFML/Network.hpp"
#include "server/server.hpp"
#include "msgpack.hpp"
#include <sstream>
#include <iostream>

int main() {
    std::vector<std::pair<std::string, int>> topics = {
        {"server", 1000},   // important topic, only serves to communicate global server messages
        {"topic1", 10},     // every client subs to the server topic upon connection
        {"topic2", 2},
        {"topic3", 1}
    };

    Server server("127.0.0.1", 54000, topics);
    server.serverLoop();

    return EXIT_SUCCESS;
}