#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "IPClient.hh" 
#include "IPPackets.hh"
#include "TCPPackets.hh"
CLICK_DECLS

IPClient::IPClient() : _timerHello(this)
{
	_periodHello = 1;
	_my_address = 0;
	_other_address = 0;
}

IPClient::~IPClient() {}

int IPClient::initialize(ErrorHandler *errh)
{
	_timerHello.initialize(this);
	_timerHello.schedule_after_sec(_periodHello);

	// Open the log file and initialize.
	int filelen = s_filename.length();
	for (int i = 0; i < filelen; i++)
	{
		c_filename[i] = s_filename[i];
	}
	for (int i = filelen; i < FILENAMELEN; i++)
	{
		c_filename[i] = 0;
	}
	log = fopen(c_filename, "w+");
	if (log == NULL)
	{
		click_chatter("IP: Open ip log file failed!");
	}
	else
	{
		fprintf(log, "%s", "----------IP client log----------\n");
	}

	return 0;
}

int IPClient::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
		"MY_ADDRESS", cpkP + cpkM, cpUnsigned, &_my_address,
		"OTHER_ADDRESS", cpkP + cpkM, cpUnsigned, &_other_address,
		"PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
		"FILENAME", cpkP, cpString, &s_filename,
		cpEnd) < 0)
	{
		return -1;
	}
	return 0;
}

void IPClient::run_timer(Timer *timer)
{
	if (timer == &_timerHello)
	{
		WritablePacket *packet = Packet::make(0, 0, sizeof(struct IPPacketHeader), 0);
		memset(packet->data(), 0, packet->length());
		struct IPPacketHeader *format = (struct IPPacketHeader*) packet->data();
		format->type = HELLO;
		format->source = _my_address;
		format->size = sizeof(struct IPPacketHeader);

		output(0).push(packet);
		_timerHello.schedule_after_sec(_periodHello);
	}
	else
	{
		assert(false);
	}
}

void IPClient::push(int port, Packet *packet)
{
	assert(packet);

	// ip_client output 0 : to router
	// ip_client output 1 : to tcp_client
	// ip_client input 0 : from router
	// ip_client input 1 : from  tcp_client

	// Packet from router.
	if (port == 0)
	{
		struct IPPacketHeader * header = (IPPacketHeader *)packet->data();
		if (header->type == HELLO)
		{
			packet->kill();
		}
		else
		{
			// Delete IP header.
			unsigned int ipHeadLen = sizeof(struct IPPacketHeader);
			char *data = (char*)(packet->data() + ipHeadLen);
			WritablePacket *tcpPacket = Packet::make(0, 0, header->size - ipHeadLen, 0);
			memcpy(tcpPacket->data(), data, tcpPacket->length());

			// Send to TCP layer.
			output(1).push(tcpPacket);
			packet->kill();
		}
	}

	// From tcp_client.
	else if (port == 1)
	{
		// Add IP header.
		struct TCPPacketHeader *tcpheader = (struct TCPPacketHeader *)packet->data();
		char *data = (char*)(packet->data());
		unsigned int ipHeadLen = sizeof(struct IPPacketHeader);
		WritablePacket *ipPacket = Packet::make(0, 0, tcpheader->size + ipHeadLen, 0);
		memset(ipPacket->data(), 0, ipHeadLen);
		struct IPPacketHeader *header = (struct IPPacketHeader *)ipPacket->data();
		header->type = tcpheader->type;
		header->source = tcpheader->source;
		header->destination = tcpheader->destination;
		header->size = ipPacket->length();
		memcpy(ipPacket->data() + ipHeadLen, data, tcpheader->size);

		// Push to Router.
		output(0).push(ipPacket);
		packet->kill();
	}
	else
	{
		packet->kill();
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(IPClient)
