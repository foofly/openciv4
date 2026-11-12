#!/usr/bin/env python3
"""Local gameplay checks using an owned BtS installation; no assets enter CI.

--graphics needs a display and Vulkan (xvfb-run with Mesa works). --reference
accepts a Windows log made with --turns 100 --map Fractal --size STANDARD --seed 7.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CHECKSUM = re.compile(r"^\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+([0-9A-F]{8})\s+\d+\s*$", re.M)


def checksums(text):
    return {int(turn): checksum for turn, checksum in CHECKSUM.findall(text)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--civ4", required=True, type=Path)
    parser.add_argument("--build", type=Path, default=ROOT / "build/linux-release")
    parser.add_argument("--out", type=Path, default=ROOT / "build/linux-validation")
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--graphics", action="store_true")
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / "results.json").unlink(missing_ok=True)
    exe = args.build.resolve() / "bin/oc4core"
    civ4 = args.civ4.resolve()
    env = dict(os.environ, OC4_DOCUMENTS=str(out / "documents"), SDL_AUDIODRIVER="dummy")
    results = {}

    def run(name, arguments, graphical=False, failure=False):
        outputs = [Path(arguments[i + 1]) for i, value in enumerate(arguments[:-1]) if value in ("--save", "--screenshot")]
        for path in outputs:
            path.unlink(missing_ok=True)
        local_env = env.copy()
        if not graphical:
            local_env.pop("DISPLAY", None)
            local_env.pop("WAYLAND_DISPLAY", None)
        log = out / (name + ".log")
        with log.open("w") as stream:
            process = subprocess.run([str(exe), "--civ4", str(civ4), *map(str, arguments)],
                                     stdout=stream, stderr=subprocess.STDOUT, env=local_env, timeout=300)
        text = log.read_text(errors="replace")
        if failure:
            if process.returncode == 0 or "requires a custom game core" not in text:
                raise RuntimeError("Expected unsupported-core diagnostic: " + str(log))
        elif process.returncode or "Traceback (most recent call last)" in text or "Validation Error:" in text:
            raise RuntimeError("Gameplay validation failed: " + str(log))
        if not failure and any(not path.is_file() or path.stat().st_size < 8 for path in outputs):
            raise RuntimeError("Expected save/screenshot was not written: " + str(log))
        print(name, "passed", flush=True)
        results[name] = "passed"
        return checksums(text)

    game = ["--map", "Fractal", "--size", "STANDARD", "--seed", "7"]
    save = out / "turn50.CivBeyondSwordSave"
    baseline = run("autoplay100", [*game, "--turns", "100", "--save-at", "50", "--save", save])
    if len(baseline) != 100:
        raise RuntimeError("Expected 100 turn checksums")
    resumed = run("resume50", ["--load", save, "--turns", "50"])
    if resumed != {turn: value for turn, value in baseline.items() if turn > 50}:
        raise RuntimeError("Save/resume changed the game checksums")
    repeated = run("repeat100", [*game, "--turns", "100"])
    if repeated != baseline:
        raise RuntimeError("Repeated game changed the checksums")
    results["save_resume_and_repeat"] = "identical turn checksums"
    if args.reference:
        if checksums(args.reference.read_text(errors="replace")) != baseline:
            raise RuntimeError("Windows reference checksums differ")
        results["windows_parity"] = "passed"
    else:
        results["windows_parity"] = "not tested: no reference log supplied"

    mods = civ4 / "Beyond the Sword/Mods"
    if (mods / "Next War").is_dir():
        mod_save = out / "nextwar.CivBeyondSwordSave"
        run("nextwar", ["--mod", "Mods/Next War", "--turns", "10", "--seed", "7", "--save", mod_save])
        run("nextwar_resume", ["--mod", "Mods\\Next War", "--load", mod_save, "--turns", "1"])
    for mod in ("Rhye's and Fall of Civilization", "The Road to War"):
        if (mods / mod / "Assets/CvGameCoreDLL.dll").exists():
            run("unsupported_" + mod.split()[0].lower(), ["--mod", "Mods/" + mod, "--turns", "0"], failure=True)
    if args.graphics:
        # Autoplay returns control to the human on its final turn, so compare
        # runs of the same length rather than a prefix of the 100-turn run.
        short = run("render_headless3", [*game, "--turns", "3"])
        rendered = run("render", [*game, "--turns", "3", "--screenshot", out / "map.png", "--view-size", "960x540", "--look", "40,30,12"], graphical=True)
        if rendered != short or len(rendered) != 3:
            raise RuntimeError("Rendering changed the game checksums")
        run("menu_play", ["--menu", "--keys", "{MenuNew}{MenuLaunch}", "--view-seconds", "8",
                          "--screenshot", out / "play.png", "--view-size", "1280x800"], graphical=True)
    results["final_checksum"] = baseline[100]
    (out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print("Results:", out / "results.json")


if __name__ == "__main__":
    main()
