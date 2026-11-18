<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Additional private TAP playback

A separately supplied private TAP was transferred through a temporary Actions
secret to a hosted Linux runner, decoded only in a runner temporary directory,
and exercised with the existing `wz_private_media_playback` core regression.
The run passed one TAP: 220,632 segments, 220,631 signal edges, two motor stops,
and 590,004,768 master ticks. The proof contains aggregate counts and source
fixture hashes only. The temporary secret and runner media file were deleted.
