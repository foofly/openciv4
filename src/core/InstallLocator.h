#pragma once
#include <filesystem>

namespace oc4
{
	// Search native and Flatpak Steam libraries beneath a Linux user's home.
	std::filesystem::path findLinuxSteamInstall(const std::filesystem::path& home);
}
