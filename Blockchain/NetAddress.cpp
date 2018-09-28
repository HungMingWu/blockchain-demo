#include "NetAddress.h"

std::string getHost(const boost::asio::ip::address& addr)
{
	return addr.to_string();
}