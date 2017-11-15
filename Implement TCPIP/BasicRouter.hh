#ifndef CLICK_BASICROUTER_HH 
#define CLICK_BASICROUTER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <string.h>
#include <stdio.h>

CLICK_DECLS
#define MAX_NODES 10
#define MY_INFINITY 100000      // A number larger than every maximum path.
#define FILENAMELEN 30
class BasicRouter : public Element
{
public:
	BasicRouter();
	~BasicRouter();
	const char *class_name() const { return "BasicRouter"; }
	const char *port_count() const { return "1-/1-"; }
	const char *processing() const { return PUSH; }
	int configure(Vector<String> &conf, ErrorHandler *errh);

	void run_timer(Timer*);
	void push(int port, Packet *packet);
	int initialize(ErrorHandler*);
	void Dijkstra();

private:
	FILE* log;
	String s_filename;
	char c_filename[FILENAMELEN];

	Timer _timerHello;
	uint32_t _periodHello;
	Timer _timerEdge;
	uint32_t _periodEdge;
	Timer _timerUpdate;
	uint32_t _periodUpdate;
	HashTable<int, int> _forwarding_table;       // id to id.
	HashTable<int, int> _ports_table;            // id to port.
	HashTable<int, bool> _neighbours_table;
	int Distance[MAX_NODES + 1][MAX_NODES + 1];
	int id;                                      // The id number of the router.
	int portNumber;                              // The number of the ports.
};

CLICK_ENDDECLS
#endif 
