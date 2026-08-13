#pragma once

#include <string>
#include <vector>

namespace Ignition
{
	namespace Utilities
	{
		class CoreUtilities
		{
		public:
			static std::vector<char> ReadBinaryFile(const std::string& filepath);
		};
	}
}