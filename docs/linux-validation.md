# Linux port validation

Local environment: Fedora 44, x86-64, Clang 22.1.8, SDL 3.4.18, Mesa software
Vulkan and Intel Iris Xe with Mesa 26.2.6. The supplied GOG extraction reports `CIV4_VERSION=319`. Game assets,
screenshots, saves and installers are not committed.

## Verified

- All 18 CTest cases pass, including mixed-case/UTF-8 assets, Steam-library
  discovery, TCP accept/welcome/disconnect, POSIX formatting and Vulkan rendering.
  Portable asset-path and save-container tests cover filesystem identity, known
  MD5 values, compressed chunk boundaries, UTF-8 paths and truncated input.
  The upstream merge adds edge-scroll, idle-turn, gamepad and XML-decoding checks.
- PitBoss adds authentication, assigned-seat isolation, two-client replication,
  timer/pause arithmetic, journal replay, password rotation, kicks and roster checks.
  The real BtS and Next War integration runs pass two clients, research/end-turn orders,
  offline timer expiry, late joins, paused recovery and journal recovery after SIGKILL.
  The transport test also passes AddressSanitizer and UndefinedBehaviorSanitizer.
  A 180-second connected BtS soak passes with both clients synchronized through
  turn 41 (checksum `6D533000`), followed by the same timer/reconnect/recovery checks.
- Native SDL input checks cover login-field typing, select-all, backspace, Tab and
  password masking. The menu connects to a real server and renders the game. With
  diplomacy open, the client follows timed turns 83 through 86 without desynchronizing;
  the dedicated-client loop continues receiving frames during local dialogs.
- The Vulkan test draws synthetic terrain (including one-layer texture arrays),
  creates all six shader stages, and checks overlay colours and orientation through
  pixel readback. It passes under X11/Xvfb and Wayland with Khronos validation enabled.
  Intel Iris Xe hardware passes readback and owned-game/menu rendering on native
  Wayland with validation enabled. Xvfb works with software Vulkan; it lacks the
  DRI3 presentation support needed by this Intel hardware driver.
- A 100-turn Fractal/Standard/seed-7 run repeats exactly. Saving at turn 50 and
  resuming reproduces every checksum through turn 100: final checksum `D7D43800`.
- Two 500-turn runs match every pre-update checksum, ending at `2D59DB00`.
  Both the baseline and updated turn-250 saves resume identically through turn 500.
- The graphics and headless versions of the same three-turn run agree. Terrain,
  models, the menu and the original Python interface render using the supplied assets.
- Next War runs, saves and reloads. Both slash styles are accepted for its relative
  mod path. Rhye's and Fall and The Road to War receive the custom-core diagnostic.
- The audio scanner decodes 2,370 base-game WAV/MP3 files, with zero failures.
- Vulkan and Metal shader regeneration match the checked-in files using tools built
  from Vulkan SDK 1.4.363.0 source tags, matching upstream's shader CI.
- The Linux compatibility fixes are committed in the forked game core and pinned
  by the engine's submodule; dependency setup does not modify game-core sources.
- Setup upgrades an older Python cache to a private shared UCS-4 Python 2.7.18
  build (`sys.maxunicode=1114111`); a repeat setup reuses the configuration marker.
  Optional shader tools remain available. The engine/core are rebuilt after migration.

## Reproduce

See the Linux build instructions in [README](../README.md). After building:

```sh
cmake --preset linux-release -DOC4_VULKAN_TEST=ON
cmake --build --preset linux-release --parallel 4
SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy OC4_GPU_DEBUG=1 \
  VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
  xvfb-run -a -s '-screen 0 1280x1024x24' ctest --test-dir build/linux-release --output-on-failure

python3 tools/validate_linux.py --civ4 "$PWD/civ4/game/Civ4" --graphics
```

The gameplay harness writes logs, screenshots, saves and `results.json` under
`build/linux-validation`, with settings isolated from the user's normal games.
The [PitBoss guide](pitboss.md#validation) includes `--soak-seconds 180` for longer
connected-client checks. For late-game simulation checks, use the engine's
`--turns 500 --save-at 250 --save /path/to/turn250.CivBeyondSwordSave` with the same
Fractal/Standard/seed-7 settings, then reload that save with `--turns 250`.

## Remaining validation

The full fork workflow runs on `linux-port` pushes and reports Ubuntu/Windows builds,
asset-free tests and shader checks. Ubuntu uses Xvfb/Mesa software Vulkan. Windows
checksum parity, native macOS validation and an original-game save fixture still need
their respective environments or reference files. The harness accepts a Windows
reference log through `--reference`. Available owned-install saves were OpenCiv4
autosaves or WorldBuilder scenarios, not an original-game binary-save fixture.

Other hardware GPU drivers, physical audio output, a full manual playthrough and optional
FFmpeg movie playback have not been validated locally. Custom C++ mod cores remain
outside this port's scope.

Game-core changes are committed separately in `foofly/openciv4-dll` on `linux-port`.
The engine pins that commit through its submodule, so recursive clones include the
compatibility fixes without a setup-time patch.

The current playable branch includes upstream engine `10de692` and core `7eb1d6b`.
The merge retains the Linux runtime and PitBoss changes. Both client and server need
the updated core; older PitBoss checkpoints require their matching old build.
