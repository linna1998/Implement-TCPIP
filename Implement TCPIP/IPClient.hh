#ifndef CLICK_IPCLIENT_HH 
#define CLICK_IPCLIENT_HH 
#include <click/element.hh>
#include <click/timer.hh>

CLICK_DECLS
#define FILENAMELEN 30
class IPClient : public Element
{
public:
	IPClient();
	~IPClient();
	const char *class_name() const { return "IPClient"; }
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

	Timer _timerHello;
	uint32_t _periodHello;
	uint32_t _my_address;
	uint32_t _other_address;
};

CLICK_ENDDECLS
#endif 
