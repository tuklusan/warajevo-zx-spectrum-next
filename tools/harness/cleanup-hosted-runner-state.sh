#!/usr/bin/env bash
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

set -euo pipefail

workspace_only="${1:-}"

if [ "$workspace_only" != "--workspace-only" ]; then
  : "${GH_TOKEN:?GH_TOKEN is required}"
  : "${GITHUB_REPOSITORY:?GITHUB_REPOSITORY is required}"
  : "${GITHUB_RUN_ID:?GITHUB_RUN_ID is required}"
  : "${GITHUB_REF_NAME:?GITHUB_REF_NAME is required}"
fi

workflow="platform-smoke.yml"
current_run="$GITHUB_RUN_ID"

if [ "$workspace_only" != "--workspace-only" ]; then
  echo "Cleaning prior platform-smoke runs for ref ${GITHUB_REF_NAME}; preserving run ${current_run}."
  prior_runs="$({
    gh run list --repo "$GITHUB_REPOSITORY" --workflow "$workflow" \
      --branch "$GITHUB_REF_NAME" --limit 1000 --json databaseId,status
  } | jq -r --arg current "$current_run" \
    '.[] | select((.databaseId | tostring) != $current) | [.databaseId, .status] | @tsv')"

  while IFS=$'\t' read -r run_id status; do
    [ -n "$run_id" ] || continue
    case "$status" in
      queued|in_progress)
        echo "Cancelling stale prior run ${run_id} (${status})."
        gh run cancel "$run_id" --repo "$GITHUB_REPOSITORY"
        ;;
    esac

    # Delete all artifacts for a run in one API operation. Per-artifact
    # deletion is prohibitively slow when the repository has a long backlog.
    echo "Deleting prior temporary artifacts from run ${run_id}."
    gh api --method DELETE "repos/${GITHUB_REPOSITORY}/actions/runs/${run_id}/artifacts"
  done <<< "$prior_runs"
fi

# Hosted runners are normally fresh; make reuse deterministic without touching
# checked-in source, guidance, or retained evidence.
rm -rf -- "${GITHUB_WORKSPACE:-.}/.wzsn-harness"
rm -rf -- "${GITHUB_WORKSPACE:-.}/test-artefacts/github"
if [ -n "${RUNNER_TEMP:-}" ] && [ -d "$RUNNER_TEMP" ]; then
  find "$RUNNER_TEMP" -mindepth 1 -maxdepth 1 -name 'wzsn-*' -exec rm -rf -- {} +
fi

echo "Hosted runner housekeeping passed."
