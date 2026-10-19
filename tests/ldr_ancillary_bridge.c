/* SPDX-License-Identifier: GPL-3.0-only */
/* Keep the real loader's fixture in a separate translation unit from the app.
 * Every flash primitive is the existing in-memory adapter; no device I/O. */
#define main unused_loader_fixture_main
#include "ldr_test.c"
#undef main

int ancillary_seed_app(uint8_t *image, size_t bytes, const char *package)
{
    size_t length;
    uint8_t *data = load_logical(package, &length);
    uint32_t off = flash_off(data);
    int valid = bytes == sizeof nor && off && off <= length &&
                0x93000u <= length - off;
    if (valid) memcpy(image, data + off, 0x93000u);
    free(data);
    return valid ? 0 : -1;
}

int ancillary_upgrade(uint8_t *image, size_t bytes, const char *package)
{
    if (bytes != sizeof nor) return -1;
    memcpy(nor, image, bytes);
    logical = load_logical(package, &logical_len);
    int rc = ldr_session();
    memcpy(image, nor, bytes);
    free(logical);
    logical = NULL;
    return rc || bad_range ? -1 : 0;
}
