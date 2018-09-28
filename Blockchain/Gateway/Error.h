#pragma once
#include <system_error>
namespace gateway {
	enum class GatewayErrc
	{
		Success = 0, // 0 should not represent an error
		SelfAddress = 1,
	};
	std::error_code make_error_code(GatewayErrc e);
}