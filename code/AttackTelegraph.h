// Excerpt from Soul Arena (Unreal Engine 5.8, C++). Shown as a code sample only.
// Copyright (c) 2026 sas0208. All rights reserved.
//
// Attack telegraphs: one readable on-body warning for the whole game.
//  - a short flash on the weapon at the start of a marked wind-up
//    (gold = parry it, red = dangerous, better dodge);
//  - a 0.25 s screen vignette on a boss's lethal attacks;
//  - a slow, faint "breathing" glow on the torso while a boss is enraged.
// Only the pure rules live here (curves, colours, timings), so they can be unit-tested
// without spawning anything in a level. The runtime component reads these values.

#pragma once

#include "CoreMinimal.h"

/** Which telegraph an attack shows (in the game this lives next to the move data). */
enum class EAttackWarning : uint8 { None, Parry, Danger };

/** The few fields of a boss phase this rule needs (the real struct has many more). */
struct FBossPhaseView
{
    float SpeedScale = 1.f;
    float DamageScale = 1.f;
    /** 1 = always glow, 0 = never, -1 = decide from the numbers. */
    int32 FuryGlow = -1;
};

namespace AttackTelegraph
{
    // Weapon flash: rises in 0.04 s, holds until 0.10 s, gone by 0.32 s (always before the blade lands).
    constexpr float FlashRise = 0.04f;
    constexpr float FlashHold = 0.10f;
    constexpr float FlashLife = 0.32f;
    /** The flash may use at most this share of the wind-up: it must be off when the hit arrives. */
    constexpr float FlashMaxChargeShare = 0.6f;
    // Small radius so it reads as a glint on the weapon, not as a pool of light on the ground.
    constexpr float FlashCandelas = 320.f;
    constexpr float FlashRadius = 120.f;

    // Vignette on lethal boss attacks: 0.05 s rise, gone at 0.25 s.
    constexpr float VignetteRise = 0.05f;
    constexpr float VignetteLife = 0.25f;

    // Enraged glow: faint, slow breathing (0.9 Hz between 70% and 100%), fades in over 0.6 s.
    constexpr float FuryCandelas = 45.f;
    constexpr float FuryRadius = 180.f;
    constexpr float FuryHz = 0.9f;
    constexpr float FuryFadeIn = 0.6f;

    inline FLinearColor Color(EAttackWarning Kind)
    {
        switch (Kind)
        {
        case EAttackWarning::Danger: return FLinearColor(1.f, 0.10f, 0.05f);
        case EAttackWarning::Parry:  return FLinearColor(1.f, 0.72f, 0.26f);
        default:                     return FLinearColor::Black;
        }
    }

    /** Flash length for a wind-up of ChargeSeconds: FlashLife, shortened on quick wind-ups. 0 = no flash. */
    inline float FlashSeconds(float ChargeSeconds)
    {
        return ChargeSeconds <= 0.f ? 0.f : FMath::Min(FlashLife, FlashMaxChargeShare * ChargeSeconds);
    }

    /** Flash envelope (0-1) at T seconds over a lifetime Life. Rise and hold scale with the lifetime. */
    inline float FlashEnvelope(float T, float Life)
    {
        if (Life <= 0.f || T < 0.f || T >= Life)
        {
            return 0.f;
        }
        const float Scale = Life / FlashLife;
        const float Rise = FlashRise * Scale;
        const float Hold = FlashHold * Scale;
        if (T < Rise)
        {
            return T / Rise;
        }
        if (T < Hold)
        {
            return 1.f;
        }
        const float Fade = (T - Hold) / FMath::Max(KINDA_SMALL_NUMBER, Life - Hold);
        return FMath::Square(1.f - Fade);
    }

    /** Vignette envelope (0-1) at T seconds. */
    inline float VignetteEnvelope(float T)
    {
        if (T < 0.f || T >= VignetteLife)
        {
            return 0.f;
        }
        if (T < VignetteRise)
        {
            return T / VignetteRise;
        }
        return 1.f - (T - VignetteRise) / (VignetteLife - VignetteRise);
    }

    /** Enraged glow (0-1) at T seconds after the phase starts. */
    inline float FuryEnvelope(float T)
    {
        if (T < 0.f)
        {
            return 0.f;
        }
        const float In = FMath::Clamp(T / FuryFadeIn, 0.f, 1.f);
        const float Breath = 0.85f + 0.15f * FMath::Sin(2.f * PI * FuryHz * T);
        return In * Breath;
    }

    /** A phase must be at least ~10% faster or harder-hitting to count as "enraged";
     *  below that it is just a change of rhythm and shows nothing. */
    constexpr float FuryMinScale = 1.095f;

    inline bool PhaseGlows(const FBossPhaseView& Phase, int32 PhaseIndex)
    {
        if (Phase.FuryGlow >= 0)
        {
            return Phase.FuryGlow > 0;
        }
        return PhaseIndex > 0 && (Phase.SpeedScale >= FuryMinScale || Phase.DamageScale >= FuryMinScale);
    }
}
