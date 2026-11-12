# Distribution audit

Audited on 2026-11-12: engine commit `0f8bc11` and game-core commit `e80e8b5`,
including the public fork branches and tags fetched from GitHub. This is a file
inventory and provenance check, not a finding that either repository is copyright-free.

The agreed scope is to retain the Firaxis SDK source and licensed project code,
while excluding commercial game assets, installers, saves and packaged binaries.
No SDK removal or history rewrite is planned.

## Published content checked

| Repository | Reachable commits checked | Distinct file blobs checked | Result |
|---|---:|---:|---|
| `foofly/openciv4` | 38 | 292 | No game installers, packaged binaries, archives, models, textures, audio, movies or saves found. |
| `foofly/openciv4-dll` | 23 | 341 | No game media, installers, packaged binaries or saves found. Firaxis SDK source is present. |

Neither fork had GitHub release attachments at the time of the audit. Game files
under local `civ4/`, dependency downloads under `external/`, and generated files under
`build/` are excluded from Git. No supplied game files were deleted or uploaded by
the audit.

Exact-content comparisons against the supplied extraction found no engine matches.
A second comparison normalized CRLF/LF line endings and UTF-8 BOMs in 11,782 local
text files. It found no engine matches and identified 198 distinct game-core paths
matching SDK source included with the game. This confirms the SDK provenance; it
does not establish redistribution permission. Exact comparisons do not detect all
modified or derivative material.

The engine's only image/binary file is the inherited project icon,
`apps/oc4core/openciv4.ico`, introduced by upstream commit `d689eb4`. Its Git blob is
`906ea16766d65b3d91aa43842ad8dd920fa03b50`. It did not exactly match a supplied game
file. Its artwork provenance was not independently verified; the distribution check
records this specific upstream file as an inventory exception.

## License findings

* The engine's `LICENSE` identifies its code as MIT-licensed. Its copyright and
  license notices are retained. MIT permits redistribution under its conditions;
  it does not make the code copyright-free. See the
  [MIT license text](https://opensource.org/license/mit).
* The game-core `LICENSE` explicitly excludes `CvGameCoreDLL/` from MIT and refers
  to Firaxis SDK terms. That source is intentionally retained under those terms,
  with its existing notices. The audit does not independently determine the scope
  of SDK redistribution rights; retaining the source does not relicense it as MIT.
* The supplied game's readme and installer EULA were inspected locally. Neither
  legal document has been copied into the repositories. They are not substituted
  for an authoritative determination of the applicable SDK redistribution terms.
* Dependency archives remain local; the setup script fetches the declared pinned
  upstream packages. Binary release licensing would require a separate review of
  the actual package contents, including each bundled dependency's notices.

The icon's artwork provenance was not independently verified. The retained source
and artwork remain subject to their applicable licenses. If prohibited game files
are ever found in history, removing them only from the current tree would leave
earlier copies reachable through branches/tags; remediation must cover those copies too.

## Checks for future changes

```sh
# Current committed source and initialized game core:
python3 tools/check_distribution.py

# Staged changes, before committing/publishing:
python3 tools/check_distribution.py --staged

# All locally reachable history; fetch public branches/tags before an audit:
python3 tools/check_distribution.py --all-history
```

The check blocks local-only directories, common game/media/archive extensions,
unreviewed binary content, LFS objects and unexpectedly large source files. New
original artwork or synthetic data needs an explicit provenance review and a
deliberate policy update. The inherited project icon is fingerprinted rather than
allowing arbitrary icons. The check also scans the initialized `gamecore` repository.

The `source distribution audit` workflow runs this check for pushes, pull requests
and manual dispatch when Actions is enabled. CI detects a bad push after publication;
it is not a guarantee against disclosure. Neither the script nor filename checks
can establish ownership, detect every copied code fragment, or certify license
compliance. Keep all original copyright and license notices.
