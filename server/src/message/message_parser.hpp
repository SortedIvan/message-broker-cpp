#include "message_serializer.hpp"

class MessageParser {
private:
	MessageSerializer messageSerializer;
public:
	MessageParser(MessageSerializer _messageSerializer);
	ConnectionMessage parseConnectionMessage(const std::vector<char>& data);
};