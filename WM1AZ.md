# WM1A / WM1Z display adaptation — development build

This branch targets the **first-generation NW-WM1A and NW-WM1Z**. Development
build **1.14.7-wm1az.1** includes a compiled ARM application, a Windows installer,
a Walkman One service UPG and the upstream Walkman One uninstaller. The build
and package checks passed; installation on a physical device is not yet validated.

## Display and input

Both models have a 4.0-inch **854 × 480 (FWVGA)** capacitive TFT display
([Sony specifications](https://helpguide.sony.net/dmp/nwwm1/v1/en/contents/TP0001449078.html)).
In portrait panel coordinates this is 480 pixels wide and 854 pixels high;
Wampy's landscape coordinates are 854 × 480.

Existing skins retain their 800 × 480 logical canvas. On an 854 × 480 panel,
full screen expands the horizontal axis by **854 / 800 = 1.0675**. This first
step slightly stretches artwork; it does not redesign every skin for native
854-pixel layout. Other panel sizes keep their existing layout.

The OpenGL backend applies the rotation and scaling to every draw list and
scissor rectangle, including popups and renderer-state resets. GLFW touch input
uses evdev axis limits, then the ImGui backend maps physical positions back to
the same logical canvas. The polling path applies the same conversion as the
cursor callback. If axis limits cannot be queried, the reader logs a fallback
to the existing two-times-resolution convention. The existing event device
paths and screen orientation have not been changed; they need hardware checks.

In Settings → Misc, **WM1A/Z full screen** toggles between full screen and the
800 × 480 layout. The previous Left / Center / Right choice is retained and
becomes available again when full screen is disabled. The setting is saved as:

```ini
[wampy]
wm1azFullscreen=1
```

Use `0` for the previous layout. A missing setting defaults to `1`; the option
only takes effect on an 854 × 480 panel. Simplified and Traditional Chinese
labels are included. The optional 24 fps cap now sleeps for the remainder of
the current frame instead of spinning the CPU or catching up after screen wake.

## Supplied Walkman One package

The package was inspected as data; its Windows executable and embedded scripts
were not executed. No Sony binaries or firmware images are included here.

| Item | Static finding |
| --- | --- |
| Installer | `Walkman_One_WM1_(21_09).exe` |
| Installer SHA-256 | `76287ea28db9f1ac275a06be496eb7105136e93b750cd019cc1f387f91cf0280` |
| Active HgrmMediaPlayerApp MD5 | `7a8ed50c18ca13fd914e0aa9e2bbbbea` |
| AudioAnalyzerService MD5 | `9bde2430bcf44298c650744416bc4e9d` (recognized by upstream installer) |
| ICU data | `libicudata.so.61` is present |
| Main system image | Its MD5 differs from the inner manifest; cause unresolved |

The active Sony player hash differs from the A50 Walkman One binary recognized
by the upstream CUE-icon patch. Do not copy the A50 fixed offsets into this
binary. The existing installer skips that unknown hash. Static file matches
do not establish runtime ABI or installation compatibility. The main-image
manifest discrepancy must be resolved before repacking this supplied firmware.

This change leaves the playback, DSP, tuning and USB audio code unchanged.
The upstream installer still modifies platform and preload components; this
branch has not validated those changes on WM1A/Z. The optional LLUSBDAC module
is a separate PCM path and is not a replacement for Sony's USB DAC / DoP path.

## Checks and build status

The development build also fixes multilingual metadata handling. Glyph collection
includes albums and filenames, malformed UTF-8 always advances the decoder,
and bounded text copies preserve whole characters. Uppercase fields explicitly
decode UTF-8 and use locale-independent casing. Supplementary CJK uses ImGui's
32-bit character configuration; GPU vertex indices remain 16-bit.

Cassette, TrueType Winamp and settings fonts merge missing glyphs from the
existing Sony Noto KR, SST Japanese, DFHei Chinese, Noto Thai and SST UI fonts. Font range
storage remains alive for texture rebuilds. A local test using fonts from the
supplied image found glyphs for Chinese, Japanese, Korean, Thai, accented Latin,
Cyrillic and U+20BB7 in a compact sample atlas. This is a sample test, not a
measurement of a full-library atlas. No extracted Sony fonts are redistributed.

Run the metadata/Unicode tests after initializing and patching `libs/imgui`:

```sh
# Requires pkg-config, ICU and SQLite development headers/libraries.
TEXT_TEST_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh scripts/test-text.sh
```

Run the dependency-free geometry and C touch tests on macOS or Linux:

```sh
make test-display
DISPLAY_TEST_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh scripts/test-display.sh
```

The workflow `WM1A-Z display` also applies the combined ImGui/GLFW patches to
their pinned commits and checks the actual backend sources with a Linux
compiler. It does **not** build or certify the ARM application.

The new [WM1AZ ARM build workflow](.github/workflows/wm1az-arm.yml) uses the
public Linaro GCC 4.9.4 ARM hard-float toolchain, pinned project submodules and
a runtime SDK extracted statically from Sony's public A50 1.02 updater. It does
not execute the Sony Windows updater or depend on the original private builder
registry. Proprietary SDK libraries and Sony system images are not uploaded as
build artifacts. The service libraries shipped by upstream Wampy are retained.

[Build 42855949061 passed](https://github.com/HachiMiku39/wampy/actions/runs/42855949061)
for application source commit `726115ce8ef356e0acdfe1df4193107cdc265b00`:

- All production C/C++ sources compile and link for ARM EABI hard-float.
- QEMU runs the original Sony ARM glibc loader in trace mode with immediate
  relocation checks. Dependencies resolve without missing symbols; Wampy's
  main function and hardware initialization do not execute in this check.
- The original updater's first nine engine entries remain byte-for-byte
  unchanged. The new Walkman One UPG is decrypted again and its engine and
  userdata contents checked against the inputs.
- NSIS produces a WM1A/Z-only Windows wizard, retaining the upstream install
  and uninstall actions and the exact verified upstream scsitool.
- A separate local check uses system libraries extracted from the supplied WM1
  firmware, plus the unchanged service libraries. Required library versions
  and 3,458 strong symbol imports resolve without missing dependencies. This
  is an ABI/symbol check, not device execution.

To reproduce on Ubuntu 22.04, install the utilities listed in the workflow,
check out this branch and run from the repository root:

```sh
bash scripts/arm/prepare.sh
python3 scripts/arm/compile.py
python3 scripts/arm/check-loader.py
python3 scripts/arm/package.py
```

Outputs are in `.arm-build/output/`; logs and compile commands are in
`.arm-build/logs/`. Public compiler/header downloads are cached in `.arm-cache/`.
The older [BUILD.md](BUILD.md) remains as the upstream build reference; its A50
extraction steps are not a WM1A/Z flashing guide.

Before treating the installer as hardware-validated, check on a backed-up
device: all four touch corners, popup clipping, full-screen/legacy alignment,
physical buttons, suspend/wake and return to Sony UI. Then check PCM, DSD64,
DSD128 and Sony USB DAC mode. No device was flashed during this build.

## Installation path verified against upstream

Follow the original [INSTALL.md](INSTALL.md) and the pinned `nw-installer`
submodule. Its Windows installer selects **NW-WM1A/Z** and separately asks
whether **Walkman One is installed**. A Walkman One device needs that package
variant, rather than the stock WM1 variant. It copies a service UPG and a
userdata archive to internal storage, then uses `scsitool` to request the
normal updater flow. The installer modifies the installed boot/initramfs and
platform files; it is not a complete Sony system-image reflash.

Read-only preflight on the user's Parallels Windows 11 ARM64 VM succeeded:
internal `WALKMAN` volume and SD card were visible, and the original x86
`scsitool-nwz-v27.exe` listed both volumes and read Sony device information.
The tool was extracted from upstream release v1.14.6 and its SHA-256 matched
the pinned installer's value. The firmware identifies itself as WM1Z/3.02 with
128 GB storage, consistent with a modified WM1 firmware; that is not evidence
of the physical chassis model. No upgrade command was issued.

Windows ARM is therefore usable for this installation front end in the tested
VM. It does not replace the Linux ARM cross-build. The new Windows installer
is now built, but has not been run against the device. For this user-confirmed
Walkman One setup, select Install → NW-WM1A/Z → YES for Walkman One and choose
the internal WALKMAN volume. The standalone UPG uses Walkman One encryption
and must not be substituted for a stock-firmware UPG. The service installer
does not consume the supplied full system image, so that image's
inner-manifest mismatch is specifically a concern for repacking the full
firmware; it is not by itself a blocker to the original service-install route.
