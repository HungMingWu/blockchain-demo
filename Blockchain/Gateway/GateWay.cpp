#include <vector>
#include "GateWay.h"
#include "Error.h"
#include "const.h"
#include "../NetAddress.h"

namespace std
{
	// Tell the C++ 11 STL metaprogramming that enum ConversionErrc
	// is registered with the standard error code system
	template <> struct is_error_code_enum<gateway::GatewayErrc> : std::true_type
	{
	};
}
namespace gateway {
	std::error_code Gateway::managedConnect(const boost::asio::ip::address &addr)
	{
		auto selfaddr = Address();
		if (selfaddr == addr) {
			return GatewayErrc::SelfAddress;
		}
		return GatewayErrc::Success;
	}

	std::error_code Gateway::Connect(const boost::asio::ip::address &addr)
	{
		return managedConnect(addr);
	}

	void Gateway::addPeer(Peer *p)
	{
		peers[p->NetAddress] = p;
	}

	void Gateway::acceptPeer(Peer *p)
	{
		boost::shared_lock<boost::shared_mutex> lock(mutex);
		// If we are not fully connected, add the peer without kicking any out.
		if (peers.size() < fullyConnectedThreshold) {
			addPeer(p);
			return;
		}

		// Select a peer to kick. Outbound peers and local peers are not
		// available to be kicked.
		std::vector<IpAddr> addrs;
		for (const auto &pairs : peers) {
			const auto &addr = pairs.first;
			const auto &peer = pairs.second;
			// Do not kick outbound peers or local peers.
			if (!peer->Inbound || peer->Local) {
				continue;
			}

			// Prefer kicking a peer with the same hostname.
			if (getHost(addr) == getHost(p->NetAddress)) {
				addrs = { addr };
				break;
			}
			addrs.push_back(addr);	
		}

		if (addrs.empty()) {
			// There is nobody suitable to kick, therefore do not kick anyone.
			addPeer(p);
			return;
		}

#if 0
		// Of the remaining options, select one at random.
		kick: = addrs[fastrand.Intn(len(addrs))]

		g.peers[kick].sess.Close()
		delete(g.peers, kick)
		g.log.Printf("INFO: disconnected from %v to make room for %v\n", kick, p.NetAddress)
		addPeer(p)
#endif
	}
}