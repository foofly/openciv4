#undef NDEBUG
#include "core/AssetResolver.h"
#include "core/FileUtil.h"
#include "core/InstallLocator.h"

#include <cassert>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

static void write(const fs::path& path, const std::string& data)
{
	fs::create_directories(path.parent_path());
	std::ofstream(path, std::ios::binary) << data;
}

int main()
{
	const auto temporary = fs::temp_directory_path() / ("oc4-assets-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	const struct Cleanup { fs::path path; ~Cleanup() { fs::remove_all(path); } } cleanup{ temporary };
	const auto root = temporary / fs::u8path("Civ4 café");
	write(root / "Assets/Art/MiXeD.DDS", "base");
	write(root / "Warlords/Assets/XML/Test.XML", "warlords");
	write(root / "Beyond the Sword/Assets/Art/Mixed.dds", "bts");
	write(root / "Beyond the Sword/Assets/Python/CvMixed.py", "python");
	write(root / "Mod/Assets/aRT/MIXED.dds", "mod");
	{
		auto assets = oc4::AssetResolver::forInstall(root);
		const auto art = assets.locate("ASSETS\\ART\\mixed.dds");
		assert(art && art->file == root / "Beyond the Sword/Assets/Art/Mixed.dds");
		const auto bytes = assets.read(*art);
		assert(std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()) == "bts");
		assert(assets.locate("xml/test.xml")->file == root / "Warlords/Assets/XML/Test.XML");
		assert(assets.list(".DDS").size() == 1);
		assert(assets.list(".py").front().file == root / "Beyond the Sword/Assets/Python/CvMixed.py");
		assert(!assets.locate("art/missing.dds"));
		assets.addLayer(root / "Extra"); // a missing optional layer is harmless
		assert(!assets.locate("art/missing.dds"));
	}
	{
		auto assets = oc4::AssetResolver::forInstall(root, root / "Mod/Assets");
		assert(assets.locate("art/mixed.dds")->file == root / "Mod/Assets/aRT/MIXED.dds");
		assert(assets.list(".dds").size() == 1);
	}
	const auto home = temporary / "home";
	assert(oc4::findLinuxSteamInstall({}).empty());
	assert(oc4::findLinuxSteamInstall(home).empty());
	const auto library = temporary / fs::u8path("Steam library café");
	const auto steamGame = library / "steamapps/common/Sid Meier's Civilization IV Beyond the Sword";
	fs::create_directories(steamGame / "Assets");
	fs::create_directories(steamGame / "Beyond the Sword/Assets");
	write(home / ".local/share/Steam/steamapps/libraryfolders.vdf", "\"libraryfolders\"\n{\n\"0\"\n{\n\"path\" \"" + oc4::utf8Path(library) + "\"\n}\n}\n");
	assert(oc4::findLinuxSteamInstall(home) == steamGame);
	fs::remove_all(home / ".local");
	const auto flatpakGame = home / ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common/Sid Meier's Civilization IV Beyond the Sword";
	fs::create_directories(flatpakGame / "Assets");
	fs::create_directories(flatpakGame / "Beyond the Sword/Assets");
	assert(oc4::findLinuxSteamInstall(home) == flatpakGame);
}
