# Preserved initial focused failures

v1: seven failures, including obsolete fixtures that treated supported apply bits as unsupported. Updated fixtures to unknown bits8/16/32.
v2: compile failure from nonexistent GR_MIXER; actual page enum GR_TRK substituted.
v3: two encoder-routing failures from a duplicated encoder mapping; corrected to a valid K2/K3 permutation.
v4: optimized619507 PASS.
v5: optimized/inherited sanitizer619507 each PASS after moving416-byte overlay cache to existing fourth-source extent.
v6: one newly added Stop gate fixture failed: actual ownership survived boundary but other sequence voices could steal its sounding voice under the eight shared-voice limit. Fixture now renews actual panel input after boundary and separately verifies Stop/key-up gate preservation. No production fix made for this fixture failure.
v7: optimized/inherited sanitizer625904 each PASS. Inherited sanitizer exclusions and detect_leaks=0 remain explicit, not full undefined-behavior coverage. Virtual NOR only, no hardware.
