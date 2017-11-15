#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "BasicRouter.hh" 
#include "IPPackets.hh"
#include "TCPPackets.hh"

CLICK_DECLS
BasicRouter::BasicRouter() : _timerHello(this), _timerEdge(this), _timerUpdate(this)
{
	_periodHello = 2;
	_periodEdge = 2;
	_periodUpdate = 1000;

	// Initialize Distance.
	for (int i = 1; i <= MAX_NODES; ++i)
	{
		for (int j = 1; j <= MAX_NODES; ++j)
		{
			if (i == j) Distance[i][j] = 0;
			else Distance[i][j] = MY_INFINITY;
		}
	}

	// Initialize _neighbours_table.
	for (int i = 1; i <= MAX_NODES; ++i)
		_neighbours_table.set(i, false);

	// Initialize _forwarding_table.
	for (int i = 1; i <= MAX_NODES; ++i)
		_neighbours_table.set(i, 0);
}

BasicRouter::~BasicRouter() {}

int BasicRouter::initialize(ErrorHandler *errh)
{
	_timerHello.initialize(this);
	_timerHello.schedule_after_sec(_periodHello);
	_timerEdge.initialize(this);
	_timerEdge.schedule_after_sec(_periodEdge);
	_timerUpdate.initialize(this);
	_timerUpdate.schedule_after_sec(_periodUpdate);

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
		click_chatter("Open router log file failed!");
	}
	else
	{
		fprintf(log, "%s", "----------Router log----------\n");
	}
	return 0;
}

int BasicRouter::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
		"ID", cpkP + cpkM, cpUnsigned, &id,
		"PORT_NUMBER", cpkP + cpkM, cpUnsigned, &portNumber,
		"PERIOD_EDGE", cpkP, cpUnsigned, &_periodEdge,
		"PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
		"PERIOD_UPDATE", cpkP, cpUnsigned, &_periodUpdate,
		"FILENAME", cpkP, cpString, &s_filename,
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
		for (int i = 0; i < portNumber; ++i)
		{
			WritablePacket *packet = Packet::make(0, 0, sizeof(struct IPPacketHeader), 0);
			memset(packet->data(), 0, packet->length());
			struct IPPacketHeader *format = (struct IPPacketHeader*) packet->data();
			format->type = HELLO;
			format->source = id;
			format->destination = 0;
			format->size = sizeof(struct IPPacketHeader);
			output(i).push(packet);
		}
		_timerHello.schedule_after_sec(_periodHello);
	}
	else if (timer == &_timerEdge)
	{
		// Broadcast to neighbours.
		for (int i = 0; i < portNumber; ++i)
		{
			int length = 8 * MAX_NODES;
			WritablePacket *packet = Packet::make(0, 0, sizeof(struct IPPacketHeader) + length, 0);
			memset(packet->data(), 0, packet->length());
			struct IPPacketHeader *format = (struct IPPacketHeader*) packet->data();
			format->type = EDGE;
			format->source = id;
			format->destination = 0;      // Broadcast.
			format->ttl = MAX_NODES + 1;  // Avoid routing circle.
			format->size = sizeof(struct IPPacketHeader) + length;
			char *data = (char*)(packet->data() + sizeof(struct IPPacketHeader));
			// Fill the EDGE packet.
			int count = 0;
			for (int j = 0; j < MAX_NODES; j++)
			{
				bool ifexist = _neighbours_table.get(j);
				char s[9];
				if (ifexist == false)
				{
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
				{
					s[k] = '0';
				}
				memcpy(data + 8 * count, s, 8);
				count++;
			}
			for (int j = count; j < MAX_NODES; ++j)
			{
				memset(data + 8 * j, '0', 8);
			}

			output(i).push(packet);
		}
		_timerEdge.schedule_after_sec(_periodEdge);
	}
	else if (timer == &_timerUpdate)  // Delete IP.
	{
		// Initialize Distance.
		for (int i = 1; i <= MAX_NODES; ++i)
		{
			for (int j = 1; j <= MAX_NODES; ++j)
			{
				if (i == j) Distance[i][j] = 0;
				else Distance[i][j] = MY_INFINITY;
			}
		}

		// Initialize _neighbours_table.
		for (int i = 1; i <= MAX_NODES; ++i)
			_neighbours_table.set(i, false);

		// Initialize _forwarding_table.
		for (int i = 1; i <= MAX_NODES; ++i)
			_neighbours_table.set(i, 0);

		_timerHello.schedule_after_sec(_periodUpdate);
	}
	else
	{
		assert(false);
	}
}

void BasicRouter::push(int port, Packet *packet)
{
	assert(packet);
	struct IPPacketHeader *header = (struct IPPacketHeader *)packet->data();
	if (header->type == DATA || header->type == ACK)
	{
		int next_node = _forwarding_table.get(header->destination);
		if (next_node == 0)// Can't get to the point
		{
			packet->kill();
		}
		else
		{
			int next_port = _ports_table.get(next_node);
			output(next_port).push(packet);
			if (header->type == DATA)
			{
				click_chatter("Router: Received Data from %u with destination %u, next_node %d, next_port %d.",
					header->source, header->destination, next_node, next_port);
				fprintf(log, "Received Data from %u with destination %u, next_node %d, next_port %d.\n",
					header->source, header->destination, next_node, next_port);
			}
			else if (header->type == ACK)
			{
				click_chatter("Router: Received Ack from %u with destination %u, next_node %d, next_port %d.",
					header->source, header->destination, next_node, next_port);
				fprintf(log, "Received Ack from %u with destination %u, next_node %d, next_port %d.\n",
					header->source, header->destination, next_node, next_port);
			}
		}
	}
	else if (header->type == EDGE)
	{
		// Update Distance.
		char *data = (char*)(packet->data() + sizeof(struct IPPacketHeader));
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
			while (s[k] == '0'&& k > 0) k--;
			if ((k == 0 && s[k] != '0') || (k > 0))
			{
				for (int j = (int)k; j >= 0; j--)
				{
					col = col * 10 + s[j] - '0';
				}
			}
			if (col == 0)break;
			Distance[row][col] = 1;
			Distance[col][row] = 1;
		}
		// Forward edge packet.
		if ((row == id) || (header->ttl < 1))
			packet->kill();
		else
		{
			header->ttl -= 1;
			// Flood the pcket to all other ports.
			for (int i = 0; i < portNumber; ++i)
			{
				if (i == port)
					continue;
				int length = 8 * MAX_NODES;
				WritablePacket *new_packet = Packet::make(0, 0, sizeof(struct IPPacketHeader) + length, 0);
				memcpy(new_packet->data(), packet->data(), new_packet->length());

				output(i).push(new_packet);
			}
		}
		Dijkstra();
	}
	else if (header->type == HELLO)
	{
		_ports_table.set(header->source, port);
		_neighbours_table.set(header->source, true);
		Distance[header->source][id] = 1;
		Distance[id][header->source] = 1;  // Update the Distance matrix.
		Dijkstra();
		packet->kill();
	}
	else
	{
		// Wrong packet type.
		packet->kill();
	}
}

void BasicRouter::Dijkstra()
{
	// Dijkstra.
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
	visit[s] = true;
	prev[s] = -1;
	dist[s] = 0;
	int replace = s, replaceD = MY_INFINITY, N = MAX_NODES - 1;
	while (N > 0)  // Run.
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
	for (int i = 1; i <= MAX_NODES; i++)  // Update.
	{
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
		}
	}
	return;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicRouter)
