#pragma once

namespace signal_residue {

constexpr int FragmentCount = 8;
constexpr float ImmediateCaptureCharge = 10.0f;
constexpr float ChargePerFragment = 0.5f;
constexpr float TotalRecoverableCharge = FragmentCount * ChargePerFragment;
constexpr float LifetimeSeconds = 10.0f;
constexpr float AttractionRadius = 3.2f;
constexpr float CaptureRadius = 0.24f;
constexpr float PullSpeed = 7.5f;

static_assert(ImmediateCaptureCharge + TotalRecoverableCharge == 14.0f,
    "Signal residue must redistribute, not inflate, the established capture reward.");

}  // namespace signal_residue
