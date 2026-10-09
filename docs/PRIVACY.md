# Local data and connections

Raff stores settings, library state and transfer history on the user's console in `/data/raff/native-v5`. Its frontend API binds to loopback and requires a per-session token. aria2 is configured for local RPC access on fresh installations. Existing third-party service configuration is preserved.

There is no Raff account, hosted user database, analytics endpoint or embedded GitHub token. Update checks contact GitHub and release assets may redirect to GitHub's distribution hosts. Download sources, image providers and BitTorrent peers see the connections necessary for those transfers; they have their own policies. Torrent transfers can upload pieces to peers after the in-app notice is accepted.

Do not attach `settings.json`, `jobs.json`, `user-library.sqlite`, session/RPC tokens or complete device logs to public issues. A useful bug report includes Raff version, firmware, loader versions, the visible error and steps to reproduce, with personal paths and account data removed.
