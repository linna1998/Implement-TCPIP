#ifndef CLICK_TCPCLIENT_HH 
#define CLICK_TCPCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <string.h>
#include <stdio.h>

CLICK_DECLS

#define QLENGTH 100   // The length of packet queue.
#define WINDOWSIZE 5  // The size of sliding window.
#define FILENAMELEN 30

class TCPClient : public Element
{
public:
	TCPClient();
	~TCPClient();
	const char *class_name() const { return "TCPClient"; }
	const char *port_count() const { return "1-/1-"; }
	const char *processing() const { return PUSH; }
	int configure(Vector<String> &conf, ErrorHandler *errh);

	void run_timer(Timer*);
	void push(int port, Packet *packet);
	int initialize(ErrorHandler*);
	void Break();

private:
	FILE* log;
	String s_filename;
	char c_filename[FILENAMELEN];

	uint32_t _delay;
	Timer _timerTO;
	uint32_t _period;
	Timer _timerRETR;
	uint32_t _periodRetr;

	uint32_t _seq;
	uint32_t _my_address;
	uint32_t _other_address;

	WritablePacket* DataQueue[QLENGTH];
	int transmissions[WINDOWSIZE];  // The retransmitting times of a packet.
	uint32_t used_Qlen;             // The number of packets in the queue.
	uint32_t old_seq, new_seq;      // The flags controlling retransmission.
	int LAR;  // Last ack received.
	int LFS;  // Last frame sent.
	int LFE;  // Last frame enqueued.
	uint32_t start;   // Start=0 before finishing Three-Way Handshake.
	uint32_t empty;	  // Empty=0 means the queue is empty.
	uint32_t retranFlag;  // RetranFlag=0 means no retransmission after handshake.
};

CLICK_ENDDECLS
#endif 
