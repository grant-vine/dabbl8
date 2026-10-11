// SPDX-License-Identifier: GPL-3.0-only
// Known modern INFO trailer only. Identity strings and track count grant nothing.
// The capture extension stays last for the independent read-only collector.
const TAGS = Object.freeze([
  [0, 16], [1, 0x55], [2, 1], [4, 0x4d], [5, 1], [6, 64], [7, 1],
  [8, 0x42], [9, 1], [11, 0x46], [12, 1], [14, 0], [15, 0x53], [16, 1],
  [18, 0x50], [19, 1], [21, 0x4e], [22, 1], [24, 0x52], [25, 1], [26, 4],
  [27, 0x4c], [28, 1], [29, 1], [30, 68], [31, 56], [32, 1],
]);
export function d8InfoDiscovery(trailer) {
  if (!Array.isArray(trailer) || (trailer.length !== 33 && trailer.length !== 37) ||
      trailer.some(v => !Number.isInteger(v) || v < 0 || v > 127) ||
      TAGS.some(([i, value]) => trailer[i] !== value) || trailer[3] > 15 || trailer[10] > 3) return 0;
  if (trailer.length === 37 &&
      (trailer[33] !== 67 || trailer[34] !== 56 || trailer[35] !== 1 || trailer[36] !== 78)) return 0;
  return 1; // discovery only: callers still negotiate and validate actual CAPS.
}
