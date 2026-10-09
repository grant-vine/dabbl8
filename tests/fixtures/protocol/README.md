# Legacy editor compatibility observer

`legacy-info-parser.js` contains the unchanged Reader and INFO parsing code from audited Felucca v1.1.5 commit `276f72a4e6ea8a12499a7a6819aadf3165126755`, extracted from `web/editor.html`. It retains the upstream license/copyright. Its purpose is to read the new firmware handler's actual INFO payload with an old client parser, independently of the changed editor. Do not update it to make a changed INFO layout pass.

The original Reader's unused `v()` references the original value decoder; INFO uses only `b()` and `s()`. The enclosing minimal CMD/parse binding is an extraction harness. Firmware replies are emitted by `editor_test` with `D8INFO_JSON`; web tests compare all original fields with and without the appended discovery tag.
