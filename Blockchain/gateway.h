#pragma once
#include <system_error>
#include <boost/asio/ip/address.hpp>
class IGateway {
public:
	virtual ~IGateway() = default;
	virtual std::error_code Connect(const boost::asio::ip::address &) = 0;
	virtual boost::asio::ip::address Address() const = 0;
};