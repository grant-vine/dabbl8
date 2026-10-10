# Implementation backlog

All 19 implementation tasks are loaded into GitHub Issues. [Roadmap #2](https://github.com/grant-vine/dabbl8/issues/2) groups the phases. Start with [baseline build #3](https://github.com/grant-vine/dabbl8/issues/3).

GitHub Issues are authoritative for progress. Dependency links are planning gates, not automatically enforced native blocking relationships. Phase identifiers M0–M5 are not configured GitHub milestone objects.

| Plan ID | Phase | Issue | Prerequisites |
|---|---|---|---|
| D8-001 | M0 | [#3 Reproduce pinned upstream build](https://github.com/grant-vine/dabbl8/issues/3) | None |
| D8-002 | M1 | [#4 Freeze historical project layouts](https://github.com/grant-vine/dabbl8/issues/4) | [#3](https://github.com/grant-vine/dabbl8/issues/3) |
| D8-003 | M1 | [#5 Version automation addresses](https://github.com/grant-vine/dabbl8/issues/5) | [#4](https://github.com/grant-vine/dabbl8/issues/4) |
| D8-004 | M1 | [#6 Define capability and protocol handshake](https://github.com/grant-vine/dabbl8/issues/6) | [#5](https://github.com/grant-vine/dabbl8/issues/5) |
| D8-005 | M1 | [#7 Model D8P1 maximum storage](https://github.com/grant-vine/dabbl8/issues/7) | [#4](https://github.com/grant-vine/dabbl8/issues/4) |
| D8-006 | M2 | [#8 Specify bounded voice allocation](https://github.com/grant-vine/dabbl8/issues/8) | [#3](https://github.com/grant-vine/dabbl8/issues/3) |
| D8-007 | M2 | [#9 Enable eight track sequencing and MIDI](https://github.com/grant-vine/dabbl8/issues/9) | [#5](https://github.com/grant-vine/dabbl8/issues/5), [#6](https://github.com/grant-vine/dabbl8/issues/6), [#8](https://github.com/grant-vine/dabbl8/issues/8), [#7](https://github.com/grant-vine/dabbl8/issues/7) |
| D8-008 | M2 | [#10 Build eight track mixer and selection](https://github.com/grant-vine/dabbl8/issues/10) | [#9](https://github.com/grant-vine/dabbl8/issues/9) |
| D8-009 | M2 | [#11 Update editor and backup conversion](https://github.com/grant-vine/dabbl8/issues/11) | [#6](https://github.com/grant-vine/dabbl8/issues/6), [#7](https://github.com/grant-vine/dabbl8/issues/7), [#9](https://github.com/grant-vine/dabbl8/issues/9) |
| D8-010 | M3 | [#12 Measure and bound engine state](https://github.com/grant-vine/dabbl8/issues/12) | [#9](https://github.com/grant-vine/dabbl8/issues/9) |
| D8-011 | M3 | [#13 Implement atomic multi-sector saves](https://github.com/grant-vine/dabbl8/issues/13) | [#7](https://github.com/grant-vine/dabbl8/issues/7), [#12](https://github.com/grant-vine/dabbl8/issues/12) |
| D8-012 | M3 | [#14 Prepare recovery and first target alpha](https://github.com/grant-vine/dabbl8/issues/14) | [#11](https://github.com/grant-vine/dabbl8/issues/11), [#13](https://github.com/grant-vine/dabbl8/issues/13), [#10](https://github.com/grant-vine/dabbl8/issues/10) |
| D8-013 | M4 | [#15 Add bounded performance arrangement](https://github.com/grant-vine/dabbl8/issues/15) | [#14](https://github.com/grant-vine/dabbl8/issues/14) |
| D8-014 | M4 | [#16 Evaluate weighted FM voice budget](https://github.com/grant-vine/dabbl8/issues/16) | [#14](https://github.com/grant-vine/dabbl8/issues/14) |
| D8-015 | M4 | [#17 Audit selected donor feature](https://github.com/grant-vine/dabbl8/issues/17) | [#14](https://github.com/grant-vine/dabbl8/issues/14) |
| D8-016 | M5 | [#20 Qualify and publish release](https://github.com/grant-vine/dabbl8/issues/20) | [#10](https://github.com/grant-vine/dabbl8/issues/10), [#11](https://github.com/grant-vine/dabbl8/issues/11), [#13](https://github.com/grant-vine/dabbl8/issues/13), [#14](https://github.com/grant-vine/dabbl8/issues/14), [#15](https://github.com/grant-vine/dabbl8/issues/15), [#18](https://github.com/grant-vine/dabbl8/issues/18), [#19](https://github.com/grant-vine/dabbl8/issues/19) |
| D8-017 | M0 | [#18 Establish CI and evidence reporting](https://github.com/grant-vine/dabbl8/issues/18) | [#3](https://github.com/grant-vine/dabbl8/issues/3) |
| D8-018 | M3 | [#19 Define fork identity and license provenance](https://github.com/grant-vine/dabbl8/issues/19) | [#6](https://github.com/grant-vine/dabbl8/issues/6), [#14](https://github.com/grant-vine/dabbl8/issues/14) |
| D8-019 | M5 | [#21 Prepare Dabbl8 information for dabbl.co.za](https://github.com/grant-vine/dabbl8/issues/21) | [#20](https://github.com/grant-vine/dabbl8/issues/20) |

Implementation steps and acceptance criteria live in the issue bodies. backlog.json preserves the original plan IDs and links to the corresponding issues; it does not mirror live completion status automatically.

## Owner requested extensions

- [Software voice cards #53](https://github.com/grant-vine/dabbl8/issues/53): expanded resource research, test matrix and staged profile/exclusion/catalogue implementation; supports #12/#21 without replacing the original 19-task planning snapshot.
