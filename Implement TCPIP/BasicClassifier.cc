#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "BasicClassifier.hh" 
#include "Packets.hh"

CLICK_DECLS

BasicClassifier::BasicClassifier() {}
BasicClassifier::~BasicClassifier() {}

int BasicClassifier::initialize(ErrorHandler *errh)
{
	return 0;
}

void BasicClassifier::push(int port, Packet *packet)
{
	assert(packet);
	struct PacketHeader *header = (struct PacketHeader *)packet->data();
	if (header->type == 0)
	{
		output(0).push(packet);
	}
	else if (header->type == 1)
	{
		output(1).push(packet);
	}
	else if (header->type == 2)
	{
		output(2).push(packet);
	}
	else
	{
		click_chatter("Wrong packet type");
		packet->kill();
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicClassifier)
