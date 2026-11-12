# Linux port validation

Local environment: Fedora 44, x86-64, Clang 22.1.8, SDL 3.4.18 and Mesa software
Vulkan. The supplied GOG extraction reports `CIV4_VERSION=319`. Game assets,
screenshots, saves and installers are not committed.

## Verified

- All 11 CTest cases pass, including mixed-case/UTF-8 assets, Steam-library
  discovery, TCP accept/welcome/disconnect, POSIX formatting and Vulkan rendering.
- The Vulkan test draws synthetic terrain (including one-layer texture arrays),
  creates all six shader stages, and checks overlay colours and orientation through
  pixel readback. It passes under X11/Xvfb and Wayland with Khronos validation enabled.
- A 100-turn Fractal/Standard/seed-7 run repeats exactly. Saving at turn 50 and
  resuming reproduces every checksum through turn 100: final checksum `D7D43800`.
- The graphics and headless versions of the same three-turn run agree. Terrain,
  models, the menu and the original Python interface render using the supplied assets.
- Next War runs, saves and reloads. Both slash styles are accepted for its relative
  mod path. Rhye's and Fall and The Road to War receive the custom-core diagnostic.
- The audio scanner decodes 2,370 base-game WAV/MP3 files, with zero failures.
- Vulkan shader regeneration matches the checked-in bytes using the pinned tools.
  The existing Metal generator also runs; its checked-in output is unchanged.
- The Linux compatibility fixes are committed in the forked game core and pinned
  by the engine's submodule; dependency setup does not modify game-core sources.

## Reproduce

See the Linux build instructions in [README](../README.md). After building:

```sh
cmake --preset linux-release -DOC4_VULKAN_TEST=ON
cmake --build --preset linux-release --parallel 4
SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy OC4_GPU_DEBUG=1 \
  VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
  xvfb-run -a ctest --test-dir build/linux-release --output-on-failure

python3 tools/validate_linux.py --civ4 "$PWD/civ4/game/Civ4" --graphics
```

The gameplay harness writes logs, screenshots, saves and `results.json` under
`build/linux-validation`, with settings isolated from the user's normal games.

## Remaining validation

The Ubuntu CI job is provided but has not been run remotely. Native Windows and
macOS builds, Windows checksum parity and an original-game save fixture still need
their respective environments or reference files. The harness accepts a Windows
reference log through `--reference`.

Hardware GPU drivers, physical audio output, a full manual playthrough and optional
FFmpeg movie playback have not been validated locally. Custom C++ mod cores remain
outside this port's scope.

Game-core changes are committed separately in `foofly/openciv4-dll` on `linux-port`.
The engine pins that commit through its submodule, so recursive clones include the
compatibility fixes without a setup-time patch.
