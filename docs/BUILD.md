# Build and release

The source is C++20 for the native UI and C/QuickJS for the service. Build tooling uses Node.js (the existing tests use `node:sqlite`), Python 3 with SQLite FTS5 for catalog tests, and PowerShell on Windows for the Arabic label atlas and brand artwork. There is no dependency on a sibling PS2-Library checkout; shared code is under `shared/`.

1. Read the exact upstream revisions in `third_party/SOURCES.json` and runtime provenance in `third_party/runtime-sources.json`. Obtain Zig 0.14.1, the PS5 Payload SDK, PS5 OpenGL SDK 1.0.1, QuickJS at the pinned revision, PacBrew OpenSSL and the native converter from the documented boilerplate sources.
2. Copy `toolchain.example.json` to `toolchain.local.json` and set the absolute paths on your machine. The latter is ignored by Git. No Sony proprietary SDK is required by these build scripts.
3. Extract the matching release's build-assets archive at the repository root. It contains the catalog databases and redistributable runtime inputs; it is separate from Git history to avoid committing large caches. The `assets/catalog*.json` files retain their own source credits. Game payloads and personal data are not build inputs.
4. Set `RAFF_PYTHON` to Python if it is not available as `python`, then run:

```powershell
node --test tests/*.test.mjs
./tools/make-labels.ps1
node tools/build-service.mjs
node tools/build-updater.mjs
node tools/build.mjs
node tools/package-release.mjs
node tools/build-installer.mjs
```

`build.mjs --ui-qa` is a local screenshot harness, not a distributable binary. Packaging refuses a QA-mode build. The current native SDK/converter setup is configured locally; CI validates the portable policy tests, not a complete PS5 SDK build.

## Publishing a release

Run `node tools/configure-release.mjs --repo OWNER/REPO --version 1.1.1` before building public packages. Verify it matches the Git remote. Change the application/service version and release notes together; release tags must use `vMAJOR.MINOR.PATCH`. A new release must have a strictly higher version than the installed release.

The packager creates an install archive, a smaller `.raffupdate` without the cover cache, SHA-256 checksums, and a file manifest. Test the native UI, a fresh setup and an in-app upgrade with user-data preservation on a console before publishing as stable. Upload the corresponding source and required third-party source archives with the binaries, preserve license notices, and publish a GitHub Release. Uploading a commit or tag alone does not notify applications.

Create the source archive from the clean repository with `python tools/archive-release.py . public-release/Raff-v1.1.1-source.zip --source`. This includes tracked and non-ignored source files; review `git status` first. Supply the matching build-assets archive (runtime files, `assets/library.sqlite`, `assets/downloads.sqlite` and `assets/covers/`) separately, with SHA-256 entries for every artifact. Copy `docs/VALIDATION.md` into the release directory and update it with actual test results. `node tools/publish-release.mjs` requires these artifacts and an authenticated GitHub CLI; it creates a draft release for final review.

GitHub's generated asset `digest` must be present before the release is offered to clients. Draft releases can be prepared first, verified, then published. The release workflow should not contain a personal access token in source; use GitHub's scoped workflow token or the maintainer's local authenticated CLI.

The installer is built after the release bundle so that its URL, size and SHA-256 can be embedded. It does not depend on a PC server or an existing Raff service. `build-installer.mjs --isolated-test` targets `/data/raff/installer-test/app` and skips app registration; never distribute this test build. The production installer refuses to overwrite an existing application.
