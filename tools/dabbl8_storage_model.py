#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""D8P1 worst-case arithmetic model, no firmware writes or compression assumptions."""
import json

SECTOR = 4096
JOURNAL_HEADER = 256
NOINIT = 15696
OTHER_RETAINED = 204  # baseline map: 14796 - 4 * 3648
TRACKS = 8
STEPS = 64
PARAMS = 99
MAX_BANKS = 4
MAX_SCENES = 16


def model():
    # New-format explicit bytes, not native C sizes. Chunk headers are 8 B.
    chunks = {
        "globals": 27 * 2,
        "tracks": TRACKS * (PARAMS + 2 + STEPS * 9),
        "patches": TRACKS * 128,
        "motion_D8M1": 8 + 64 * 5,
        "chain": 1 + 16 * 2,
        "metadata": 16,
        "banks": 1 + MAX_BANKS * (12 + TRACKS * 2),
        "scenes": 1 + MAX_SCENES * (12 + 1 + 1 + 1 + TRACKS * 3),
    }
    total = 32 + len(chunks) * 8 + sum(chunks.values())
    per_copy = 2 * SECTOR - JOURNAL_HEADER
    slot_cache = (total + 3) & ~3
    result = {
        "schema": "D8P1 proposal 1", "tracks": TRACKS, "steps": STEPS,
        "shared_voices": 8, "shared_motion_records": 64,
        "max_banks": MAX_BANKS, "max_scenes": MAX_SCENES,
        "file_header": 32, "chunk_header": 8, "chunks": chunks,
        "max_project_bytes": total, "two_sector_copy_payload": per_copy,
        "copy_spare_bytes": per_copy - total,
        "one_AB_project_flash_bytes": 4 * SECTOR,
        "one_AB_autosave_flash_bytes": 4 * SECTOR,
        "current_project_region_bytes": 8 * SECTOR,
        "current_autosave_region_bytes": 2 * SECTOR,
        "retained": {"capacity": NOINIT, "other_baseline_bytes": OTHER_RETAINED,
            "one_cache_total": slot_cache + OTHER_RETAINED,
            "two_caches_total": 2 * slot_cache + OTHER_RETAINED,
            "four_caches_total": 4 * slot_cache + OTHER_RETAINED,
            "two_caches_spare": NOINIT - 2 * slot_cache - OTHER_RETAINED},
        "alternatives": [
            {"projects": 2, "sample_slots": 3, "project_bytes": 8 * SECTOR,
             "autosave_bytes": 4 * SECTOR, "unallocated_autosave_deficit": 2 * SECTOR,
             "status": "projects fit original region; full atomic autosave needs an additional approved 8 KiB"},
            {"projects": 4, "sample_slots": 2, "project_bytes": 16 * SECTOR,
             "autosave_bytes": 4 * SECTOR, "sample_slot_retired_bytes": 20 * SECTOR,
             "retired_slot_used_bytes": 12 * SECTOR,
             "status": "candidate: internal remap required; no boundary/map implementation approved"},
        ],
    }
    assert total == 7705 and total > SECTOR - JOURNAL_HEADER and total <= per_copy
    assert result["retained"]["two_caches_spare"] == 76
    assert result["retained"]["four_caches_total"] > NOINIT
    return result


if __name__ == "__main__":
    print(json.dumps(model(), indent=2))
