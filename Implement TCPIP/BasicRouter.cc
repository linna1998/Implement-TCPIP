#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "BasicRouter.hh" 
#include "Packets.hh"

CLICK_DECLS
BasicRouter::BasicRouter() : _timerHello(this), _timerEdge(this)
{
	_periodHello = 2;
	_periodEdge = 2;
	// Initialize Distance
	for (int i = 1; i <= MAX_NODES; ++i)
	{
		for (int j = 1; j <= MAX_NODES; ++j)
		{
			if (i == j) Distance[i][j] = 0;
			else Distance[i][j] = MY_INFINITY;
		}
	}

	for (int i = 1; i <= MAX_NODES; ++i)
		_neighbours_table.set(i, false);
}

BasicRouter::~BasicRouter() {}

int BasicRouter::initialize(ErrorHandler *errh)
{
	_timerHello.initialize(this);
	_timerHello.schedule_after_sec(_periodHello);
	_timerEdge.initialize(this);
	_timerEdge.schedule_after_sec(_periodEdge);

	if (id == 2)
	{
		click_chatter("Set forwarding table of router %d", id);
		_forwarding_table.set(1, 1);
		_forwarding_table.set(2, 0);
		_forwarding_table.set(3, 3);
		_forwarding_table.set(4, 3);
		_forwarding_table.set(5, 5);
		_forwarding_table.set(6, 5);
	}
	else if (id == 3)
	{
		click_chatter("Set forwarding table of router %d", id);
		_forwarding_table.set(1, 2);
		_forwarding_table.set(2, 2);
		_forwarding_table.set(3, 0);
		_forwarding_table.set(4, 4);
		_forwarding_table.set(5, 2);
		_forwarding_table.set(6, 2);
	}
	else if (id == 5)
	{
		click_chatter("Set forwarding table of router %d", id);
		_forwarding_table.set(1, 2);
		_forwarding_table.set(2, 2);
		_forwarding_table.set(3, 2);
		_forwarding_table.set(4, 2);
		_forwarding_table.set(5, 0);
		_forwarding_table.set(6, 6);
	}

	return 0;
}

int BasicRouter::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
		"ID", cpkP + cpkM, cpUnsigned, &id,
		"PERIOD_EDGE", cpkP, cpUnsigned, &_periodEdge,
		"PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
		cpEnd) < 0)
	{
		return -1;
	}
	return 0;
}

void BasicRouter::run_timer(Timer *timer)
{
	if (timer == &_timerHello)
	{
		// 3 is the number of ports.
		for (int i = 0; i < 3; ++i)
		{
			WritablePacket *packet = Packet::make(0, 0, sizeof(struct PacketHeader), 0);
			memset(packet->data(), 0, packet->length());
			struct PacketHeader *format = (struct PacketHeader*) packet->data();
			format->type = HELLO;
			format->source = id;
			format->destination = 0;
			format->size = sizeof(struct PacketHeader);
			output(i).push(packet);
			click_chatter("Flooding on port %d", i);
		}
		_timerHello.schedule_after_sec(_periodHello);
	}
	else if (timer == &_timerEdge)
	{
		// Broadcast to neighbours.
		// 3 is the number of ports.
		for (int i = 0; i < 3; ++i)
		{
			int length = 8 * MAX_NODES;
			WritablePacket *packet = Packet::make(0, 0, sizeof(struct PacketHeader) + length, 0);
			memset(packet->data(), 0, packet->length());
			struct PacketHeader *format = (struct PacketHeader*) packet->data();
			format->type = EDGE;
			format->source = id;
			format->destination = 0;  // Broadcast.
			format->ttl = MAX_NODES + 1;  // Avoid routing circle.
			format->size = sizeof(struct PacketHeader) + length;
			char *data = (char*)(packet->data() + sizeof(struct PacketHeader));
			int count = 0;
			for (int j = 0; j < MAX_NODES; j++)
			{
				//if(Neighbours[j]==0)break;
				//itoa(Neighbours[j],data+8*j,10);  // 10 means the positional notation
				bool ifexist = _neighbours_table.get(j);
				char s[9];
				if (ifexist == false)
				{
					//memset(data+8*j,'0',8);
					continue;
				}
				int t = 0;
				int tmp = j;
				while (tmp != 0)
				{
					s[t++] = tmp % 10 + '0';
					tmp = tmp / 10;
				}
				for (int k = t; k < 9; ++k)
					s[k] = '0';
				memcpy(data + 8 * count, s, 8);
				count++;
			}
			for (int j = count; j < MAX_NODES; ++j)
			{
				memset(data + 8 * j, '0', 8);
			}
			output(i).push(packet);
			click_chatter("Sending new Edge packet on port %d", i);
		}
		_timerEdge.schedule_after_sec(_periodEdge);
	}
	else
	{
		assert(false);
	}
}

