typedef enum
{
	DATA = 0,
	HELLO,  // Send HELLO packet with ttl=1 to my neighbours.
	ACK,
	EDGE  // Send out the information with all my neighbours.
} packet_types;

struct PacketHeader
{
	uint8_t type;
	uint8_t sequence;
	uint8_t source;
	uint8_t destination;
	uint32_t size;
	uint32_t ttl;
};