#include <map>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "../gateway.h"
namespace gateway {
	struct Peer {
		bool Inbound;
		bool Local;
		boost::asio::ip::address NetAddress;
	};

	class Gateway : public IGateway {
		using IpAddr = boost::asio::ip::address;
		mutable boost::shared_mutex mutex;
		IpAddr myAddr;
		std::map<IpAddr, Peer *> peers;
		std::error_code managedConnect(const boost::asio::ip::address &addr);
		bool found(const IpAddr &addr) {
			boost::shared_lock<boost::shared_mutex> lock(mutex);
			auto it = peers.find(addr);
			return it != end(peers);
		}
	public:
		std::error_code Connect(const boost::asio::ip::address &) override;
		boost::asio::ip::address Address() const override {
			boost::shared_lock<boost::shared_mutex> lock(mutex);
			return myAddr;
		}
		void addPeer(Peer *);
		void acceptPeer(Peer *);
	};
}