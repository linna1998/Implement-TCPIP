#ifndef CLICK_BASICROUTER_HH 
#define CLICK_BASICROUTER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>

CLICK_DECLS
#define MAX_NODES 6
#define MY_INFINITY 100000      /* a number larger than every maximum path */
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

	int shortest_path(int s, int t, int n);
	void Dijkstra();

private:	
	Timer _timerHello;
	uint32_t _periodHello;
	Timer _timerEdge;
	uint32_t _periodEdge;
	HashTable<int, int> _forwarding_table;
	HashTable<int, int> _ports_table;
	HashTable<int, bool> _neighbours_table;
	int Distance[MAX_NODES + 1][MAX_NODES + 1];
	int id;  // The id number of the router
	int portNumber;  // The number of the ports.
	//int Neighbours[MAX_NODES];  // Neighbours
};

CLICK_ENDDECLS
#endif 
