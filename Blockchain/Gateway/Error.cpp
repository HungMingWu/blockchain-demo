#include "Error.h"

#include "error.h"

namespace gateway
{
	namespace detail
	{
		// Define a custom error code category derived from std::error_category
		class GatewayErrc_category : public std::error_category
		{
		public:
			// Return a short descriptive name for the category
			virtual const char *name() const noexcept override final { return "ConversionError"; }
			// Return what each enum means in text
			virtual std::string message(int c) const override final
			{
				switch (static_cast<GatewayErrc>(c))
				{
				case GatewayErrc::Success:
					return "operation successful";
				case GatewayErrc::SelfAddress:
					return "converting empty string";
				default:
					return "unknown";
				}
			}
		};
	}

	const detail::GatewayErrc_category &GatewayErrc_category()
	{
		static detail::GatewayErrc_category c;
		return c;
	}


	// Overload the global make_error_code() free function with our
	// custom enum. It will be found via ADL by the compiler if needed.
	std::error_code make_error_code(GatewayErrc e)
	{
		return { static_cast<int>(e), GatewayErrc_category() };
	}
}
