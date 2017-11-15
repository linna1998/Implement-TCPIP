#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "UserClient.hh" 

CLICK_DECLS

UserClient::UserClient() : _timerTO(this)
{
	_period = 1;
	_delay = 0;
}
UserClient::~UserClient() {}

int UserClient::initialize(ErrorHandler *errh)
{
	_timerTO.initialize(this);
	if (_delay > 0)
	{
		_timerTO.schedule_after_sec(_delay);
	}

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
		click_chatter("User: Open user log file failed!");
	}
	else
	{
		fprintf(log, "%s", "----------User client log----------\n");
	}
	return 0;
}

int UserClient::configure(Vector<String> &conf, ErrorHandler *errh)
{
	if (cp_va_kparse(conf, this, errh,
		"DELAY", cpkP, cpUnsigned, &_delay,
		"PERIOD", cpkP, cpUnsigned, &_period,
		"FILENAME", cpkP, cpString, &s_filename,
		cpEnd) < 0)
	{
		return -1;
	}
	return 0;
}

void UserClient::run_timer(Timer *timer)
{
	// User Client only send the data to the TCP Client.
	if (timer == &_timerTO)
	{
		WritablePacket *packet = Packet::make(0, 0, 5, 0);
		memset(packet->data(), 0, packet->length());
		char *data = (char*)(packet->data());
		memcpy(data, "hello", 5);
		output(0).push(packet);
		click_chatter("User: sent data.");
		fprintf(log, "%s", "User sent data.\n");
		_timerTO.schedule_after_sec(_period);
	}
	else
	{
		assert(false);
	}
}

void UserClient::push(int port, Packet *packet)
{
	assert(packet);
	if (port == 0)
	{
		click_chatter("User: received data.");
		fprintf(log, "%s", "User received data.\n");
	}
	else
	{
		packet->kill();
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(UserClient)
