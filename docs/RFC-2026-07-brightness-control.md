# RFC 2026-07: Per-Monitor Brightness Control

Status: Partly implemented (P1, P2, P4 in v26.07.29d; P3 outstanding)
Date: 2026-07-29

## Summary

hgfloater drives monitor brightness through exactly one Windows call pair,
`GetMonitorBrightness` / `SetMonitorBrightness`, and falls back to a gamma ramp
when that pair fails. That is the narrowest of the three paths Windows offers,
and it is the wrong one for two common cases: an external monitor that speaks
DDC/CI but does not implement the high-level API, and a laptop's internal panel,
which is not a DDC/CI device at all. Both currently land on the gamma fallback,
which does not change the backlight - it darkens the colours being sent to a
panel that is still burning full power.

This RFC records how brightness control actually works on Windows, so that the
implementation can be widened deliberately rather than by trial. It is written
from the Microsoft API contracts listed under References; **no source from any
other project was read into it, and none is to be transcribed while implementing
it.** The prompt for this work named an existing application as an example of
the result; the method below is derived from the documented API surface, and any
resemblance in structure is the shape the API forces on every caller.

## Motivation

Three symptoms, all reported by the same underlying gap:

- A monitor whose brightness the OSD can change, and hgfloater cannot.
- A laptop panel where the app appears to work but the room does not get
  brighter, because gamma is not backlight.
- Brightness that reads back as a coarse or wrong number, because the app
  assumes a 0..100 scale that the monitor never agreed to.

## Background: the three paths

Windows exposes monitor control at three levels. They are not alternatives to
choose between once; they are a ladder to walk down per display.

### 1. High-level monitor configuration (what hgfloater uses)

`GetMonitorBrightness` and `SetMonitorBrightness` in `dxva2.dll`, taking a
physical-monitor handle obtained from an `HMONITOR`. Simple, and it reports a
minimum, current, and maximum triple. Its weakness is that it is a convenience
layer over the low-level path: a monitor that does not satisfy whatever the
driver requires of it simply fails, with no way to ask why.

### 2. Low-level monitor configuration (VCP)

The same physical-monitor handle, driven through the VESA Monitor Control
Command Set. The documented sequence is:

1. `GetCapabilitiesStringLength`, then `CapabilitiesRequestAndCapabilitiesReply`,
   to fetch an ASCII **capabilities string**. It is static per monitor and lists,
   among other things, the VCP codes the monitor implements, in a `vcp(...)`
   section.
2. Parse that section. Brightness is **VCP code `0x10`** (luminance) in MCCS. If
   `10` is not in the list, this monitor cannot be driven this way and the ladder
   continues down.
3. `GetVCPFeatureAndVCPFeatureReply` for `0x10` returns the **current value and
   the maximum value**. Luminance is a *continuous* code, so any value from zero
   to that vendor-specific maximum is legal - and the maximum is emphatically not
   guaranteed to be 100.
4. `SetVCPFeature` writes a new value in that same vendor scale.

This path reaches monitors the high-level one does not, and it is the only one
that tells the caller the monitor's real scale.

### 3. WMI, for the internal panel

A laptop's built-in display is not reached over DDC/CI at all. Windows exposes
it through WMI in the `root\wmi` namespace:

- `WmiMonitorBrightness` - read the current brightness and the list of levels
  the panel supports.
- `WmiMonitorBrightnessMethods.WmiSetBrightness` - set it, taking a timeout and
  a brightness value.

This is the same mechanism the Windows brightness slider uses, and it moves the
actual backlight.

### 4. Gamma, which is not brightness

`SetDeviceGammaRamp` changes the values sent to the panel. It is a last resort
worth keeping - it is the only thing that works on a monitor that answers
nothing - but the RFC records it plainly: **it dims the picture, not the lamp**,
it costs contrast, and it is invisible to anything else that reads brightness.

## Design

### D1. A ladder, resolved once per display and remembered

For each monitor, the first path that answers becomes that monitor's method:

1. WMI, if this display is the internal panel.
2. Low-level VCP, if the capabilities string advertises code `0x10`.
3. High-level `Get/SetMonitorBrightness`, if it answers.
4. Gamma, always available, always last.

