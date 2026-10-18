/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef NATIVE_MIDI_PROJECT_FIXTURE_H
#define NATIVE_MIDI_PROJECT_FIXTURE_H

static size_t native_midi_project_fixture(uint8_t *out, size_t capacity, const char *path)
{
    size_t size = 0;
    if (path) {
        FILE *file = fopen(path, "rb");
        assert(file);
        size = fread(out, 1, capacity, file);
        assert(!ferror(file) && fgetc(file) == EOF);
        assert(!fclose(file));
    } else {
        assert(!d8p1_capture_runtime(out, capacity, &size));
    }
    return size;
}

#endif
