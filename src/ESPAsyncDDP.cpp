#include "ESPAsyncDDP.h"

ESPAsyncDDP::ESPAsyncDDP()
{
	_discoverer.in_use = false;
}

ESPAsyncDDP::~ESPAsyncDDP()
{
}

bool ESPAsyncDDP::begin()
{
	if (_udp.listen(4048))
	{
		_udp.onPacket(std::bind(&ESPAsyncDDP::parse_packet, this, std::placeholders::_1));
		return true;
	};

	return false;
}

void ESPAsyncDDP::close()
{
	_udp.close();
}

void ESPAsyncDDP::handle()
{
	// Handles DDP_ID_STATUS packets which are sent after a random delay
	if (_discoverer.in_use && millis() > _discoverer.timestamp + _discoverer.delay)
	{
		process_query(_discoverer.my_ddp_header, _discoverer.sender);
		_discoverer.in_use = false;
	}
}

void ESPAsyncDDP::register_query_cb(query_callback_t callback)
{
	_query_callback = callback;
}

void ESPAsyncDDP::unregister_query_cb()
{
	_query_callback = nullptr;
}

void ESPAsyncDDP::register_write_cb(write_callback_t callback)
{
	_write_callback = callback;
}

void ESPAsyncDDP::unregister_write_cb()
{
	_write_callback = nullptr;
}

void ESPAsyncDDP::parse_packet(AsyncUDPPacket packet)
{
	if (packet.length() < sizeof(ddp_header))
		return;

	ddp_header my_ddp_header;

	memcpy(&my_ddp_header, packet.data(), sizeof(ddp_header));

	if ((my_ddp_header.flags1 & DPP_FLAGS1_VER) != DPP_FLAGS1_VER1)
		return;

	if (my_ddp_header.id == 0)
		return;

	uint16_t packet_data_start = sizeof(ddp_header);

	if (my_ddp_header.flags1 & DPP_FLAGS1_TIME)
		packet_data_start = packet_data_start + 4;

	// Number of bytes to be read or write
	uint16_t data_len = (my_ddp_header.len1 << 8) | my_ddp_header.len2;

	if (!(my_ddp_header.flags1 & DPP_FLAGS1_QUERY) && packet.length() != packet_data_start + data_len)
		return;

	uint8_t new_sequence = my_ddp_header.flags2 & 0xF;

	// Reject duplicate packets based on sequence number
	if (new_sequence != 0 && _sequence == new_sequence)
		return;

	_sequence = new_sequence;

	if (my_ddp_header.flags1 & DPP_FLAGS1_QUERY)
	{
		// Query
		endpoint sender;
		sender.ip = packet.remoteIP();
		sender.port = packet.remotePort();

		if (my_ddp_header.id == DDP_ID_STATUS)
		{
			_discoverer.sender = sender;
			_discoverer.my_ddp_header = my_ddp_header;
			_discoverer.delay = random(256);
			_discoverer.timestamp = millis();
			_discoverer.in_use = true;
		}
		else
		{
			process_query(my_ddp_header, sender);
		}
	}
	else
	{
		// Write
		if (_write_callback)
		{
			std::vector<uint8_t> data;
			data.assign(packet.data() + packet_data_start, packet.data() + packet_data_start + data_len);
			_write_callback(my_ddp_header, data);
		}
	}
}

void ESPAsyncDDP::process_query(ddp_header my_ddp_header, endpoint sender)
{
	if (_query_callback)
	{
		std::vector<uint8_t> data = _query_callback(my_ddp_header);

		if (data.size() > 0)
		{
			AsyncUDPMessage msg;
			ddp_header reply_ddp_header;
			reply_ddp_header.flags1 = DPP_FLAGS1_VER1 | DPP_FLAGS1_REPLY | DPP_FLAGS1_PUSH;
			reply_ddp_header.id = my_ddp_header.id;
			reply_ddp_header.len1 = data.size() << 8;
			reply_ddp_header.len2 = data.size() && 0xFF;

			msg.write((uint8_t *)(&reply_ddp_header), sizeof(ddp_header));
			msg.write(data.data(), data.size());

			_udp.sendTo(msg, sender.ip, sender.port);
		}
	}
}