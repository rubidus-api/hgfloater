# RFC 2026-07: CPU Temperature on the Floater

Status: Implemented (v26.07.29f)
Date: 2026-07-29

## Summary

The floater already draws CPU, memory, and battery as bars. This adds
temperature as a fourth, directly after CPU, on a fixed 20-100 degree Celsius
scale with the reading printed on the bar.

The hard part is not the bar. It is that **Windows has no dependency-free way to
read a CPU's die temperature**, and this project may not take a dependency. This
RFC records what the options actually are, picks the only one the project's
rules allow, and states plainly what that choice cannot do - because a
temperature that is quietly wrong is worse than no temperature at all.

It is written from the Microsoft and WMI class documentation cited under
References. **No source from any other project was read into it, and none is to
be transcribed while implementing it.** The prompt named an existing application
as a reference; what was taken from it is one fact - which library it uses - and
that fact is why this RFC goes a different way.

## What the reference project does, and why hgfloater cannot

The named reference reads hardware sensors through **LibreHardwareMonitor**, a
third-party open-source library, and its own README says the standard build
**requires administrator privileges** and that "hardware monitoring
functionality may have problems", with users reporting that enabling temperature
features causes crashes and hangs.

That approach is closed to hgfloater on three counts, each of which is a
standing project rule rather than a preference:

- LibreHardwareMonitor is a .NET assembly. This project is pure C against Win32
  with no external libraries and no CLR.
- It reads sensors by loading a **kernel driver** to touch MSRs and SMBus
  directly. hgfloater ships a single unsigned executable, runs unelevated, and
  installs nothing.
- The reference's own experience with it is crashes and hangs. A clock widget
  may not risk the machine to draw a number.

This is worth stating rather than implying: **the accurate path is the one we
are declining.** Anything reachable without a driver is less precise, and the
design below is honest about that instead of dressing it up.

## The dependency-free option

`MSAcpi_ThermalZoneTemperature`, in the `root\WMI` namespace. Each instance is
an ACPI thermal zone the firmware chose to expose; `CurrentTemperature` holds
**tenths of a degree Kelvin**, so

    celsius = CurrentTemperature / 10 - 273.15

hgfloater already speaks to `root\WMI` for the internal panel's backlight
(RFC 2026-07 brightness control), so the connection, its cache, and its bounded
waits already exist and are reused rather than duplicated.

### What this reading is, and is not

- It is **a thermal zone**, generally on the motherboard, near but not on the
  CPU die. It tracks CPU temperature; it does not equal it, and it usually lags
  and reads lower under a sudden load.
- **Many machines do not expose it at all**, or expose one zone that never
  moves. Firmware decides, and a lot of OEM firmware declines.
- Zone names vary (`TZ0__0`, `CPUZ_0`, `THM0_0`, and others), so a specific zone
  cannot be assumed.

The design consequence is D3 below: a machine that does not answer must show
nothing, not a zero.

## Design

### D1. Presentation: a fourth bar, second in the row

Order becomes **CPU, TMP, MEM, BAT**, so temperature sits directly after CPU as
asked. The existing panel already lays out a variable number of rows and hides
the battery row on desktops, so a fourth row costs no new layout model - only
the row array growing from three to four.

### D2. Scale: 20 to 100 degrees, fixed

Unlike the other three, temperature is not a percentage, so the bar needs a
scale of its own:

    fill fraction = (celsius - 20) / (100 - 20), clamped to 0..1

Fixed rather than adaptive: a bar whose meaning moves is a bar that cannot be
read at a glance, and the whole point of the panel is the glance. Below 20 the
bar is empty, above 100 it is full, and in both cases the printed number still
says what is actually happening.

### D3. The number is printed on the bar

The other rows are read by length alone. Temperature is not - the difference
between 55 and 75 degrees matters and 25 pixels of bar does not convey it - so
the reading is drawn as text over its own bar, right-aligned, in the same tiny
label font the row labels use.

This also settles the failure case: **when no zone answers, the row is absent
entirely**, exactly as the battery row is absent on a desktop. No zero, no
dash, no bar pinned at the bottom of the scale pretending to be a reading.

### D4. Cost

