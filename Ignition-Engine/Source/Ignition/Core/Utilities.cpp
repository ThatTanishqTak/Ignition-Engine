#include "Ignition/Core/Utilities.h"

#include "Ignition/Core/Log.h"

#include <fstream>
#include <ios>

namespace Ignition
{
	namespace Utilities
	{
		std::vector<char> CoreUtilities::ReadBinaryFile(const std::string& filepath)
		{
			std::ifstream file(filepath, std::ios::ate | std::ios::binary);
			if (!file.is_open())
			{
				IG_CORE_ERROR("Failed to open file: {}", filepath);

				return {};
			}

			const std::streamsize size = file.tellg();
			if (size <= 0)
			{
				IG_CORE_ERROR("File is empty or unreadable: {}", filepath);

				return {};
			}

			std::vector<char> buffer(static_cast<size_t>(size));

			file.seekg(0);
			file.read(buffer.data(), size);

			return buffer;
		}
	}
}