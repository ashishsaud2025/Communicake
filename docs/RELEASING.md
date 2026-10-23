# Releasing

The version lives in one place, the `project()` call in `CMakeLists.txt`. The
`communicake/version.hpp` header that the binary prints is generated from it at
configure time, so there is nothing else to edit by hand.

## Checklist

1. **Start from a clean, green tree.**

   ```bash
   git status                # no uncommitted changes
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ctest --test-dir build --output-on-failure
   ```

   All tests must pass. The same three jobs run on every push in CI (Linux,
   Windows, and the AddressSanitizer build); wait for them on the last commit
   before tagging.

2. **Bump the version in `CMakeLists.txt`.**

   ```cmake
   project(communicake VERSION 5.2.0 LANGUAGES CXX)
   ```

   Follow semantic versioning: a new option or behaviour is a minor bump, a
   bug fix is a patch, and a change to the configuration format is a major
   bump.

3. **Move the changelog.**

   In `CHANGELOG.md`, rename `## [Unreleased]` to `## [5.2.0] - YYYY-MM-DD`,
   leave a fresh empty `## [Unreleased]` on top, and describe the release
   under Added / Changed / Fixed / Removed. Write for the person upgrading, not
   for the person who wrote the patch.

4. **Reconfigure and check the number the binary reports.**

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ./build/communicake --version    # must print the new version
   ctest --test-dir build --output-on-failure
   ```

   The configure step is required: the version header is only regenerated
   when CMake runs.

5. **Smoke-test the real thing.** Unit tests never open a camera, so run once
   with a camera and once against a recording:

   ```bash
   ./build/communicake --dry-run
   ./build/communicake --headless --input clip.mp4 --benchmark 120
   ```

   Check that a gesture fires once, that the printed command is the one in
   `config/actions.json`, and that the benchmark reports plausible timings.

6. **Commit and tag.**

   ```bash
   git add CMakeLists.txt CHANGELOG.md
   git commit -m "chore: release 5.2.0"
   git tag -a v5.2.0 -m "Communicake 5.2.0"
   git push origin main
   git push origin v5.2.0
   ```

7. **Publish the release.** Create a GitHub release from the tag, using the
   changelog section as the body. Attach nothing: the project builds from
   source and has no binary artefacts to distribute yet.

## If a release is broken

Do not move the tag. Ship a patch release instead: revert or fix on `main`,
bump the patch version, and note the reason in the changelog. A tagged commit
is a promise about what the source contained, even if the promise turns out to
be wrong.

## After the release

- Check that `README.md` still matches the behaviour you shipped, especially
  the option table.
- Add anything learned during the smoke test to `docs/TROUBLESHOOTING.md`
  while it is still fresh.
