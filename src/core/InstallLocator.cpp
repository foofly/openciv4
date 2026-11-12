#include "core/InstallLocator.h"

#include <fstream>
#include <regex>
#include <vector>

namespace oc4
{
	std::filesystem::path findLinuxSteamInstall(const std::filesystem::path& home)
	{
		namespace fs = std::filesystem;
		if (home.empty())
			return {};
		const std::vector<fs::path> roots = { home / ".local/share/Steam", home / ".steam/steam", home / ".steam/root",
			home / ".var/app/com.valvesoftware.Steam/.local/share/Steam" };
		std::vector<fs::path> libraries = roots;
		const std::regex pathLine(R"vdf("path"\s*"((?:\\.|[^"\\])*)")vdf");
		for (const auto& root : roots)
		{
			std::ifstream file(root / "steamapps/libraryfolders.vdf");
			for (std::string line; std::getline(file, line);)
			{
				std::smatch match;
				if (!std::regex_search(line, match, pathLine))
					continue;
				std::string path = match[1], unescaped;
				for (size_t i = 0; i < path.size(); ++i)
				{
					if (path[i] == '\\' && i + 1 < path.size())
						++i;
					unescaped += path[i];
				}
				libraries.push_back(fs::u8path(unescaped));
			}
		}
		for (const auto& library : libraries)
		{
			const auto candidate = library / "steamapps/common/Sid Meier's Civilization IV Beyond the Sword";
			std::error_code ec;
			if (fs::is_directory(candidate / "Assets", ec) && fs::is_directory(candidate / "Beyond the Sword/Assets", ec))
				return candidate;
		}
		return {};
	}
}
