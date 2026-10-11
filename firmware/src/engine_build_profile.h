/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Dabbl8 contributors */
#ifndef DABBL8_ENGINE_BUILD_PROFILE_H
#define DABBL8_ENGINE_BUILD_PROFILE_H
/* Immutable curated build selection. It grants no storage/hardware authority.
 * 0 FULL retains all inherited build switches/state, 1 Core, 2 Breaks.
 * Arbitrary generated card headers remain unsupported. */
#if defined(D8BUILD_HAS) || defined(D8BUILD_POOL_HAS) || defined(D8BUILD_MASK)
#error "Engine profile internals cannot be externally overridden"
#endif
#if defined(D8_PROOF_PROFILE) || defined(D8_PROOF_TRANSLATION_UNIT) || defined(D8_PROOF_HAS) || defined(D8_PROOF_POOL_HAS)
#error "Private proof overrides are not firmware profile inputs"
#endif
#ifndef D8BUILD_PROFILE
#define D8BUILD_PROFILE 0
#endif
#if D8BUILD_PROFILE < 0 || D8BUILD_PROFILE > 2
#error "Unsupported engine build profile"
#endif
#if D8BUILD_PROFILE != 0 && (NPART != 8 || FELUCCA_FM4 || !FELUCCA_SLICE)
#error "Curated engine profiles require eight tracks, retired DIGITAL and stable SLICE slot"
#endif
_Static_assert(D8BUILD_PROFILE >= 0 && D8BUILD_PROFILE <= 2, "Unsupported engine build profile");
#define D8BUILD_MASK (D8BUILD_PROFILE == 1 ? 0x1409u : 0x3411u)
/* Full path retains even historically allocated unused members. */
#define D8BUILD_HAS(e) (D8BUILD_PROFILE == 0 || ((D8BUILD_MASK & (1u << (e))) != 0))
#define D8BUILD_POOL_HAS(e) D8BUILD_HAS(e)
#endif
