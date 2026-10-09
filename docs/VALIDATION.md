# Raff 1.1.1 validation

Development console: PS5 firmware 13.60, working homebrew environment. Results apply to this configuration, not every console or emulator.

## Verified

- The native frontend, service, update helper and standalone installer compile successfully.
- Seven native controller-navigation checks passed: manufacturer filter, entering PS4, returning home, settings, page movement, page boundaries and header navigation.
- Arabic and English system dashboards, Nintendo/PlayStation filters, credits and emulator details were captured on the console and visually reviewed.
- Emulators no longer expose the Sources tab or raw source links. Internal provider metadata remains available to the download service.
- Prior native update tests passed for a valid archive, traversal, duplicate names, corrupt file digest, missing required files and truncation, including executable mode preservation.
- Existing first-run helper setup was tested in an isolated directory and preserves existing configuration on repeat.
- The previous application has an independent verified backup; settings and game folders are outside the application update directory.

## Release validation in progress

The public repository is configured as Alruwili0x/raff. The maintainer is testing the first-install ELF against the published package in an isolated console directory, then the actual GitHub update flow from 1.1.0 to 1.1.1. Final results will accompany the release.

Rendering observed during dashboard QA was approximately 55–58 fps, including screenshot capture; this is a single-console observation. No clean physical second console or multi-user load test has been performed.

The installer does not provide a jailbreak and requires an existing native app loader. It never overwrites an existing Raff application. Update checks rely on the configured GitHub repository and verified HTTPS; hashes do not constitute an independent signature.