void BasicRouter::push(int port, Packet *packet)
{
	assert(packet);
	struct PacketHeader *header = (struct PacketHeader *)packet->data();
	if (header->type == DATA || header->type == ACK)
	{
		int next_node = _forwarding_table.get(header->destination);
		// Can't get to the point
		if (next_node == 0)
		{
			click_chatter("Received Data from %u with destination %u", header->source, header->destination);
			click_chatter("But can't find the next node in forwarding table, so packet is killed");
			packet->kill();
		}
		else
		{
			int next_port = _ports_table.get(next_node);
			output(next_port).push(packet);
			if (header->type == DATA)
				click_chatter("Received Data from %u with destination %u, next_node %d, next_port %d",
					header->source, header->destination, next_node, next_port);
			else if (header->type == ACK)
				click_chatter("Received Ack from %u with destination %u, next_node %d, next_port %d",
					header->source, header->destination, next_node, next_port);
		}
	}
	else if (header->type == EDGE)
	{
		click_chatter("Received Edge from %u on port %d", header->source, port);
		// Update Distance.
		char *data = (char*)(packet->data() + sizeof(struct PacketHeader));
		int row = header->source;
		for (int i = 0; i < MAX_NODES; ++i)
		{
			int col = 0;
			char s[9];
			for (int j = 0; j < 8; ++j)
			{
				s[j] = *(data + 8 * i + j);
			}
			int k = 7;
			while (s[k] == '0')
				k--;
			for (int j = k; j >= 0; --j)
			{
				col = col * 10 + s[j] - '0';
			}
			//if(col==0)continue;
			if (col == 0)break;
			//int col=atoi(data+8*i);
			Distance[row][col] = 1;
			Distance[col][row] = 1;
		}
		//forward hello packet
		if ((row == id) || (header->ttl < 1))
			packet->kill();
		else
		{
			header->ttl -= 1;
			// Flood the pcket to all other ports.
			for (int i = 0; i < 3; ++i)
			{
				if (i == port)
					continue;
				int length = 8 * MAX_NODES;
				WritablePacket *new_packet = Packet::make(0, 0, sizeof(struct PacketHeader) + length, 0);
				memcpy(new_packet->data(), packet->data(), new_packet->length());
				/* struct PacketHeader *format = (struct PacketHeader*) new_packet->data();
				format->type = EDGE;
				format->source = id;
				format->destination = 0;//broadcast
				format->ttl = MAX_NODES+1; //avoid circle
				format->size = sizeof(struct PacketHeader) + length;
				char *data = (char*)(new_packet->data() + sizeof(struct PacketHeader)); */
				output(i).push(new_packet);
				click_chatter("Flood Edge on port %d", i);
			}
		}
		Dijkstra();
	}
	else if (header->type == HELLO)
	{
		click_chatter("Received Hello from %u on port %d", header->source, port);
		_ports_table.set(header->source, port);
		_neighbours_table.set(header->source, true);
		Distance[header->source][id] = 1;
		Distance[id][header->source] = 1;  // Update the Distance matrix.
		Dijkstra();
		packet->kill();
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
	enum Label { permanent, tentative };  // Label state.
	/* the path being worked on */
	struct state
	{
		int predecessor;  // Previous node.
		int length;  // Length from source to this node.
		Label label;
	}state[MAX_NODES];
	int k;
	int min;
	struct state *p;
	// Initialize state.
	for (p = &state[0]; p < &state[n]; p++)
	{
		p->predecessor = -1;
		p->length = MY_INFINITY;
		p->label = tentative;
	}
	state[t].length = 0;
	state[t].label = permanent;
	int count = 0;
	k = t;
	/*k is the initial working node */
	do
	{
		count++;
		/* is  the better path from k? */
		for (int I =1; I <=n; I++)
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
		min = MY_INFINITY;
		for (int I = 1; I <= n; I++)
		{
			if (state[I].label == tentative && state[I].length < min)
			{
				min = state[I].length;
				k = I;
			}
		}
		state[k].label = permanent;
	} while (k != s && count <= MAX_NODES);
	click_chatter("Node %d to node %d's next node is %d", s, t, state[0].predecessor);
	return state[0].predecessor;
}

void BasicRouter::Dijkstra()
{
	click_chatter("Doing Dijkstra Algorithm.");
	/*for (int i = 1; i <= MAX_NODES; i++)
	{
		for (int j = 1; j <= MAX_NODES; j++)
		{
			click_chatter("Distance from %d to %d is %d", i, j, Distance[i][j]);
		}
	}
	for (int i = 1; i <= MAX_NODES; i++)
	{
		click_chatter("Update the forwarding table to node %d", i);
		int next_node = shortest_path(id, i, MAX_NODES);
		_forwarding_table.set(i, next_node);
		click_chatter("Update node %d's forwarding table with next_node %d", i, next_node);
	}*/
	
	//_forwarding_table.set(1, 1);
	//_forwarding_table.set(2, 2);
	//_forwarding_table.set(3, 3);

	return;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicRouter)
