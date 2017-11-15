#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "TCPClient.hh" 
#include "TCPPackets.hh"

using namespace std;

CLICK_DECLS

TCPClient::TCPClient() : _timerTO(this), _timerRETR(this)
{
	_seq = 1;
	LAR = 0;
	LFS = 0;
	LFE = 0;
	start = 0;
	empty = 1;
	_period = 10;
	_periodRetr = 3 * _period;
	_delay = 0;
	_my_address = 0;
	_other_address = 0;
	used_Qlen = 0;
	old_seq = 0;
	new_seq = 0;
	retranFlag = 0;

	for (int i = 0; i < WINDOWSIZE; i++)
	{
		transmissions[i] = 0;
	}

}

TCPClient::~TCPClient() {}

int TCPClient::initialize(ErrorHandler *errh)
{
	_timerTO.initialize(this);
	_timerRETR.initialize(this);
	if (_delay > 0)
	{
		_timerTO.schedule_after_sec(_delay);
	}
	_timerRETR.schedule_after_sec(_periodRetr);

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
		click_chatter("TCP: Open log file failed!");
	}
	else
	{
		fprintf(log, "%s", "----------TCP client log----------\n");
	}
	return 0;
}

int TCPClient::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
		"MY_ADDRESS", cpkP + cpkM, cpUnsigned, &_my_address,
		"OTHER_ADDRESS", cpkP + cpkM, cpUnsigned, &_other_address,
		"DELAY", cpkP, cpUnsigned, &_delay,
		"PERIOD", cpkP, cpUnsigned, &_period,
		"FILENAME", cpkP, cpString, &s_filename,
		cpEnd) < 0)
	{
		return -1;
	}
	return 0;
}

void TCPClient::run_timer(Timer *timer)
{
	if (timer == &_timerTO)
	{
		if (start == 1)
		{
			if (((LFS + QLENGTH - LAR) % QLENGTH < WINDOWSIZE) && (used_Qlen>0))
			{
				LFS = (LFS + 1) % QLENGTH;
				WritablePacket *TCPpacket = DataQueue[LFS];
				WritablePacket *newTCPpacket = Packet::make(0, 0, TCPpacket->length(), 0);
				memcpy(newTCPpacket->data(), TCPpacket->data(), newTCPpacket->length());
				transmissions[LFS%WINDOWSIZE]++;
				output(0).push(newTCPpacket);
				click_chatter("TCP: Transmitting packet %u for %d time.",
					LFS, transmissions[LFS%WINDOWSIZE]);
				fprintf(log, "Transmitting packet %u for %d time.\n",
					LFS, transmissions[LFS%WINDOWSIZE]);

				if (retranFlag == 0)  // Slow-start Algorithm.
				{
					_period = _period / 2 + 1;
					_periodRetr = _period * 3;
					click_chatter("TCP: Slow-start, change _period to %d.", _period);
					fprintf(log, "Slow-start, change _period to %d.\n", _period);
				}
			}
		}
		_timerTO.schedule_after_sec(_period);
	}
	else if (timer == &_timerRETR)
	{

		if (start == 1)
		{
			new_seq = (LAR + 1) % QLENGTH;
			if (new_seq == old_seq)
			{
				retranFlag++;
				// MD.
				if (transmissions[(LAR + 1) % WINDOWSIZE] >= 3)
				{
					_period = _period + 1;
					_periodRetr = _period * 3;
					click_chatter("TCP: MD, change _period to %d.", _period);
					fprintf(log, "MD, change _period to %d.\n", _period);
				}
				WritablePacket *TCPpacket = DataQueue[(LAR + 1) % QLENGTH];
				WritablePacket *newTCPpacket = Packet::make(0, 0, (DataQueue[(LAR + 1) % QLENGTH])->length(), 0);
				memcpy(newTCPpacket->data(), TCPpacket->data(), newTCPpacket->length());
				transmissions[(LAR + 1) % WINDOWSIZE]++;
				output(0).push(newTCPpacket);
				click_chatter("TCP: Transmitting packet %u for %d time.",
					new_seq, transmissions[(LAR + 1) % WINDOWSIZE]);
				fprintf(log, "Transmitting packet %u for %d time.\n",
					new_seq, transmissions[(LAR + 1) % WINDOWSIZE]);
			}
			old_seq = new_seq;
		}
		_timerRETR.schedule_after_sec(_periodRetr);
	}
	else
	{
		assert(false);
	}

}

