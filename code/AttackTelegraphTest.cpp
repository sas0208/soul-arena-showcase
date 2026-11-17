// Excerpt from Soul Arena: automation test for the pure telegraph rules (runs in the editor's Session Frontend).
// Copyright (c) 2026 sas0208. All rights reserved.

#include "Misc/AutomationTest.h"
#include "AttackTelegraph.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAttackTelegraphTest,
    "Project.Combat.AttackTelegraph",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAttackTelegraphTest::RunTest(const FString& Parameters)
{
    namespace W = AttackTelegraph;

    // Weapon flash.
    const float Life = W::FlashSeconds(0.9f);
    TestEqual(TEXT("long wind-up: full 0.32 s flash"), Life, W::FlashLife, 0.0001f);
    TestEqual(TEXT("short 0.30 s wind-up: 0.18 s flash (60%)"), W::FlashSeconds(0.30f), 0.18f, 0.0001f);
    TestEqual(TEXT("no wind-up: no flash"), W::FlashSeconds(-1.f), 0.f);
    TestEqual(TEXT("flash at 0: dark"), W::FlashEnvelope(0.f, Life), 0.f, 0.0001f);
    TestEqual(TEXT("flash half-way up"), W::FlashEnvelope(0.02f, Life), 0.5f, 0.001f);
    TestEqual(TEXT("flash full while holding"), W::FlashEnvelope(0.07f, Life), 1.f, 0.0001f);
    TestTrue(TEXT("flash fading after the hold"), W::FlashEnvelope(0.2f, Life) < 1.f && W::FlashEnvelope(0.2f, Life) > 0.f);
    TestEqual(TEXT("flash off at end of life"), W::FlashEnvelope(Life, Life), 0.f);
    TestTrue(TEXT("flash never rises again after the hold"), W::FlashEnvelope(0.15f, Life) >= W::FlashEnvelope(0.25f, Life));

    // Vignette, 0.25 s.
    TestEqual(TEXT("vignette full at 0.05 s"), W::VignetteEnvelope(0.05f), 1.f, 0.0001f);
    TestEqual(TEXT("vignette off at 0.25 s"), W::VignetteEnvelope(0.25f), 0.f);
    TestEqual(TEXT("vignette half-way down"), W::VignetteEnvelope(0.15f), 0.5f, 0.001f);

    // Enraged glow: fades in over 0.6 s, then breathes between 70% and 100%.
    TestEqual(TEXT("enraged glow at start: off"), W::FuryEnvelope(0.f), 0.f, 0.0001f);
    float Low = 1.f, High = 0.f;
    for (float T = 1.f; T < 4.f; T += 0.01f)
    {
        Low = FMath::Min(Low, W::FuryEnvelope(T));
        High = FMath::Max(High, W::FuryEnvelope(T));
    }
    TestTrue(TEXT("enraged glow breathes between 0.7 and 1"), Low >= 0.69f && High <= 1.001f && High - Low > 0.25f);

    // Colours: gold and red clearly different; None is black.
    TestTrue(TEXT("gold greener than red"), W::Color(EAttackWarning::Parry).G > W::Color(EAttackWarning::Danger).G + 0.4f);
    TestTrue(TEXT("None: black"), W::Color(EAttackWarning::None).Equals(FLinearColor::Black));

    // Phase rule.
    FBossPhaseView First;
    FBossPhaseView Rage;
    Rage.SpeedScale = 1.1f;
    FBossPhaseView Calm;
    TestFalse(TEXT("first phase: no glow"), W::PhaseGlows(First, 0));
    TestTrue(TEXT("faster second phase: glow"), W::PhaseGlows(Rage, 1));
    TestFalse(TEXT("second phase with only new moves: no glow"), W::PhaseGlows(Calm, 1));
    FBossPhaseView Tempo;
    Tempo.SpeedScale = 1.08f;
    TestFalse(TEXT("x1.08 rhythm change: no glow"), W::PhaseGlows(Tempo, 1));
    Calm.FuryGlow = 1;
    TestTrue(TEXT("FuryGlow 1: always"), W::PhaseGlows(Calm, 1));
    Rage.FuryGlow = 0;
    TestFalse(TEXT("FuryGlow 0: never"), W::PhaseGlows(Rage, 1));

    return true;
}

#endif
