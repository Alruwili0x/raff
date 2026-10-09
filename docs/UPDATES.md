# Updates and recovery / التحديث والاسترجاع

The trusted repository is compiled into `assets/update-config.json`. Public builds must set `repository` to the maintainer's `owner/repo`. An empty repository disables network checks and shows a clear unconfigured state. No access token is distributed.

The service asks GitHub's latest-release API for a stable semantic version. Drafts, prereleases, equal versions and downgrades are ignored. It accepts only the expected `Raff-vX.Y.Z.raffupdate` asset from that repository and requires GitHub's `sha256:` asset digest. TLS certificate verification is mandatory. A SHA-256 checksum detects corruption; it is not an independent publisher signature. The repository's security is part of the trust boundary.

Checks run about every six hours with jitter, persisted state and error backoff. Manual checks are limited to once a minute. Many clients behind one public IP can encounter GitHub's anonymous API limits; they retain the installed app and retry later. References: [GitHub releases API](https://docs.github.com/en/rest/releases/releases#get-the-latest-release), [API rate limits](https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api).

Download and verification run outside the render thread. The custom bundle accepts only fixed application directories, sorted unique paths, regular files and bounded sizes. It rejects traversal, hidden paths, links, corrupt hashes and missing required app files. The independent installer waits for the frontend and service to close. It retains the prior app at `/data/raff/native-v5/updates/previous-app` and preserves the cover/media caches using hard links where supported, or bounded local copies on filesystems without hard-link support.

Executable files and PRX modules are staged with mode `0755`; data files use `0644`, and directories use `0755`. Copy fallback for artwork requires free space in addition to the update-package estimate.

The update does not replace `/data/raff/native-v5`, emulator directories, firmware, game folders, system databases or jailbreak settings. Updating runtime payloads already copied to `/data/raff` is a separate maintenance operation; existing payload files are preserved by first-run setup.

## Recovery

- If download or verification fails, the installed app remains unchanged. Retry from About & updates after restoring the network or freeing space.
- If the app is still open, the installer times out after three minutes. Fully close it before retrying.
- On a normal application-directory replacement error, the helper attempts to restore the previous directory and records the result in `updates/result.json`.
- A power loss between the two directory renames can require recovery through Web File Manager. With Raff closed, if `/data/homebrew/PPSA99178` is missing and `updates/previous-app` contains the prior app, move that directory back to `/data/homebrew/PPSA99178`. Keep personal data in `native-v5`. Remove stale `updates/armed` only after verifying that no updater is running, then reopen Raff.
- To roll back manually, first close Raff and pause its downloads. Back up the current app folder, restore `previous-app` to its original title directory, and reopen it. Keep your personal data backup; future major releases may require a documented database migration rollback.

لا تطفئ الجهاز أثناء التثبيت. عند فشل التحميل تبقى النسخة الحالية. عند انقطاع الكهرباء أثناء استبدال المجلد قد يلزم استرجاع مجلد `previous-app` من مدير الملفات. لا تحذف مجلد بياناتك `native-v5`.