WMI is a cross-process call and the floater repaints every second, so the zone
is read once every fifth refresh rather than on every one - a die does not move
meaningfully faster than that. The paint path reads the cached number and never
WMI. A single failed read keeps the previous value rather than blanking the row;
only a run of them retires it, the shape the brightness cache settled on after
review.

Once that run has happened the asking **stops for the session**. ACPI thermal
zones are declared by firmware at boot: a machine that has none will not grow
one, and continuing to poll would spend a cross-process call on a certainty.

### D5. Configurable, like every other accent

A `stat_temp` colour in the `[colors]` section with a default, alongside
`stat_cpu`, `stat_mem`, and `stat_bat`.

## Addendum: the GPU, and why it gets a different answer

The CPU has no driver-free path to its die. The **GPU does**, and it is the one
Task Manager itself uses: `D3DKMTQueryAdapterInfo` with
`KMTQAITYPE_ADAPTERPERFDATA`, whose `D3DKMT_ADAPTER_PERFDATA` carries the
adapter's main temperature sensor in **tenths of a degree Celsius**. The
functions are exported from `gdi32.dll`, so this costs no library, no driver,
and no elevation, and it is vendor-neutral - no NVAPI, no ADL, no per-vendor
branch.

Two things had to be established rather than assumed:

- **The structure layout**, transcribed from the Microsoft DDI reference. The
  WDK header it normally lives in is not part of the mingw-w64 toolchain, so
  every field is declared by hand - the same thing this project already does for
  the undocumented DisplayConfig scaling interface.
- **`KMTQAITYPE_ADAPTERPERFDATA` is 62**, which is its position in the
  documented enumeration. That number appears in no prose documentation; it was
  confirmed from two independent published declarations of the enumeration
  before being used.

The failure mode is the safe one. If the value or the layout were wrong,
`D3DKMTQueryAdapterInfo` validates the size against the type and returns an
error rather than writing anything, so the row would be absent instead of wrong.
The same is true of the common real case: **many drivers report zero**, which is
a machine without the sensor, not a fault. Adapters are enumerated with
`D3DKMTEnumAdapters2` and every handle is closed; on a hybrid machine the
warmest adapter that answers is the one shown, because that is the one doing
work.

## Non-Goals

- No kernel driver, no MSR reads, no SMBus, no elevation. Ever.
- No .NET, no third-party sensor library, no bundled DLL.
- No vendor SDKs. NVAPI and ADL would give a better GPU number on their own
  hardware, at the cost of a hand-declared undocumented interface per vendor and
  a third for Intel; the WDDM thunk covers all of them at once.
- No fan, voltage, or per-core sensors.
- No claim that this is the CPU die temperature. It is a thermal zone, and the
  README says so.

## Risks

- **The reading may be absent.** Handled by D3: the row disappears.
- **The reading may be wrong-ish.** A zone is not a die. Documented rather than
  hidden; anyone wanting real per-core numbers needs the driver-backed tools
  this project will not become.
- **WMI on the UI thread.** Already solved for the backlight: one cached
  connection, bounded waits, and the refresh happens on the existing timer
  rather than in paint.
- **A machine that reports a constant.** Indistinguishable from a working
  sensor. Not solvable without a second source; noted in the README so a reader
  who sees a number that never moves knows what they are looking at.

## References

- `MSAcpi_ThermalZoneTemperature`, `ROOT\WMI` - class and `CurrentTemperature`
  in tenths of a degree Kelvin.
  <https://wutils.com/wmi/root/wmi/msacpi_thermalzonetemperature/>
- Microsoft Q&A on reading CPU temperature through WMI, including that the value
  reflects a motherboard area rather than the CPU itself.
  <https://learn.microsoft.com/en-us/answers/questions/4323807/how-do-i-get-exact-cpu-temps-using-cmd-wmic>
- TrafficMonitor - the reference named in the request; its README states it uses
  LibreHardwareMonitor and that the standard version requires administrator
  privileges. <https://github.com/zhongyang219/TrafficMonitor>
- LibreHardwareMonitor - the driver-backed library that approach depends on.
  <https://github.com/LibreHardwareMonitor/LibreHardwareMonitor>
- `docs/RFC-2026-07-brightness-control.md` - the existing `root\WMI` client whose
  connection this reuses.
