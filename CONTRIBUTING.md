# Contributing

Keep the native render loop free of file and network work. Preserve per-game transfer identity, download destinations and local user data. Run the relevant service tests before changes and verify native UI changes on a console. New release/update code must test corrupt input, path traversal, interrupted transfers and data preservation.

Use docs/BUILD.md for the toolchain and release checklist. Do not include private logs, game payloads, BIOS, keys or session tokens in patches. Report version, configuration and reproduction steps with sensitive values removed.
