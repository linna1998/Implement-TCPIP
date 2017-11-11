#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "BasicRouter.hh" 
#include "Packets.hh"

CLICK_DECLS

BasicRouter::BasicRouter() {}
BasicRouter::~BasicRouter() {}

int BasicRouter::initialize(ErrorHandler *errh) 
{
	return 0;
}

void BasicRouter::push(int port, Packet *packet) 
{
	assert(packet);
	struct PacketHeader *header = (struct PacketHeader *)packet->data();
	if (header->type == DATA || header->type == ACK) 
	{
		click_chatter("Received Data from %u with destination %u", header->source, header->destination);
		int next_port = _ports_table.get(header->destination);
		output(next_port).push(packet);
	}
	else if (header->type == HELLO) 
	{
		click_chatter("Received Hello from %u on port %d", header->source, port);
		_ports_table.set(header->source, port);
	}
	else 
	{
		click_chatter("Wrong packet type");
		packet->kill();
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicRouter)
