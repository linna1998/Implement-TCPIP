struct IPPacketHeader
{
	uint8_t type;
	uint8_t source;
	uint8_t destination;
	uint32_t size;
	uint32_t ttl;
};