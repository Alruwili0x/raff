# Third-party notices

1.0.0 adds metadata references from r-roms.github.io and Minerva Archive. Exact source pages, HTTPS metadata URLs, torrent infohashes and metadata SHA-256 values are retained in `assets/game-torrents.json`; no collection payloads are bundled. Game names, releases and artwork keep their respective source rights. The linked data is not licensed under Raff's GPL merely by appearing in a catalog. Emulator and free-game release provenance and permission evidence are in `assets/hub.json`.

BitTorrent runs in the user's separate aria2 1.37.0 process (GPL-2.0-or-later); the unmodified engine binary is bundled as a separate executable in runtime/ (1.1.0 and later). `tools/torrent-admin.mjs` and the bounded metadata readers are local Raff code. WebTorrent was used only in a temporary local CC0 verification fixture, not linked or shipped as the console engine.

Raff's native application and service are GPL-3.0-or-later. Retained notices in individual source files apply. This directory includes source and local build scripts; shared/ contains the companion source required to rebuild the service.

- Native UI, fonts loader, platform and runtime integration: BlackBearReloaded's ps5-homebrew-ui, GPL-3.0-or-later. The native converter and startup derive from ps5-native-app-boilerplate and SharpProspero. See `licenses/ps5-homebrew-ui.txt`, `licenses/native-boilerplate-NOTICES.md`, and exact revisions in `third_party/SOURCES.json`.
- PS5 OpenGL SDK 1.0.1: GPL-3.0-or-later project code plus Mesa, OpenGNM and other separately licensed components. Its complete distributed license notices are retained in `licenses/ps5-opengl/` and `licenses/ps5-opengl-NOTICES.md`. Its SDK release includes corresponding dependency sources and patches.
- QuickJS: Fabrice Bellard and Charlie Gordon, MIT. See `licenses/QuickJS.txt`.
- nlohmann/json 3.12.0: Niels Lohmann, MIT. See `licenses/nlohmann-json.txt`.
- stb_image: Sean Barrett and contributors, MIT/public domain. See `licenses/stb_image.txt`; the exact vendored file is pinned by SHA-256.
- LLVM compiler runtime, emulated TLS, libc++, libc++abi and libunwind: Apache-2.0 with LLVM exception/component notices. See `licenses/LLVM.txt`. Emulated TLS source is from llvmorg-18.1.8.
- Zig compiler runtime: Zig project contributors, MIT. See `licenses/Zig.txt`.
- Inter font: retained font license in `assets/fonts/`. Fixed Arabic labels are rendered during the local Windows build using Segoe UI; the Segoe UI font file is not distributed.
- PS5 Payload SDK: John Törnblom and contributors, GPL-3.0-or-later with component-specific BSD/LLVM licenses. Homebrew SDK, not Sony's SDK.
- Clean-room `libc.prx`: the runtime described by the native boilerplate, verified against its recorded SHA-256 `e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036`. The local packaging script extracts the matching runtime from the previously obtained PS5 RetroArch archive.

Catalog descriptions, ratings, source statistics and game cover art retain their respective owners' rights. Source metadata is not a license to redistribute games. No game images or BIOS binaries are included in this application archive.

Artwork metadata and alternate PS2 cover matching also use libretro-thumbnails/Sony_-_PlayStation_2. Switch icons use Nintendo images referenced by blawar/TitleDB. PS5 catalogue metadata uses saawant12/orbit-store-ps5's public catalogue-v2.json; Orbit is GPL-3.0. No Orbit executable code is incorporated in Raff. Game artwork retains its respective owners' rights.

Additional catalogue metadata: Pegasus PFS public catalogue and M3hmetSa1t/pegasus-ps4-collection-catalog (MIT, retained in licenses/Pegasus-PS4-Catalog.txt). PS4 upstream file and icon metadata credits Maelly Pooh / Software Capsules. DLPSGame via Pegasus was researched but its browser-verification host links are not bundled as direct download URLs.

PlayStation Store user ratings and PlayStation Blog monthly regional download rankings: links, observation dates and periods are retained in assets/playstation-metadata.json. Public Archive entries were also located using game links from user-provided Telegram exports; private message text, participant data, and channel links are not shipped. PS5 finalized-image header validation was checked against the WFM package reader and LibProsperoPKG format documentation.

