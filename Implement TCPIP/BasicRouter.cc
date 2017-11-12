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
	return 0;
}

int BasicRouter::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
		"ID", cpkP + cpkM, cpUnsigned, &id,
		"PORT_NUMBER", cpkP + cpkM, cpUnsigned, &portNumber,
		"PERIOD_EDGE", cpkP, cpUnsigned, &_periodEdge,
		"PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
		cpEnd) < 0)
	{
		return -1;
	}

	/*if (id == 2)
	{
		click_chatter("Set forwarding table of node %d", id);
		_forwarding_table.set(1, 1);
		_forwarding_table.set(2, 0);
		_forwarding_table.set(3, 3);
		_forwarding_table.set(4, 3);
		_forwarding_table.set(5, 5);
		_forwarding_table.set(6, 5);
	}
	else if (id == 3)
	{
		click_chatter("Set forwarding table of node %d", id);
		_forwarding_table.set(1, 2);
		_forwarding_table.set(2, 2);
		_forwarding_table.set(3, 0);
		_forwarding_table.set(4, 4);
		_forwarding_table.set(5, 2);
		_forwarding_table.set(6, 2);
	}
	else if (id == 5)
	{
		click_chatter("Set forwarding table of node %d", id);
		_forwarding_table.set(1, 2);
		_forwarding_table.set(2, 2);
		_forwarding_table.set(3, 2);
		_forwarding_table.set(4, 2);
		_forwarding_table.set(5, 0);
		_forwarding_table.set(6, 6);
	}*/

	return 0;
}

void BasicRouter::run_timer(Timer *timer)
{
	if (timer == &_timerHello)
	{
		// !!!
		for (int i = 0; i < portNumber; ++i)
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
		// !!!
		for (int i = 0; i < portNumber; ++i)
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
			//click_chatter("MAX_NODES : %d", MAX_NODES);
			for (int j = 0; j < MAX_NODES; j++)
			{
				//click_chatter("j : %d", j);
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
				//click_chatter("click_chatter( %c , data);");
				//click_chatter(" %c", data);
			}
			//click_chatter("Fill the count part with count: %d", count);
			for (int j = count; j < MAX_NODES; ++j)
			{
				//click_chatter("Before memset.");
				memset(data + 8 * j, '0', 8);
				//click_chatter("After memset.");
				// click_chatter("%c", *(data + j));
			}
			//click_chatter("Finish the memset part");

			click_chatter("Sending new Edge packet on port %d", i);
			//click_chatter("Before print data.");
			//for (int j = 0; j < length; j++)
			//{
				//click_chatter("%c", *(data + j));
			//}
			//click_chatter("After print data.");
			output(i).push(packet);
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
			//click_chatter("Enter circle %d", i);
			int col = 0;
			char s[9];
			//click_chatter("Before Read data");
			for (int j = 0; j < 8; ++j)
			{
				s[j] = *(data + 8 * i + j);
			}
			//click_chatter("After Read data");
			//click_chatter("Before compute col");
			int k = 7;
			while (s[k] == '0'&& k > 0) k--;
			//click_chatter("Print k %d", k);
			if ((k == 0 && s[k] != '0') || (k > 0))
			{
				for (int j = (int)k; j >= 0; j--)
				{
					col = col * 10 + s[j] - '0';
					//click_chatter("Print col %d", col);
				}
			}
			//click_chatter("After compute col");
			//if(col==0)continue;
			//if (col == 0)break;
			if (col == 0)break;
			//int col=atoi(data+8*i);
			//click_chatter("Set Distance %d to %d", row, col);
			Distance[row][col] = 1;
			Distance[col][row] = 1;
		}
		//forward hello packet
		if ((row == id) || (header->ttl < 1))
			packet->kill();
		else
		{
			header->ttl -= 1;
			click_chatter("Flood the pcket to all other ports.");
			// Flood the pcket to all other ports.
			// !!!
			for (int i = 0; i < portNumber; ++i)
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

void BasicRouter::Dijkstra()
{
	click_chatter("Doing Dijkstra Algorithm.");
	/*
	for (int i = 1; i <= MAX_NODES; i++)
	{
		for (int j = 1; j <= MAX_NODES; j++)
		{
			click_chatter("Distance from %d to %d is %d", i, j, Distance[i][j]);
		}
	}
	*/
	// Dijkstra
	int s = id;
	bool visit[MAX_NODES + 1] = { 0 };
	int dist[MAX_NODES + 1] = { 0 }, prev[MAX_NODES + 1] = { 0 };
	for (int i = 1; i <= MAX_NODES; ++i) //initialize
	{
		if (Distance[s][i] != MY_INFINITY)
		{
			dist[i] = Distance[s][i];
			prev[i] = s;
		}
		else
			dist[i] = MY_INFINITY;
	}
	visit[s] = true; prev[s] = -1;
	dist[s] = 0;
	int replace = s, replaceD = MY_INFINITY, N = MAX_NODES - 1;
	while (N > 0)//run
	{
		N--;
		replaceD = MY_INFINITY;
		for (int i = 1; i <= MAX_NODES; ++i)
		{
			if (!visit[i])
			{
				if (dist[i] < replaceD)
				{
					replaceD = dist[i];
					replace = i;
				}
			}
		}
		visit[replace] = true;
		for (int i = 1; i <= MAX_NODES; ++i)
		{
			if (!visit[i])
			{
				if (dist[i] > replaceD + Distance[replace][i])
				{
					dist[i] = replaceD + Distance[replace][i];
					prev[i] = replace;
				}
			}
		}
	}
	for (int i = 1; i <= MAX_NODES; i++)  // Update
	{
		//click_chatter("Update the forwarding table to node %d", i);
		if (i == id)  // Throw this packet away.
		{
			_forwarding_table.set(id, 0);
			continue;
		}
		int tmp = i;
		while (prev[tmp] != s)
		{
			if (prev[tmp] == 0)
			{
				_forwarding_table.set(i, 0);
				break;
			}
			tmp = prev[tmp];
		}
		if (prev[tmp] == s)
		{
			_forwarding_table.set(i, tmp);
			//click_chatter("Update node %d's forwarding table with next_node %d", i, tmp);
		}
	}
	//_forwarding_table.set(1, 1);
	//_forwarding_table.set(2, 2);
	//_forwarding_table.set(3, 3);
	//_forwarding_table.set(4, 0);
	return;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicRouter)
