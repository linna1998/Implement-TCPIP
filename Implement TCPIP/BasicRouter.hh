#ifndef CLICK_BASICROUTER_HH 
#define CLICK_BASICROUTER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>

CLICK_DECLS
#define MAX_NODES 100
class BasicRouter : public Element 
{
public:
	BasicRouter();
	~BasicRouter();
	const char *class_name() const { return "BasicRouter"; }
	const char *port_count() const { return "1-/1-"; }
	const char *processing() const { return PUSH; }

	void push(int port, Packet *packet);
	int initialize(ErrorHandler*);

	int shortest_path(int s, int t, int n);
	void Dijkstra(int n);

private:
	HashTable<int, int> _ports_table;
	int Distance[MAX_NODES][MAX_NODES];
	int id;  // The id number of the router.
};

CLICK_ENDDECLS
#endif 
