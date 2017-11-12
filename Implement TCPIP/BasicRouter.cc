#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "BasicRouter.hh" 
#include "Packets.hh"

CLICK_DECLS

BasicRouter::BasicRouter() {}
BasicRouter::~BasicRouter() {}

int BasicRouter::initialize(ErrorHandler *errh)
{
	return 0;
}

void BasicRouter::push(int port, Packet *packet)
{
	assert(packet);
	struct PacketHeader *header = (struct PacketHeader *)packet->data();
	if (header->type == DATA || header->type == ACK)
	{
		click_chatter("Received Data from %u with destination %u", header->source, header->destination);
		int next_port = _ports_table.get(header->destination);
		output(next_port).push(packet);
	}
	else if (header->type == HELLO)
	{
		click_chatter("Received Hello from %u on port %d", header->source, port);

		_ports_table.set(header->source, port);
	}
	else
	{
		click_chatter("Wrong packet type");
		packet->kill();
	}
}

// Return the next node on the path from s to t.
int BasicRouter::shortest_path(int s, int t, int n)
// n = number of nodes
{
	enum Label { permanent, tentative };    /*label state*/
	struct state
	{                          /* the path being worked on */
		int predecessor;                     /*previous node */
		int length;                                /*length from source to this node*/
		Label label;
	}state[MAX_NODES];
	int MyInfinity = 100000;      /* a number larger than every maximum path */
	int k;
	int min;
	struct state *p;
	for (p = &state[0]; p < &state[n]; p++)
	{       /*initialize state*/
		p->predecessor = -1;
		p->length = MyInfinity;
		p->label = tentative;
	}
	state[t].length = 0;
	state[t].label = permanent;
	k = t;
	/*k is the initial working node */
	do
	{
		/* is  the better path from k? */
		for (int I = 0; I < n; I++)
		{
			/*this graph has n nodes */
			if (Distance[k][I] != 0 && state[I].label == tentative)
			{
				if (state[k].length + Distance[k][I] < state[I].length)
				{
					state[I].predecessor = k;
					state[I].length = state[k].length + Distance[k][I];
				}
			}
		}
		/* Find the tentatively labeled node with the smallest label. */
		k = 0;
		min = MyInfinity;
		for (int I = 0; I < n; I++)
		{
			if (state[I].label == tentative && state[I].length < min)
			{
				min = state[I].length;
				k = I;
			}
		}
		state[k].label = permanent;
	} while (k != s);
	return state[0].predecessor;
}

void BasicRouter::Dijkstra(int n)
{
	for (int i = 0; i < n; i++)
	{
		_ports_table.set(i, shortest_path(id, i, n));
	}
	return;
}


CLICK_ENDDECLS
EXPORT_ELEMENT(BasicRouter)
