# Editor capability contract correction

PR #123 CI run 38143032462 failed the old editor assertion twice: schema byte 0 now receives unsupported-version reason 2, rather than bounds reason 3. The production handler is unchanged. This test correction separately checks supported schema 2 with trailing arguments (bounds reason 3), the exact read-only schema-2 payload, and unchanged project state. Schema-1 flags remain 3 for four-track legacy compatibility and 5 for eight-track native compatibility.

All 16 focused executions of the actual editor capability test function passed (4/8 tracks × FM4 off/on × SLICE off/on × normal/inherited sanitizer). The complete inherited four-track editor test also passed normally and under the inherited sanitizer, for 18 executions total. Commands, raw logs and source snapshot are retained here. Generated headers came from the existing uac-array-bounds build; this is a host test, not new target firmware evidence. Sanitizer exclusions are unchanged and listed in results.json.

Initial driver setup failures (missing generated-header path, wrong main guard, and the four-track-only flag expectation revealed by the expanded matrix) are preserved separately from final passing logs. No golden hashes, firmware production code, loader, flash boundaries or device were changed. Full CI for the corrected source remains required.
