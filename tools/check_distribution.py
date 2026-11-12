#!/usr/bin/env python3
"""Detect accidentally committed game files in the engine and its game core.

This checks distribution hygiene, not copyright ownership or SDK licensing.
Run before publishing with --staged, or audit reachable Git history with --all-history.
"""
import argparse
from pathlib import Path, PurePosixPath
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PRIVATE_DIRS = {"civ4", "external", "build", "out", "mods", "screenshots", "captures"}
GAME_SUFFIXES = {
    ".exe", ".dll", ".so", ".dylib", ".msi", ".bin", ".iso", ".cue",
    ".zip", ".7z", ".rar", ".gz", ".xz", ".fpk", ".dds", ".nif", ".kf", ".kfm",
    ".bik", ".wav", ".mp3", ".ogg", ".mp4", ".avi", ".ttf", ".otf",
    ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".ico", ".pdf", ".xml",
    ".civbeyondswordsave", ".civwarlordssave", ".civ4savedgame",
    ".civbeyondswordwbsave", ".civwarlordswbsave", ".civ4worldbuildersave",
}
# Inherited project artwork, retained unchanged. This fingerprint is an inventory
# exception, not an independent certification of the artwork's provenance.
PROJECT_ART = {("apps/oc4core/openciv4.ico", "906ea16766d65b3d91aa43842ad8dd920fa03b50")}
MAX_SOURCE_BYTES = 10 * 1024 * 1024


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args])


def entries(repo, history, staged):
    found = set()
    if staged:
        for record in git(repo, "ls-files", "--stage", "-z").split(b"\0"):
            if not record:
                continue
            header, name = record.split(b"\t", 1)
            mode, sha, stage = header.decode().split()
            if stage != "0":
                raise RuntimeError("Resolve unmerged index entries before auditing")
            found.add((mode, sha, name.decode("utf-8", errors="surrogateescape")))
    else:
        refs = git(repo, "rev-list", "--all").decode().splitlines() if history else ["HEAD"]
        for ref in refs:
            for record in git(repo, "ls-tree", "-r", "-z", ref).split(b"\0"):
                if not record:
                    continue
                header, name = record.split(b"\t", 1)
                mode, _, sha = header.decode().split()
                found.add((mode, sha, name.decode("utf-8", errors="surrogateescape")))
    return found


def audit(repo, history, staged):
    records = entries(repo, history, staged)
    errors = []
    checked = set()
    for mode, sha, name in sorted(records):
        if mode == "160000":
            continue
        path = PurePosixPath(name)
        if any(ord(c) < 32 for c in name):
            errors.append((name, "control characters in filename"))
        elif path.parts[0].lower() in PRIVATE_DIRS:
            errors.append((name, "local-only directory"))
        elif (name, sha) in PROJECT_ART:
            continue
        elif path.suffix.lower() in GAME_SUFFIXES:
            errors.append((name, "game data, media, binary or archive requires review"))
        elif sha not in checked:
            checked.add(sha)
            size = int(git(repo, "cat-file", "-s", sha))
            if size > MAX_SOURCE_BYTES:
                errors.append((name, "unexpectedly large source file"))
                continue
            data = git(repo, "cat-file", "blob", sha)
            if b"\0" in data or data.startswith((b"MZ", b"PK\x03\x04", b"%PDF-", b"version https://git-lfs.github.com/spec/")):
                errors.append((name, "binary content or unreviewed LFS object"))
    for name, reason in sorted(set(errors)):
        print(f"FAIL {repo}: {name!r}: {reason}")
    if not errors:
        print(f"PASS {repo}: {len(records)} distinct Git entries; no blocked game files found")
    return not errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=ROOT)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--all-history", action="store_true")
    mode.add_argument("--staged", action="store_true")
    args = parser.parse_args()
    repo = args.repo.resolve()
    repos = [repo]
    if (repo / ".gitmodules").exists():
        core = repo / "gamecore"
        if not (core / "GameCore.cmake").exists():
            raise SystemExit("Initialize the game core first: git submodule update --init")
        repos.append(core)
    results = [audit(p, args.all_history, args.staged) for p in repos]
    print("Licenses and provenance, including Firaxis SDK terms, require separate review.")
    raise SystemExit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
