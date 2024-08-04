#include "message_serializer.hpp"

template <typename T>
static T deserialize(sf::Packet& packet) {
    // Extract the serialized data
    std::string data;
    packet >> data;
    std::vector<char> serializedData(data.begin(), data.end());

    // Deserialize the data using MessagePack
    std::string str(serializedData.begin(), serializedData.end());
    msgpack::object_handle objectHandle = msgpack::unpack(str.data(), str.size());
    msgpack::object deserialized = objectHandle.get();

    T deserializedObject;
    deserialized.convert(deserializedObject);
    return deserializedObject;
}


// Serialize method for any type
template <typename T>
static std::vector<char> serialize(const T& data) {
    // Serialize the data into a stringstream buffer
    std::stringstream buffer;
    msgpack::pack(buffer, data);

    std::string str = buffer.str();
    return std::vector<char>(str.begin(), str.end());
}

sf::Packet MessageSerializer::packData(const std::vector<char>& serializedData) {
    sf::Packet packet;
    packet << static_cast<sf::Uint32>(serializedData.size());
    packet.append(serializedData.data(), serializedData.size());
    return packet;
}
