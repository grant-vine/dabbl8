#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Validate the selected storage geometry; never read or write a device."""
import json
import re
from pathlib import Path


def plan():
    # Existing allocations only: four historical project pairs and autosave.
    blocks = [(0x97000 + i * 8192, 8192) for i in range(4)] + [(0xE5000, 8192)]
    reserved = [(0, 0x97000), (0x9F000, 4096), (0xA0000, 0x3C000), (0xDC000, 0x4000),
                (0xE0000, 0x5000), (0xE7000, 0x15000), (0xFC000, 0x4000)]
    source = (Path(__file__).resolve().parents[1] / "firmware/src/d8p1.h").read_text()
    maximum = int(re.search(r"^#define D8P1_MAX_FILE (\d+)u$", source, re.M)[1])
    limit = int(re.search(r"^#define D8P1_LIMIT (\d+)u$", source, re.M)[1])
    sector, header = 4096, 256
    assert limit == 8192 - header
    assert len(blocks) == 3 + 1 + 1  # projects + logical autosave + write spare
    assert sum(n for _, n in blocks) == 40960
    for i, (address, size) in enumerate(blocks):
        assert address % sector == 0 and size == 2 * sector
        assert size - header >= maximum
        assert address + size <= 0x100000
        for other, length in blocks[i + 1:] + reserved:
            assert address + size <= other or other + length <= address
    return {
        'schema': 'dabbl8-selected-storage-plan', 'version': 1,
        'selected_date': '2026-10-10', 'projects': 3, 'user_samples': 3,
        'logical_autosaves': 1, 'shared_write_spares': 1,
        'per_object_ab_pairs': False, 'total_bytes': 40960,
        'block_bytes': 8192, 'payload_offset': header,
        'payload_capacity': 8192 - header, 'maximum_d8p1_bytes': maximum,
        'project_reference_mask': 7,
        'blocks': [{'index': i, 'existing_address': a, 'bytes': n} for i, (a, n) in enumerate(blocks)],
        'block_ownership': 'dynamic object ID/generation, not fixed per-object addresses',
        'save_policy': 'stopped; payload first, commit header last; reclaim previous only after validation',
        'migration_required': True, 'flash_backend_implemented': False,
        'hardware_qualified': False,
    }


if __name__ == '__main__':
    print(json.dumps(plan(), indent=2))
