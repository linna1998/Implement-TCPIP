#ifndef CLICK_USERCLIENT_HH 
#define CLICK_USERCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <string.h>
#include <stdio.h>

CLICK_DECLS
#define FILENAMELEN 30
class UserClient : public Element
{
public:
	UserClient();
	~UserClient();
	const char *class_name() const { return "UserClient"; }
	const char *port_count() const { return "1-/1-"; }
	const char *processing() const { return PUSH; }
	int configure(Vector<String> &conf, ErrorHandler *errh);

	void run_timer(Timer*);
	void push(int port, Packet *packet);
	int initialize(ErrorHandler*);

private:
	FILE* log;
	String s_filename;
	char c_filename[FILENAMELEN];

	Timer _timerTO;
	uint32_t _delay;
	uint32_t _period;
};

CLICK_ENDDECLS
#endif 
