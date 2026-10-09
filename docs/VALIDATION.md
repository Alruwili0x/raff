# Raff 1.1.1 validation

Tested on 9 October 2026 on one PS5 with firmware 13.60 and an existing homebrew environment.

## Completed

- 133 local service/policy tests passed. The portable GitHub Actions workflow also passed.
- Release-mode native frontend, service, independent update helper and standalone ELF installer built successfully.
- Seven controller-navigation checks passed on the console: manufacturer filter, opening PS4, returning home, settings, next page, page boundary and header navigation.
- Arabic/English dashboards, PlayStation/Nintendo filters, credits and emulator details were captured on the console and visually reviewed.
- The installer downloaded the actual pinned GitHub release over verified HTTPS, checked the package and individual files, and installed into an isolated console test directory. Selected installed binary/configuration hashes matched the release manifest.
- The production installer detected the existing application, produced a valid status receipt, and left the application byte-identical.
- The installed 1.1.0 service found the GitHub release, downloaded and verified its asset through the same local update API used by the UI. The first apply attempt stopped safely when a resident service was detected during preparation. After explicitly stopping that service, the verified stage resumed and the regular 1.1.1 application reopened. The new service binary was then installed before launch and compared with the final release manifest. A fresh full upgrade with the final guard enabled at both ends has not been run.
- The final service refuses startup while an update is armed; this guard passed an on-console launch test. A regression test also verifies shutdown when the helper has advanced to artwork preservation.

- The new service reports version 1.1.1 and ready status. Settings were preserved byte for byte; 88 existing transfer records remain. Program binary hashes match the public update package.
- Native archive tests also covered traversal, duplicates, corrupt file digest, missing required files and truncation. Executable/PRX modes are 0755 and data files 0644.
- ZIP CRCs, bundle SHA-256, configured repository, source exclusions and build database inputs were verified.

## Scope and limitations

The first-install test was isolated on the development console, not on a second freshly reset physical console. Existing helper setup was tested separately for idempotence. Other firmware and loader configurations remain unverified. Rendering observed during dashboard QA was approximately 55–58 fps including screenshot capture; this is not a performance guarantee or multi-user load test.

The installer requires an existing homebrew environment and native application loader. Raff does not install a jailbreak, games, BIOS or keys. Game/emulator compatibility and external provider availability vary. GitHub HTTPS and the configured repository are the update trust boundary; hashes are integrity checks, not independent signatures.
