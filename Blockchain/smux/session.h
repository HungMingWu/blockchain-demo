#pragma once
#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/fiber/all.hpp>
#include "../outcome.hpp"
namespace outcome = OUTCOME_V2_NAMESPACE;
namespace smux {
	using result = outcome::result<size_t>;
	struct Frame {

	};

	struct writeRequest {
		Frame frame;
		boost::fibers::promise<result> promise;
	};

	class Session {
		std::shared_ptr<boost::asio::ip::tcp> socket;
		boost::fibers::unbuffered_channel<writeRequest> write_channel;
		void sendLoop();
	public:
		Session(std::shared_ptr<boost::asio::ip::tcp> socket_);
		result writeFrame(const Frame& f);
	};
}