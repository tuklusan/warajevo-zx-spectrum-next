<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Hosted private-media playback

The hosted Linux contract checks every authorized TAP/TZX input in the
temporary runner checkout. Production parsers expand each file to canonical
master-tick EAR segments; the production machine mount, transport, motor-stop,
and playback state then consume the full segment timeline. The test reports
aggregate counts and hashes only its public harness/source fixtures. Media
paths, names, hashes, and bytes are not emitted in logs or proof artifacts.

This exercises tape transport playback, not ROM-driven loading or gameplay.
The separate DIZZY4K normal-ROM contract and Fuse parser comparison remain
distinct evidence; this is not physical hardware validation.

## Review adjudication

The automated review warnings about scan cleanup were non-actionable: Ubuntu glibc cleans an unsuccessful `scandir` result, the output pointer starts null, successful iteration frees each entry once, and the pointer array is freed and nulled before shared cleanup. The proof-key warning was non-actionable because a missing file count defaults below the required minimum and short-circuits before later direct indexing.
