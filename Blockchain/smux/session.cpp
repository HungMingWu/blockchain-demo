#include "session.h"

namespace smux {
	void Session::sendLoop()
	{
		while (true) {
			writeRequest req = write_channel.value_pop();
		}
	}

	Session::Session(std::shared_ptr<boost::asio::ip::tcp> socket_) 
		: socket(socket_)
	{
		boost::fibers::fiber([this] {
			sendLoop();
		}).detach();
	}

	result Session::writeFrame(const Frame& f)
	{
		writeRequest req { f };
		auto future(req.promise.get_future());
		return future.get();
	}
}