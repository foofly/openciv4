# OpenCiv4 (Linux fork)

[![fork build](https://github.com/foofly/openciv4/actions/workflows/build.yml/badge.svg?branch=linux-port)](https://github.com/foofly/openciv4/actions/workflows/build.yml?query=branch%3Alinux-port)
[![upstream build](https://github.com/bssth/openciv4/actions/workflows/build.yml/badge.svg)](https://github.com/bssth/openciv4/actions/workflows/build.yml)
[![Status](https://img.shields.io/badge/status-alpha-orange)]()
![MIU](https://img.shields.io/badge/made_in-Ukraine-ffd700?labelColor=0057b7)

An experimental, clean-room, 64-bit engine for *Sid Meier's Civilization IV: Beyond the Sword*
that runs on the original game's assets. Game assets and installers are not bundled:
you need your own installed copy (3.19, Steam, GOG or disc). The `gamecore` submodule
does include Firaxis SDK source under separate terms; see [distribution and licensing](#distribution-and-licensing).

The game's own game core (`CvGameCoreDLL`, from the SDK Firaxis released, rebuilt as 64-bit)
and its Python scripts run as they are; openciv4 does the rest: the map, the interface, the
leaders, sound and music. Mods run too, including mods with a game core of their own once it
is rebuilt for x64.

Support the original developer

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/V7V51ZQ1RH)

Discussion: [CivFanatics thread](https://forums.civfanatics.com/threads/openciv4-clean-room-x64-engine-reimplementation.705223/).

## About this fork

[foofly/openciv4](https://github.com/foofly/openciv4) adds a native x86-64 Linux port to
[bssth/openciv4](https://github.com/bssth/openciv4). The default branch is `linux-port`.
It reuses upstream's SDL3 window/input/audio, FreeType text and POSIX compatibility support,
and adds Vulkan rendering plus the build and runtime fixes described below.

The game core is pinned to a commit in [foofly/openciv4-dll](https://github.com/foofly/openciv4-dll).
Its Linux fixes are committed there, so recursive clones include them without applying a
patch during setup. The engine port was introduced in
[461b1e4](https://github.com/foofly/openciv4/commit/461b1e4); the game-core fixes were introduced in
[8b94754](https://github.com/foofly/openciv4-dll/commit/8b94754).

The playable branch includes upstream engine `10de692` and game core `7eb1d6b`.
This brings in gamepad controls, edge scrolling, optional automatic idle turns,
minimap and terrain/forest rendering fixes, Windows-1252 XML decoding, the non-Windows
registry shim and standard-setting fallbacks. Linux support and PitBoss are retained.
The core also includes upstream's array-deletion, terrain guard, Python-binding and
POSIX compatibility fixes.

Start with [Building on Linux](#building-linux-x86-64). Existing Windows and macOS instructions
are retained below. The fork build badge tracks `linux-port`; the upstream badge tracks
the original project.

### Changes from upstream

| Area | Code changes and resulting behavior |
|---|---|
| Build configuration | `CMakePresets.json` adds Clang/Ninja Linux Release and Debug presets. CMake links the POSIX loader library and OpenSSL Crypto, uses the target's actual game-core filename, and discovers optional FFmpeg through pkg-config. |
| Dependency setup | `tools/setup_linux.py` downloads sources with pinned SHA-256 hashes and builds a private shared UCS-4 Python 2.7.18. Its completion marker records that configuration, so an older cache is rebuilt. It supplies pybind11, SDL3, FreeType, zlib, DirectXMath and dr_mp3, with optional pinned shader tools. System Python is unchanged. |
| Vulkan shaders | `tools/shaders/gen_spirv.py` translates the existing HLSL for all six model, overlay and terrain stages. It combines image samplers, restores SDL descriptor sets, preserves the terrain uniform layout, validates SPIR-V, and supports `--check` for stale generated output. `SpirvShaders.inc` embeds the results, so normal builds need no shader compiler. |
| Renderer | `Renderer.cpp` selects SDL's Vulkan/SPIR-V path on Linux and reports Vulkan initialization failures. The existing Windows DXBC and macOS Metal paths remain. `Renderer::createTexture()` gains an explicit array option; terrain atlases, details and decals retain array views even when they have only one layer. |
| Asset lookup | `AssetResolver` indexes normalized loose-file keys while retaining actual filesystem spelling. Mixed-case paths work on case-sensitive filesystems; mod/expansion/base priority and loose-file precedence over archives are preserved. The same index serves asset listings used by XML and Python loading. Archive-owning layers move during container growth on both MSVC and Clang. |
| Installation discovery | `InstallLocator` searches native and Flatpak Steam locations and additional libraries in `libraryfolders.vdf`. Explicit `--civ4` and `OC4_CIV4_DIR` paths remain available for GOG, disc and other layouts. |
| Saves | `SaveFile.cpp` uses OpenSSL EVP MD5 on Linux for the existing save checksum. The Windows BCrypt and macOS CommonCrypto implementations remain; the serialized save format is unchanged by this port. |
| Networking | `Net.cpp` supplies the POSIX descriptor range to `select()`, prevents a disconnected peer from terminating Linux through SIGPIPE, clears a closed listener handle, and explicitly includes the header needed for `std::exchange`. The existing `--host`/`--join` wire protocol is unchanged. |
| PitBoss | `Pitboss.cpp` adds the Linux dedicated server, seat authentication, ordered frames, local admin socket, timers, durable checkpoints and journal replay. `PitbossGame.cpp` adapts the SDK and validates commands; `GameSession`, `Utility` and `Net` connect server and client roles. The menu adds seat/password login and screen edit-box input. [Operator guide](docs/pitboss.md). |
| Mods and paths | Relative `--mod` paths accept forward slashes or backslashes, while save metadata retains Windows separators. Non-Windows game-core loading rejects mods containing custom cores with a clear diagnostic. XML/Python/art mods use the linked vanilla native core. |
| Automated tests | `linux_assets_test` covers mixed-case/UTF-8 assets, layer priority and Steam discovery. `linux_net_test` exercises TCP accept, welcome data and disconnect handling. `vulkan_smoke_test` draws synthetic terrain, exercises one-layer arrays and storage buffers, creates all shader stages and checks overlay pixels/orientation. It fails on SDL error logs; CTest also rejects Vulkan validation errors. |
| Portable regression checks | `asset_paths_test` checks mixed-case/backslash/UTF-8 paths and layer precedence using filesystem identity. `save_file_test` checks synthetic save round trips, known MD5 values, compressed chunk boundaries and truncated input. Neither needs game assets. |
| CI and gameplay checks | The Ubuntu 24.04 job builds the engine, checks shader regeneration and runs CTest with Xvfb/Mesa Vulkan. `tools/validate_linux.py` checks repeated autoplay, save/resume, data mods, custom-core diagnostics and optional rendering/menu launch; `--reference` accepts a Windows checksum log. |
| Repository and documentation | `.gitmodules` pins the forked game core. `.gitignore` excludes local `civ4/` assets/installers and Python caches in addition to existing build/dependency exclusions. This README and `docs/linux-validation.md` document setup, usage and validation. |

The [game-core README](https://github.com/foofly/openciv4-dll#changes-in-this-fork) lists
its individual include-path, logging and variadic-format fixes.
This fork does not introduce gameplay-rule or AI changes.

### Linux validation and limitations

Local validation used Fedora 44, Clang 22.1.8, SDL 3.4.18, Mesa software Vulkan,
Intel Iris Xe hardware Vulkan and a Beyond the Sword installation reporting version 3.19:

* All **18 CTest cases** pass, including portable asset/save checks and upstream's
  edge-scroll, idle-turn, gamepad and XML-decoding tests. Vulkan rendering passes under X11/Xvfb and Wayland with
  Khronos validation layers enabled, including synthetic terrain and overlay readback.
* A **100-turn Fractal/Standard/seed-7** game repeats exactly. A turn-50 save resumes with
  identical checksums through turn 100; the final checksum is `D7D43800`.
* A **500-turn** run matches every pre-update checksum and repeats exactly. Both the
  old and new turn-250 saves resume identically; the final checksum is `2D59DB00`.
* Matching three-turn graphics/headless runs agree. The menu, original Python interface,
  map and models render with the supplied assets. **Next War** runs, saves and reloads.
* PitBoss passes two-client BtS and Next War checks, late joins/rejoins, offline timer
  expiry, paused recovery, journal recovery after SIGKILL, authentication and invalid-order
  rejection, and shutdown. Menu login and timed turns with diplomacy open also pass.
  A three-minute connected soak keeps both clients synchronized through turn 41.
* Intel Iris Xe with Mesa 26.2.6 passes terrain/overlay readback, owned-game rendering
  and menu launch on native Wayland with Khronos validation enabled. Xvfb uses software
  Vulkan; its virtual display does not provide the DRI3 support needed by this Intel driver.
* The audio scanner decodes **2,370 base-game WAV/MP3 files** with zero failures. Vulkan
  shader regeneration is reproducible with the pinned tools; the Metal generator also runs.

Other hardware GPU drivers, physical audio output, a full manual playthrough, optional
FFmpeg movies, a Windows checksum reference and an original-game save fixture still need
validation. Native macOS validation also remains outstanding; the fork workflow reports
Windows and Ubuntu build/test results.
The full build workflow runs on `linux-port` and `master` pushes, pull requests, version
tags and manual dispatch. It builds Linux and Windows, checks shader generation and
runs asset-free tests; Linux includes Xvfb/Mesa Vulkan validation. New pushes cancel
older runs for the same branch. Publishing the same commit on `feature/pitboss` does
not duplicate the full build; the `linux-port` run covers the playable commit.
Custom C++ mod cores, Linux ARM64, Steam Deck certification and Linux release packages are
outside the initial port. See the [validation report](docs/linux-validation.md) for commands
and remaining checks.

## Playing

**Linux:** build this fork using the [Linux instructions](#building-linux-x86-64), then run:

```sh
./build/linux-release/bin/oc4core --civ4 "/path/to/Civ4" --menu
```

`/path/to/Civ4` must contain `Assets`, `Warlords` and `Beyond the Sword`. This fork currently
provides source builds for Linux; the downloads below are upstream Windows releases.

**Windows:** download the zip of the latest [upstream release](https://github.com/bssth/openciv4/releases), unpack it
anywhere and start `Play Beyond the Sword.bat`; `README.txt` inside says the rest. Windows 10
or 11, 64-bit, with a Direct3D 12 graphics card.

**Windows mods.** `Play Mod.bat` takes a mod's folder (drop it onto the file). Mods that change only
XML, Python and art run on the vanilla game core. A mod with a game core of its own runs when
its `Assets` folder has the 64-bit build, `CvGameCoreDLL-x64.dll`, next to the 32-bit
`CvGameCoreDLL.dll` the original game loads: one copy of the mod serves both games. How to
port a mod's game core: [openciv4-dll/docs/PORTING.md](https://github.com/bssth/openciv4-dll/blob/main/docs/PORTING.md).

## What works

The engine's existing feature overview follows. The checks completed for this Linux fork
are listed under [Linux validation and limitations](#linux-validation-and-limitations).

* **Playing a game**, new or loaded (saves of the original game load too): the game's own
  interface (`CvMainInterface.py` and the mod's screens, drawn with the game's interface
  theme), the advisors and the Civilopedia, the city screen, diplomacy with the leaders, the
  game's popups, hotkeys, sounds and music, movies (FFmpeg), autosaves.
* **The map** read through the DLL's exports as the original engine reads it: terrain blended
  from the terrain art, rivers, roads, features, improvements and resources, cities laid out
  by the city LSystem with their walls and wonders, units with their formations and
  animations, combat, borders, fog of war, city bars and unit flags.
* **AI autoplay**, headless or watched, deterministic per seed; saves in the original format
  that continue exactly as if never stopped.
* **Mods**: Rise of Mankind: A New Dawn 2 and Chronicles of Mankind play with their own
  game cores ported to x64. BUG 4.4 runs as it is; with BULL 1.2, its game core, ported
  ([BULL-x64](https://github.com/MortenLyhr/BULL-x64), so far built and played on macOS only).
* **Multiplayer** over the network (`--host`, `--join`), early.
* **PitBoss on Linux** (`--pitboss CONFIG.ini`): a headless persistent server with
  simultaneous turns, assigned password-protected seats, reconnects, a turn timer,
  local administration and crash recovery. See [setup and operation](docs/pitboss.md).
  This uses an OpenCiv4-only protocol over a trusted LAN/VPN.

Not yet: the original's main menu (a simpler one stands in), some animations, and custom
mod game cores on Linux and macOS. Native Linux uses SDL3's Vulkan backend; macOS uses Metal.

## Building (Windows, x64)

Visual Studio 2022 or 2026 with **Desktop development with C++** (CMake and Ninja come with
it). The game core is the `gamecore` submodule
([this fork's openciv4-dll](https://github.com/foofly/openciv4-dll)): clone with `git clone --recursive`, or
run `git submodule update --init`.

Third-party packages go under `external\` (not committed; `.github/workflows/build.yml`
fetches the same ones):

| Package | Folder | Used for |
|---|---|---|
| Python 2.7.18 x64, extracted: `msiexec /a python-2.7.18.amd64.msi /qn TARGETDIR=%CD%\external\python27` | `python27` | the game's scripts |
| pybind11 2.9.2 (the last series for Python 2.7), source archive | `pybind11-2.9.2` | the game core's Python bindings |
| zlib 1.3.1, source archive | `zlib-1.3.1` | saves |
| SDL3 3.4.18, `SDL3-3.4.18.zip` | `SDL3-3.4.18` | window, input, sound, GPU |
| `dr_mp3.h` from mackron/dr_libs | `dr_libs` | music |
| optional: FreeType 2.13.3, source archive | `freetype-2.13.3` | text off Windows (on Windows its test only) |
| optional: FFmpeg 8.1 LGPL shared build (BtbN's `ffmpeg-n8.1-latest-win64-lgpl-shared-8.1.zip`) | `ffmpeg-8.1` | movies; without it the wonders show their model |

Other locations: `-DOC4_PYTHON27_DIR=...`, `-DOC4_PYBIND11_DIR=...`, `-DOC4_ZLIB_DIR=...`,
`-DOC4_SDL3_DIR=...`, `-DOC4_DR_LIBS_DIR=...`, `-DOC4_FREETYPE_DIR=...`, `-DOC4_FFMPEG_DIR=...`.

```bat
build.cmd release
ctest --test-dir build\Release
```

Binaries end up in `build\Release\bin`; `tools\make-demo.ps1 -Mod vanilla` packs them into a
zip as the releases are. In Visual Studio open the repository folder (it uses
`CMakePresets.json`), or `generate-sln.cmd` writes a solution into `build\vs2026`.

Mods' game cores ported to x64 are built along when their sources are there
(`-DOC4_AND2_SOURCE_DIR=...`, `-DOC4_COM_SOURCE_DIR=...`), into `build\Release\mods\<name>`.

Profile-guided optimisation of the game core (optional, about 20 minutes, needs the game):
`tools\pgo.cmd "<Civ4>"`, another 5-10% off late-game turns with identical game results.

## Building (Linux, x86-64)

Clang, CMake 3.28+, Ninja and Python 3.12+ are required. Ubuntu 24.04 prerequisites:

```sh
sudo apt-get install git cmake clang ninja-build pkg-config build-essential python3-dev libssl-dev libffi-dev zlib1g-dev \
  libx11-dev libxcursor-dev libxi-dev libxrandr-dev libxss-dev libxfixes-dev libxext-dev libxtst-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev libasound2-dev libpulse-dev libdecor-0-dev \
  libvulkan-dev mesa-vulkan-drivers fonts-dejavu-core
```

On Fedora:

```sh
sudo dnf install git clang cmake ninja-build pkgconf-pkg-config gcc gcc-c++ make python3-devel openssl-devel libffi-devel zlib-devel \
  libX11-devel libXcursor-devel libXi-devel libXrandr-devel libXScrnSaver-devel libXfixes-devel libXext-devel libXtst-devel \
  wayland-devel wayland-protocols-devel libxkbcommon-devel alsa-lib-devel pulseaudio-libs-devel \
  libdecor-devel vulkan-loader-devel mesa-vulkan-drivers dejavu-sans-fonts
```

Install the Vulkan driver for your GPU (NVIDIA users need their matching driver). Then:

```sh
git clone --recurse-submodules --branch linux-port https://github.com/foofly/openciv4.git
cd openciv4
git submodule update --init
python3 tools/setup_linux.py
cmake --preset linux-release
cmake --build --preset linux-release --parallel 4
ctest --test-dir build/linux-release --output-on-failure
build/linux-release/bin/oc4core --civ4 "/path/to/Civ4" --menu
```

The setup script verifies pinned source archives and builds a private shared UCS-4 Python 2.7.18
under `external/python27`; it does not replace your system Python. A configuration marker
detects older Python caches and rebuilds them. After rerunning setup on an older checkout,
rerun CMake and rebuild the engine/core against the updated Python headers and library.
This fork pins a game-core
commit containing the case-sensitive include and POSIX formatting fixes. Debug builds use the
`linux-debug` preset. FFmpeg is optional: install development packages for libavcodec,
libavformat and libavutil for movies, or set `OC4_FFMPEG_DIR` to a compatible prefix.

`--civ4` is the directory containing `Assets`, `Warlords` and `Beyond the Sword`, not the
expansion directory itself. `OC4_CIV4_DIR` sets the same default. Native and Flatpak Steam
libraries are searched automatically. For the extracted GOG layout in this workspace:

```sh
build/linux-release/bin/oc4core --civ4 "$PWD/civ4/game/Civ4" --menu
```

Saves and settings use `~/Documents/My Games`; `OC4_DOCUMENTS=/absolute/path` redirects them.
The original assets are read in place. XML/Python/art mods work with the vanilla game core,
for example `--mod "Mods/Next War"`. Mods shipping a custom game core are rejected until
native mod-core loading is implemented. Windows DLLs cannot be used by the native executable.

Vulkan shaders are checked in, so normal builds need no shader tools. After changing HLSL:

```sh
python3 tools/setup_linux.py --shader-tools
python3 tools/shaders/gen_spirv.py
python3 tools/shaders/gen_spirv.py --check
```

This builds glslang, SPIRV-Tools and SPIRV-Cross from the pinned Vulkan SDK 1.4.363.0
source tags, matching upstream's shader checks. Both generators use these local tools
when installed. Also regenerate Metal with `python3 tools/shaders/gen_msl.py`.
An asset-free Vulkan pipeline/overlay test is built as
`build/linux-release/bin/vulkan_smoke_test`; enable it in CTest with `-DOC4_VULKAN_TEST=ON`.
It requires a working display and Vulkan driver. CI runs it under Xvfb with Mesa software
rendering. `OC4_GPU_DEBUG=1` enables GPU diagnostics. If GPU creation fails, check `vulkaninfo`
and the installed driver; headless simulation (`--turns`, without rendering options) requires
neither a display nor a GPU.

For local gameplay and save/resume checks using your own assets:

```sh
python3 tools/validate_linux.py --civ4 "$PWD/civ4/game/Civ4" --graphics
```

This writes logs, screenshots and isolated saves/settings under `build/linux-validation`.
Omit `--graphics` for a display-free run. `--reference windows.log` additionally compares a
Windows run with the same Fractal/Standard/seed-7 settings; it is not claimed without a
reference log. Game files and installers under `civ4/` are ignored by Git.
See [local validation results and remaining checks](docs/linux-validation.md).

## Building (macOS, Apple Silicon)

A native arm64 build (no Rosetta or Wine): the same engine and game core, built by Apple clang,
rendering through SDL3's Metal backend. Xcode or the Command Line Tools, and from Homebrew
`cmake ninja pyenv`, plus `ffmpeg` for movies (optional).

* Python 2.7.18 as a shared library (there is no Homebrew formula):
  `PYTHON_CONFIGURE_OPTS=--enable-shared pyenv install 2.7.18`.
* Under `external/`, as on Windows: pybind11 2.9.2, zlib 1.3.1, SDL3 3.4.18 (source), dr_libs,
  FreeType 2.13.3 (the text off Windows: required here), and DirectXMath (microsoft/DirectXMath,
  header only; part of the Windows SDK on Windows) in `DirectXMath`.

```sh
cmake -S . -B build/macos -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DOC4_PYTHON27_DIR=$HOME/.pyenv/versions/2.7.18 -DOC4_FFMPEG_DIR=$(brew --prefix ffmpeg)
cmake --build build/macos
ctest --test-dir build/macos
build/macos/bin/oc4core --civ4 "<Civ4>" --menu
```

`<Civ4>` is a Beyond the Sword install's root (copied from a Windows install or the GOG
or Steam download). Saves and settings go to `~/Documents/My Games` as on Windows
(`OC4_DOCUMENTS=<folder>` puts them elsewhere). The shaders' Metal versions are generated from
the HLSL and checked in (`src/render/MslShaders.inc`); after changing a shader, run
`python3 tools/shaders/gen_msl.py` (needs `glslang` and `spirv-cross` from Homebrew). CI runs
it and `gen_spirv.py` with `--check`, which fails when a checked-in file is out of date.

The macOS game core plays the same games as the Windows x64 one: AI games from the same seed
give the same sync checksums turn by turn.

## Command line

`<Civ4>` is the install folder with `Assets`, `Warlords` and `Beyond the Sword`; without
`--civ4` the engine finds it from the registry or Steam.

```bat
rem the menu: a new game or a saved one
oc4core.exe --menu

rem play a new game, or continue a save; a mod by its folder in Beyond the Sword\Mods or its full path
oc4core.exe --play --map Continents --size SMALL
oc4core.exe --play --load game.CivBeyondSwordSave --mod "Mods\My Mod"

rem the AI plays 100 turns, one line per turn with the sync checksum; save at turn 50
oc4core.exe --turns 100 --map Fractal --size STANDARD --seed 7 --save-at 50 --save game50.CivBeyondSwordSave

rem where the turns go
oc4core.exe --load game50.CivBeyondSwordSave --turns 10 --profile

rem watch the AI play (Space plays, N one turn), or render the map into a PNG
oc4core.exe --load game50.CivBeyondSwordSave --view
oc4core.exe --turns 50 --screenshot map.png --look 40,30,12 --view-size 1920x1080
```

The interface has a scale of its own: the Screen Resolution choice on the options' Graphics tab
(Auto follows the window's pixel density), or `--ui-scale PERCENT` for one run. The map keeps
all the window's pixels.

With a menu, a popup or a screen up, Tab (Shift+Tab back) starts choosing its controls with the
keys: then the arrow keys move between them instead of the map, and Return or Space uses the one
chosen (a ring marks it; moving the mouse ends it). Until then the arrows pan the map under it, as
in the original; in the main menu, with no map behind, they choose at once.

Ctrl+Shift+F shows the frame rate and the longest frame of the last half second (`--show-fps`
from the start).

W A S D can pan the map as the arrows do, an option that is off by default: "Pan the map with
W A S D" on the options' Game tab turns it on and off, and while it is on Ctrl+Shift+W turns it off
too (a note at the top says so; with the option off, Ctrl+Shift+W is the game's own); it is kept in profile.ini as `wasdPan`, and `--wasd-pan` turns it on for one run. Only
the letters pressed alone pan, so while it is on their own hotkeys (S Sentry, A Automate, W the
next unit, D destroy) need the unit's buttons; with Shift, Ctrl or Alt they are still the game's
(Ctrl+S saves, Shift+D is diplomacy), and with a menu, a popup or a screen up they are letters
there. The arrows are unchanged. The routing is `src/render/WasdPan.h`, tested by
`tests/wasd_pan_test.cpp`.

Idle turns can end by themselves, an option that is off by default. The game ends a turn alone
only when a unit moved in it (and "Wait at End of Turn" is off); with this on, a turn waiting at
"press Enter" also ends after 2 seconds, so with automated cities and workers the game keeps
going until something needs you. A popup, a talk, a screen or the city screen pauses it, and a
key or click while it waits keeps that turn; hot seat and by-email games never use it. Ctrl+Shift+E
turns it on and off (with a note at the top), and so does "End idle turns automatically" on the
options' Game tab; it is kept in profile.ini as `autoEndTurn`, and `--auto-end-turn` turns it on
for one run. `OC4_PLAY_LOG=1` prints "idle, ended by itself" for each such turn. Decisions: it is
in the engine (`apps/oc4core/AutoEndTurn.h`, called from the play loop), not the game core, so the
game's rules, AI and saves are untouched; it sends the same end-turn control as Enter. Tested by
`tests/auto_end_turn_test.cpp`.

The mouse at the window's edge scrolls the map, as in the original; "Scroll the map at the screen
edge" on the options' Game tab turns it off (on by default). It is kept in profile.ini as
`edgeScroll`, and `--no-edge-scroll` turns it off for one run (scripted runs with `--view-seconds`
never edge scroll). The rule is `src/render/EdgeScroll.h`, tested by `tests/edge_scroll_test.cpp`.

The globe view's colour layers (Culture, Trade, Religion) are drawn on the globe itself.

Tools for the game's files, in `build\Release\bin`:

* `nifview` browses the models and their animations (`--filter art/units/`), or renders one
  frame into a PNG (`--screenshot knight.png --anim strikea --time 0.3 art/units/knight/knight.nif`);
* `nifscan` (parse coverage of every model), `nifdump` (the blocks and scene graph of one),
  `animscan` (every animation bound and sampled), `themedump` (the interface theme's styles),
  `soundscan` (every sound decoded).

## Repository

| Folder | What |
|---|---|
| `src/core`, `src/nif`, `src/gfx`, `src/scene`, `src/anim` | FPK archives and asset lookup, NIF/KFM readers, DDS/TGA, the scene graph, animation |
| `src/render` | the renderer (SDL GPU), map view, interface theme, 2D overlay and text |
| `src/audio`, `src/media` | sound mixer, WAV/MP3; movies through FFmpeg |
| `apps/oc4core` | the engine: the SDK's engine interfaces, Python host, saves, the game's screens |
| `apps/nifview`, `tools` | the viewer and the tools above |
| `gamecore` | the pinned native game core ([foofly/openciv4-dll](https://github.com/foofly/openciv4-dll)), based on the [upstream SDK port](https://github.com/bssth/openciv4-dll) |

See [CONTRIBUTING.md](CONTRIBUTING.md) before sending a change.

## Distribution and licensing

The [distribution audit](docs/distribution-audit.md) checked both forks' public Git
history and release attachments. It found no game installers, textures, models,
audio, movies or saves. Local game files remain outside the published source.
`tools/check_distribution.py --staged` checks proposed commits, and the separate
source-audit workflow checks committed files and history on pushes and pull requests.

The engine's MIT license retains the original copyright notices. The Firaxis SDK
source is intentionally retained in `gamecore` under the SDK's separate terms;
the MIT license does not apply to it. The inherited project icon is recorded as a
specific exception in the audit. These checks keep commercial game files out of
the source distribution; they do not replace the applicable source and artwork licenses.

## Legal

This is an independent fan project, not affiliated with Firaxis, 2K or Gamebase. It
implements file formats and interfaces from public documentation and the officially released
Civ4 SDK headers, and loads assets from the user's own installation.

openciv4 is under the MIT License ([LICENSE](LICENSE)). The game core in `gamecore` is the
Firaxis SDK's code under its own terms. The release packages carry Python 2.7 (PSF License),
FFmpeg (LGPL) and the Visual C++ runtime, with their licenses.
