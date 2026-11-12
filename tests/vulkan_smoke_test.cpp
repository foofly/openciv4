#undef NDEBUG
#include "core/AssetResolver.h"
#include "render/MapView.h"
#include "render/Overlay.h"
#include <SDL3/SDL_log.h>

#include <atomic>
#include <cassert>
#include <cstdio>
#include <exception>

int main()
{
	std::atomic<int> errors = 0;
	SDL_LogOutputFunction previous;
	void* previousData = nullptr;
	SDL_GetLogOutputFunction(&previous, &previousData);
	SDL_SetLogOutputFunction([](void* userdata, int, SDL_LogPriority priority, const char* message) {
		if (priority >= SDL_LOG_PRIORITY_ERROR)
			++*static_cast<std::atomic<int>*>(userdata);
		std::fprintf(stderr, "%s\n", message);
	}, &errors);
	try
	{
		oc4::AssetResolver assets;
		oc4::render::Renderer renderer(nullptr, 64, 64);
		// Constructing both creates all six shader stages and the terrain pipeline.
		oc4::render::MapView map(renderer, assets);
		oc4::render::MapSnapshot snapshot;
		snapshot.width = snapshot.height = 2;
		snapshot.plotSize = 1;
		snapshot.terrains.resize(1);
		snapshot.terrains[0].slots[15].push_back({ 1, 0 });
		snapshot.plots.resize(4);
		for (auto& plot : snapshot.plots)
			plot.terrain = 0;
		map.setMap(snapshot);
		oc4::render::Camera camera;
		camera.target = { 1, 1, 0 };
		camera.distance = 4;
		renderer.beginFrame(camera);
		map.draw(); // includes one-layer array textures and the terrain storage buffer
		renderer.endFrame();
		renderer.present();
		oc4::render::Overlay overlay(renderer);
		renderer.beginFrame({}, { 0, 0, 0 });
		renderer.endFrame();
		assert(overlay.begin());
		overlay.rect(0, 0, 32, 32, { 1, 0, 0 }, 1);
		overlay.rect(32, 32, 64, 64, { 0, 0, 1 }, 1);
		overlay.end();
		uint32_t width = 0, height = 0;
		const auto pixels = renderer.readback(width, height);
		assert(width == 64 && height == 64 && pixels.size() == 64 * 64 * 4);
		const auto channel = [&](int x, int y, int c) { return pixels[(y * 64 + x) * 4 + c]; };
		// BGRA readback, origin at top left: catches shader binding and Y inversion.
		assert(channel(16, 16, 2) > 240 && channel(16, 16, 0) < 10);
		assert(channel(48, 48, 0) > 240 && channel(48, 48, 2) < 10);
		assert(channel(48, 16, 0) < 10 && channel(48, 16, 2) < 10);
		std::puts("Vulkan shaders, terrain pipeline and overlay readback passed");
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "%s\n", error.what());
		SDL_SetLogOutputFunction(previous, previousData);
		return 1;
	}
	SDL_SetLogOutputFunction(previous, previousData);
	return errors == 0 ? 0 : 1;
}