Probing costs DDC/CI round trips of tens of milliseconds, so it happens once per
display per enumeration and the answer is cached beside the monitor. A display
that changes identity re-probes, because the enumeration already rebuilds then.

### D2. Keep the monitor's own scale, present percent

Nothing outside the brightness module should know that a monitor's luminance
runs 0..64 or 0..255. Each display records its native minimum and maximum, and
percent-to-native conversion happens at the edge, rounding to nearest. Reading
back converts the other way. This is what makes a monitor that reports a
non-0..100 range stop showing misleading numbers.

### D3. Finer granularity than quarter steps

The current UI offers 0/25/50/75/100 in the menu and 5% on the wheel. With a
real scale available per monitor, the wheel should move in 1% steps with `Ctrl`
held for the coarse jump - or the reverse, whichever the maintainer prefers -
and the menu should keep a short list for one-click use while the wheel does
fine work. A monitor whose native range is coarser than 100 steps cannot be
finer than its own scale, and the conversion in D2 makes that automatic rather
than a special case.

### D4. Failure is a state, not a silent no-op

A display whose method is "none" should say so where it is offered, the way
`Scale (unavailable)` already does, rather than accepting a click that does
nothing. The reasons a monitor refuses DDC/CI are worth stating once in the
README: DDC/CI switched off in the monitor's own menu, a cable or adapter that
does not carry the channel, a KVM in the path, or a monitor that reports
capabilities it does not honour.

### D5. Do not write what cannot be undone

Already true of the gamma path after the v26.07.29c fix, and it generalises: no
path may leave a display in a state this app cannot restore at exit.

## Phases

- **P1 (done, v26.07.29d)** - Per-display method record and native-scale
  conversion, resolved lazily and reset when the monitor list is rebuilt.
- **P2 (done, v26.07.29d)** - Low-level VCP path with capabilities parsing,
  above the high-level one. The probe also removed the extra read per write:
  the scale is known from the probe, so a write is one round trip.
- **P3 (outstanding)** - The WMI path for the internal panel, above VCP. This is
  the laptop case, where the gamma fallback currently dims the picture and
  leaves the backlight alone. It needs COM against `root\wmi`, which is a larger
  piece than P1/P2 and has not been attempted.
- **P4 (done, v26.07.29d)** - The wheel over `B` moves brightness in 1% steps
  where opacity and volume keep their coarse 5%, and a display that has been
  probed and answered nothing shows `Brightness (unavailable)`.
- **P5 (done, v26.07.29d)** - README and SPEC.

## Non-Goals

- No C++, no external libraries, no WMI wrapper beyond the COM already linked.
- No contrast, colour temperature, or other VCP codes. Luminance only.
- No polling loop faster than the existing cache refresh; DDC/CI stays off the
  paint path.
- No attempt to make an uncontrollable monitor controllable.

## Risks

- **Capabilities parsing is string handling against vendor data.** It must be
  bounded, must not trust the reported length, and must tolerate a string that
  is malformed or truncated.
- **DDC/CI is slow and occasionally lies.** Probing must be off the UI path, and
  a monitor that reports `0x10` may still fail to set it; the ladder must fall
  through on a failed write, not only on a failed probe.
- **WMI means COM on a thread that already runs a message loop.** The app
  already initialises COM for the audio path; the brightness path must not
  assume a different apartment model than the process already has.
- **More paths means more state.** D1's cache is per display and must be
  invalidated by the same event that rebuilds the monitor list, or a reconnected
  monitor inherits the previous one's method.

## References

- Monitor Configuration Functions - the high-level, low-level, and enumeration
  function lists.
  <https://learn.microsoft.com/en-us/windows/win32/monitor/monitor-configuration-functions>
- Using the Low-Level Monitor Configuration Functions - the documented probe and
  set sequence, and the continuous versus noncontinuous VCP distinction.
  <https://learn.microsoft.com/en-us/windows/win32/monitor/using-the-low-level-monitor-configuration-functions>
- WmiMonitorBrightnessMethods - `root\wmi`, `WmiSetBrightness`.
  <https://learn.microsoft.com/en-us/windows/win32/wmicoreprov/wmimonitorbrightnessmethods>
- VESA Monitor Control Command Set, for the meaning of VCP code `0x10`.
