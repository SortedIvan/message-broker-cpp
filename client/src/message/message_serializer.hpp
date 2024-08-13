#pragma once
#include "SFML/Network.hpp"
#include <sstream>
#include "msgpack.hpp"

template <typename T>
static T deserialize(std::string& data) {
    msgpack::object_handle objectHandle = msgpack::unpack(data.data(), data.size());
    msgpack::object deserialized = objectHandle.get();

    T deserializedObject;
    deserialized.convert(deserializedObject);
    return deserializedObject;
}


// Serialize method for any type
template <typename T>
static std::string serialize(const T& data) {
    // Serialize the data into a stringstream buffer
    std::stringstream buffer;
    msgpack::pack(buffer, data);
    return buffer.str();
}

static sf::Packet packData(const std::vector<char>& serializedData) {
    sf::Packet packet;
    packet << static_cast<sf::Uint32>(serializedData.size());
    packet.append(serializedData.data(), serializedData.size());
    return packet;
}
