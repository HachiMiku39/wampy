# WM1A / WM1Z display adaptation — development preview

This branch targets the **first-generation NW-WM1A and NW-WM1Z**. It is a
source preview, not a validated firmware installer. ARM linking and device
validation are still required.

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

The source preview also fixes multilingual metadata handling. Glyph collection
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

The upstream ARM build currently requires Linux, the Sony sysroot and builder
images referenced at a private `192.168.10.102:5000` registry. The development
Mac has no Docker or ARM compiler. See [BUILD.md](BUILD.md) for the existing
build process; its A50 firmware-extraction steps are not a WM1A/Z flashing guide.

Before producing an installer, complete an ARM build and verify on a backed-up
device: all four touch corners, popup clipping, full-screen/legacy alignment,
physical buttons, suspend/wake and return to Sony UI. Then check PCM, DSD64,
DSD128 and Sony USB DAC mode. No device was flashed during this source change.

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
VM. It does not replace the Linux ARM cross-build. The source preview still
has no newly compiled ARM application or installable package. The service
installer does not consume the supplied full system image, so that image's
inner-manifest mismatch is specifically a concern for repacking the full
firmware; it is not by itself a blocker to the original service-install route.
