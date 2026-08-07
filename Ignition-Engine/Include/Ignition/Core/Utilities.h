#pragma once

#include "Ignition/Core/Export.h"

#include <string>
#include <vector>

namespace Ignition
{
	namespace Utilities
	{
		class IGNITION_API CoreUtilities
		{
		public:
			static std::vector<char> ReadBinaryFile(const std::string& filepath);
		};
	}
}