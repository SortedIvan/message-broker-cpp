#include "message.hpp"
#include "SFML/Network.hpp"
#include <sstream>

class MessageSerializer {
public:
    Message deserialize(sf::Packet& packet);
    std::vector<char> serialize(const Message& data);
    sf::Packet packData(const std::vector<char>& serializedData);
};