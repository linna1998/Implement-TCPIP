#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "BasicClassifier.hh" 
#include "TCPPackets.hh"

CLICK_DECLS

BasicClassifier::BasicClassifier() {}
BasicClassifier::~BasicClassifier() {}

int BasicClassifier::initialize(ErrorHandler *errh)
{
	return 0;
}

void BasicClassifier::push(int port, Packet *packet)
{
	// Classify packets ,then send them to TCP layer.
	assert(packet);
	struct TCPPacketHeader *header = (struct TCPPacketHeader *)packet->data();
	if (header->type == DATA)
	{
		output(0).push(packet);
	}
	else if (header->type == HELLO || header->type == EDGE)
	{
		output(1).push(packet);
	}
	else if (header->type == ACK)
	{
		output(2).push(packet);
	}
	else
	{
		// Wrong packet type.
		packet->kill();
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicClassifier)