void TCPClient::push(int port, Packet *packet)
{
	assert(packet);
	if (port == 0)
	{
		// Received data from ip_client.
		// TCP send ack back to IP layer, send data to user_client.
		struct TCPPacketHeader *header = (struct TCPPacketHeader *) packet->data();
		WritablePacket *ack = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
		memset(ack->data(), 0, ack->length());
		struct TCPPacketHeader *format = (struct TCPPacketHeader*) ack->data();
		format->type = ACK;
		format->syn = 0;
		format->ack = 1;
		format->fin = 0;
		format->sequence = header->sequence;
		format->source = _my_address;
		format->destination = header->source;
		format->size = sizeof(struct TCPPacketHeader);
		output(0).push(ack);

		// Send data to the user_click -> Go Back To N (receiver don't have buffer)
		// Delete TCP Header.
		char *data = (char*)(packet->data() + sizeof(struct TCPPacketHeader));
		unsigned int tcpHeadLen = sizeof(struct TCPPacketHeader);
		WritablePacket *dataPacket = Packet::make(0, 0, header->size - tcpHeadLen, 0);
		memcpy(dataPacket->data(), data, dataPacket->length());
		output(1).push(dataPacket);

		packet->kill();
	}
	else if (port == 1)
	{
		// Received ack from ip_user -> schedule new data packet
		// Judge ack, syn, fin.
		struct TCPPacketHeader *header = (struct TCPPacketHeader *)packet->data();
		int syn = header->syn, ack = header->ack, fin = header->fin;

		if (syn == 1 && ack == 2 && fin == 0)  // Active ok.
		{
			click_chatter("TCP: Received SYN %u from %u. Active ok.", 2, header->source);
			fprintf(log, "Received SYN %u from %u. Active ok.\n", 2, header->source);
			start = 1;
			WritablePacket *syn = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
			memset(syn->data(), 0, syn->length());
			struct TCPPacketHeader *format = (struct TCPPacketHeader*) syn->data();
			format->type = ACK;
			format->syn = 2;
			format->ack = 2;
			format->fin = 0;
			format->source = header->destination;
			format->destination = header->source;
			format->size = sizeof(struct TCPPacketHeader);

			output(0).push(syn);
		}
		else if (syn == 1 && ack == 0 && fin == 0)//passive syn
		{
			WritablePacket *syn = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
			memset(syn->data(), 0, syn->length());
			struct TCPPacketHeader *format = (struct TCPPacketHeader*) syn->data();
			format->type = ACK;
			format->syn = 1;
			format->ack = 2;
			format->fin = 0;
			format->source = header->destination;
			format->destination = header->source;
			format->size = sizeof(struct TCPPacketHeader);

			output(0).push(syn);
		}
		else if (syn == 2 && ack == 2 && fin == 0)//passive ok
		{
			click_chatter("TCP: Received SYN %u from %u. Passive ok.", 3, header->source);
			fprintf(log, "Received SYN %u from %u. Passive ok.\n", 3, header->source);
			start = 1;
		}
		else if (syn == 0 && ack == 2 && fin == 1)//active break
		{
			click_chatter("TCP: Received FIN ACK from %u. Active break.", header->source);
			fprintf(log, "Received FIN ACK from %u. Active break.\n", header->source);
			start = 0;
		}
		else if (syn == 0 && ack == 0 && fin == 1)//Passive close. ack fin and passive send fin
		{
			WritablePacket *finack = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
			memset(finack->data(), 0, finack->length());
			struct TCPPacketHeader *ackformat = (struct TCPPacketHeader*) finack->data();
			ackformat->type = ACK;
			ackformat->syn = 0;
			ackformat->ack = 2;
			ackformat->fin = 1;
			ackformat->source = header->destination;
			ackformat->destination = header->source;
			ackformat->size = sizeof(struct TCPPacketHeader);
			output(0).push(finack);

			WritablePacket *fin = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
			memset(fin->data(), 0, fin->length());
			struct TCPPacketHeader *finformat = (struct TCPPacketHeader*) fin->data();
			finformat->type = ACK;
			finformat->syn = 0;
			finformat->ack = 0;
			finformat->fin = 2;
			finformat->source = header->destination;
			finformat->destination = header->source;
			finformat->size = sizeof(struct TCPPacketHeader);
			output(0).push(fin);
		}
		else if (syn == 0 && ack == 0 && fin == 2)//passive fin and ack send fin ack
		{
			WritablePacket *finack = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
			memset(finack->data(), 0, finack->length());
			struct TCPPacketHeader *format = (struct TCPPacketHeader*) finack->data();
			format->type = ACK;
			format->syn = 0;
			format->ack = 3;
			format->fin = 2;
			format->source = header->destination;
			format->destination = header->source;
			format->size = sizeof(struct TCPPacketHeader);

			output(0).push(finack);

		}
		else if (syn == 0 && ack == 3 && fin == 2)//passive break
		{
			click_chatter("TCP: Received FIN ACK from %u. Passive break.", header->source);
			fprintf(log, "TCP: Received FIN ACK from %u. Passive break.\n", header->source);
			start = 0;
		}
		else if (ack == 1 && syn == 0 && fin == 0)
		{
			click_chatter("Received ACK %u from %u.", header->sequence, header->source);
			fprintf(log, "Received ACK %u from %u.\n", header->sequence, header->source);
			if (header->sequence == (_seq%QLENGTH))
			{
				transmissions[_seq%WINDOWSIZE] = 0;

				//AI
				if (_period >= 2)
				{
					_period = _period - 1;
					_periodRetr = _period * 3;
					click_chatter("TCP: AI, change _period to %d.", _period);
					fprintf(log, "AI, change _period to %d.\n", _period);
				}
				LAR = _seq%QLENGTH;
				_seq = (_seq + 1) % QLENGTH;
				used_Qlen -= 1;

				// There is no more data to be sent.
				// The connection is closed.
				if (LAR == LFS && LAR == LFE)
				{
					empty = 1;
					retranFlag = 0;
					Break();
				}
			}
		}
		else
		{
			// Received wrong sequence number.
			packet->kill();
		}
	}

	// Data from the user_click, send to the ip_click.
	else if (port == 2)
	{
		// Three-Way Handshake -> send SYN 1.
		if (start == 0)
		{
			WritablePacket *syn = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
			memset(syn->data(), 0, syn->length());
			struct TCPPacketHeader *format = (struct TCPPacketHeader*) syn->data();
			format->type = ACK;
			format->syn = 1;
			format->ack = 0;
			format->fin = 0;
			format->source = _my_address;
			format->destination = _other_address;
			format->size = sizeof(struct TCPPacketHeader);

			output(0).push(syn);
		}

		// Add TCP header.
		// packet : the packet from user_client.
		LFE = (LFE + 1) % QLENGTH;
		WritablePacket *TCPpacket = Packet::make(0, 0, sizeof(struct TCPPacketHeader) + sizeof(packet), 0);
		memset(TCPpacket->data(), 0, TCPpacket->length());
		struct TCPPacketHeader *format = (struct TCPPacketHeader*) TCPpacket->data();
		format->type = DATA;
		format->sequence = LFE%QLENGTH;
		format->syn = 0;
		format->ack = 0;
		format->fin = 0;
		format->source = _my_address;
		format->destination = _other_address;
		format->size = sizeof(struct TCPPacketHeader) + sizeof(packet);
		char *data = (char*)(TCPpacket->data() + sizeof(struct TCPPacketHeader));
		memcpy(data, packet->data(), sizeof(packet));

		// Enqueue.	
		if (used_Qlen != QLENGTH)
		{
			DataQueue[LFE] = TCPpacket;
			used_Qlen += 1;
			empty = 0;
		}
		else
		{
			LFE = (LFE - 1 + QLENGTH) % QLENGTH;
		}
	}
	else
	{
		packet->kill();
	}
}

void TCPClient::Break()
{
	if (empty == 1 && start == 1) {
		WritablePacket *fin = Packet::make(0, 0, sizeof(struct TCPPacketHeader), 0);
		memset(fin->data(), 0, fin->length());
		struct TCPPacketHeader *format = (struct TCPPacketHeader*) fin->data();
		format->type = ACK;
		format->syn = 0;
		format->ack = 0;
		format->fin = 1;
		format->source = _my_address;
		format->destination = _other_address;
		format->size = sizeof(struct TCPPacketHeader);

		output(0).push(fin);
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TCPClient)