Classic 0.8.1 metadata additions:
- Libretro database, CC BY-SA 4.0: https://github.com/libretro/libretro-database/tree/fbeefcb46c2e1b20a7e2945f34a694a41b2d6f90 . This release filters the database to the requested systems, normalizes release qualifiers, groups matching game identities, and retains source identifiers. The derived Libretro metadata remains CC BY-SA 4.0; see licenses/Libretro-database.txt. The older separately sourced catalog retains its respective source terms.
- Libretro core-info, MIT: commit 5a74858ab2f7a50cebb5a6330895bc38899531c0, licenses/Libretro-core-info.txt.
- SQLite 3.53.4 amalgamation: public domain, retained source header in third_party/sqlite/sqlite3.c.
- HarfBuzz 12.3.2: license in licenses/HarfBuzz.txt. Arabic font support incorporates the GPL code from ProsperoEden commit da6fd0eeaa762c52b7c4e9356adb7b1dfbbeba36; source headers retain attribution.
- DejaVu Sans: assets/fonts/DejaVu-LICENSE.txt. Fixed Classic Arabic interface labels continue to use the baked label atlas.
- OpenSSL 3.5.2, Apache-2.0: licenses/OpenSSL.txt; static target build from PacBrew 0.40.2 (archive SHA-256 a85f65de418a8e6a898c6c3e3c870d50fff7618a200e4dd59ea9692af6ecec4d). Classic links its MD5 support; it no longer links the experimental curl probe.
- Libretro thumbnails provides on-demand game artwork. Artwork remains with its rights holders and is not permission to distribute game content.


Xbox artwork added in 0.8.4: libretro-thumbnails/Microsoft_-_Xbox, libretro-thumbnails/Microsoft_-_Xbox_360, and public box art served by download.xbox.com. Artwork remains the property of its respective rights holders and is not covered by Raff’s source-code license. Xbox disc partition detection follows the documented XDVDFS structure and partition offsets in xenia-project/xenia (disc_image_device.cc); firmware and games are not included in Raff releases.


Emulator identity artwork in 0.8.5 is copied from the user's installed RetroArch, PS5SX2, ProsperoEden, XPSemu and PS5X360 application icons. Exact source title paths and SHA-256 digests are recorded in assets/emulators/provenance.json. These identify the corresponding emulators and retain their respective owners' rights; Raff does not claim these marks as its own. The Raff shelf mark and app icon are rendered from the editable local tools/make-brand.ps1 source.


0.8.6 platform identification marks: Simple Icons 9.21.0 (CC0-1.0), with exact source URLs and hashes in assets/platforms/provenance.json. PlayStation, Nintendo and Xbox trademarks remain their owners' property. Wide artwork is cached from the metadata links published by BlackBoxPS5/blackbox and saawant12/orbit-store-ps5, primarily PlayStation/ProsperoPatches artwork. Metadata source associations preserve both catalog credits without duplicating downloads. Pegasus DL default feeds and DLPS/Pippo (credited by Spectrum Library) were reviewed. Reports distinguish already-present files, new verified options, and unsupported browser-verification pages. No third-party UI or download-engine implementation was copied.

- PS3 cover artwork: GameTDB, exact image URLs in `assets/ps3-art-provenance.json`; rights belong to the respective owners. PS3 platform icon: Simple Icons 9.21.0, CC0, provenance in `assets/platforms/provenance.json`.

Artwork cache expanded in 1.0.1 using the public image repositories libretro-thumbnails/Sony_-_PlayStation (ccee75c7744d81676b6725307aca27ef6be6231a), libretro-thumbnails/Sony_-_PlayStation_2 (9be559835a99124720badc6ec3ec4f3f2351deb3), libretro-thumbnails/Microsoft_-_Xbox (de3d91e3b29f5e8ba2f12a7b0c9add22da726d2b), xenia-manager/x360db, and aldostools/Resources (dcec207de9de550c1192420df16cc7e86957f942). GameDB-PS3 by niemasd provides title-to-serial metadata for exact platform/title artwork matching. Images were resized to bounded JPEGs for the console cache. Artwork and logos remain the property of their respective owners. No executable code from these artwork repositories is incorporated.

PSXS5 integration in 1.0.1 follows the documented library.txt and covers conventions of SynoPiia/PSXS5 v2.0.0. The emulator is installed separately; its binary, games and firmware are not bundled in Raff.

1.1.0 bundles the separately executed aria2, Web File Manager and 7-Zip Helper payloads, and the Mozilla CA bundle. Exact runtime source archives are listed in third_party/runtime-sources.json; all component notices are in licenses/runtime. Publish the matching source archives alongside binary releases. No game binaries or console firmware are included.

1.1.1 standalone installer links PacBrew libcurl 8.18.0, OpenSSL 3.5.2, libssh2, libpsl, zlib and zstd. It uses the GPL-3.0-or-later console_curl compatibility layer by BlackBearReloaded from the pinned native boilerplate (original headers retained). It embeds the Mozilla CA bundle. Installer component notices are retained in licenses/installer, licenses/OpenSSL.txt and licenses/runtime. Versions: libssh2 1.11.1, libpsl 0.21.5, zlib 1.3.2, zstd 1.5.6.
