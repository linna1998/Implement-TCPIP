#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "Client.hh" 
#include "Packets.hh"

CLICK_DECLS

BasicClient::BasicClient() : _timerTO(this), _timerHello(this) 
{
	_seq = 0;
	_period = 3;
	_periodHello = 2;
	_delay = 0;
	_time_out = 1;
	_my_address = 0;
	_other_address = 0;
	transmissions = 0;
}

BasicClient::~BasicClient() {}

int BasicClient::initialize(ErrorHandler *errh) 
{
	_timerTO.initialize(this);
	_timerHello.initialize(this);
	if (_delay>0) 
	{
		_timerTO.schedule_after_sec(_delay);
	}
	_timerHello.schedule_after_sec(_periodHello);
	return 0;
}

int BasicClient::configure(Vector<String> &conf, ErrorHandler *errh) 
{
	if (cp_va_kparse(conf, this, errh,
		"MY_ADDRESS", cpkP + cpkM, cpUnsigned, &_my_address,
		"OTHER_ADDRESS", cpkP + cpkM, cpUnsigned, &_other_address,
		"DELAY", cpkP, cpUnsigned, &_delay,
		"PERIOD", cpkP, cpUnsigned, &_period,
		"PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
		"TIME_OUT", cpkP, cpUnsigned, &_time_out,
		cpEnd) < 0) 
	{
		return -1;
	}
	return 0;
}

void BasicClient::run_timer(Timer *timer) 
{
	if (timer == &_timerTO) 
	{
		click_chatter("Retransmitting packet %u for %d time", _seq, transmissions);
		WritablePacket *packet = Packet::make(0, 0, sizeof(struct PacketHeader) + 5, 0);
		memset(packet->data(), 0, packet->length());
		struct PacketHeader *format = (struct PacketHeader*) packet->data();
		format->type = DATA;
		format->sequence = _seq;
		format->source = _my_address;
		format->destination = _other_address;
		format->size = sizeof(struct PacketHeader) + 5;
		char *data = (char*)(packet->data() + sizeof(struct PacketHeader));
		memcpy(data, "hello", 5);
		output(0).push(packet);
		transmissions++;
		_timerTO.schedule_after_sec(_time_out);
	}
	else if (timer == &_timerHello) 
	{
		click_chatter("Sending new Hello packet");
		WritablePacket *packet = Packet::make(0, 0, sizeof(struct PacketHeader), 0);
		memset(packet->data(), 0, packet->length());
		struct PacketHeader *format = (struct PacketHeader*) packet->data();
		format->type = HELLO;
		format->source = _my_address;
		format->size = sizeof(struct PacketHeader);
		output(0).push(packet);
		_timerHello.schedule_after_sec(_periodHello);
	}
	else 
	{
		assert(false);
	}
}

void BasicClient::push(int port, Packet *packet) 
{
	assert(packet);
	if (port == 0) 
	{ //data -> send ack
		struct PacketHeader *header = (struct PacketHeader *)packet->data();
		click_chatter("Received packet %u from %u", header->sequence, header->source);
		WritablePacket *ack = Packet::make(0, 0, sizeof(struct PacketHeader), 0);
		memset(ack->data(), 0, ack->length());
		struct PacketHeader *format = (struct PacketHeader*) ack->data();
		format->type = ACK;
		format->sequence = header->sequence;
		format->source = _my_address;
		format->destination = header->source;
		format->size = sizeof(struct PacketHeader);
		packet->kill();
		output(0).push(ack);
	}
	else if (port == 1) 
	{ //received ack -> schedule new data packet
		struct PacketHeader *header = (struct PacketHeader *)packet->data();
		click_chatter("Received ack %u from %u", header->sequence, header->source);
		if (header->sequence == _seq) 
		{
			_timerTO.unschedule();
			_seq++;
			transmissions = 0;
			_timerTO.schedule_after_sec(_period);
		}
		else 
		{ //received wrong sequence number
			packet->kill();
		}
	}
	else 
	{
		packet->kill();
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicClient)
