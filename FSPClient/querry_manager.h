#pragma once

class connection;

namespace fsp::protocol::message {
	struct message;
}

class querry_manager final
{
public:
	querry_manager(connection& c);

	void resolve_querry(fsp::protocol::message::message& querry);
private:
	connection& connection_;
};