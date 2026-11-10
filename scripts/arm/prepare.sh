#!/usr/bin/env bash
set -euo pipefail
ROOT=$(pwd)
BUILD="$ROOT/.arm-build"
CACHE="$ROOT/.arm-cache"
mkdir -p "$BUILD"/{logs,output,base,sysroot/lib,include} "$CACHE"
exec > >(tee "$BUILD/logs/prepare.log") 2>&1
fetch() {
  local url=$1 output=$2 hash=${3:-}
  if [[ ! -s "$output" ]]; then curl --fail --location --retry 2 --connect-timeout 20 --max-time 300 "$url" -o "$output"; fi
  if [[ -n "$hash" ]]; then echo "$hash  $output" | sha256sum --check; fi
}
fetch 'https://media.githubusercontent.com/media/kobolabs/Kobo-Reader/master/toolchain/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf.tar.xz' "$CACHE/gcc49.tar.xz" '22914118fd963f953824b58107015c6953b5bbdccbdcf25ad9fd9a2f9f11ac07'
tar -xf "$CACHE/gcc49.tar.xz" -C "$BUILD"
TOOLCHAIN=$(find "$BUILD" -maxdepth 1 -type d -name 'gcc-linaro-*' | head -1)
export PATH="$TOOLCHAIN/bin:$PATH"
arm-linux-gnueabihf-g++ --version
arm-linux-gnueabihf-g++ -print-sysroot
cat > "$BUILD/environment.json" <<EOF
{"compiler":"$TOOLCHAIN/bin/arm-linux-gnueabihf-g++","cc":"$TOOLCHAIN/bin/arm-linux-gnueabihf-gcc","prefix":"$TOOLCHAIN/bin/arm-linux-gnueabihf-"}
EOF
cat > "$BUILD/toolchain.cmake" <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7)
set(CMAKE_C_COMPILER "$TOOLCHAIN/bin/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "$TOOLCHAIN/bin/arm-linux-gnueabihf-g++")
EOF

git submodule update --init --depth 1 libs/glfw libs/imgui libs/zlib libs/glm libs/implot libs/mINI libs/ImageMagick libs/protobuf libs/sqlite nw-installer
git -C libs/imgui apply ../imgui.patch
git -C libs/glfw apply ../glfw.patch
git -C libs/implot apply ../implot.patch
make -C nw-installer/tools/upgtool/upgtools -j2
UPGTOOL="$ROOT/nw-installer/tools/upgtool/upgtools/upgtool"
"$UPGTOOL" --help || true
fetch 'https://github.com/unknown321/wampy/releases/download/v1.14.6/walkmanOne.tar.gz' "$BUILD/base/walkmanOne.tar.gz" 'baf5ae3544909ce8f55ea3f062313a152c088d3c6a4f0ce20752da7dfcea3e8d'
tar -xf "$BUILD/base/walkmanOne.tar.gz" -C "$BUILD/base"
mkdir -p "$BUILD/base/entries"
"$UPGTOOL" -m nw-wm1a -w -e -o "$BUILD/base/entries/" "$BUILD/base/NW_WM_FW.UPG"
ls -lh "$BUILD/base/entries"
mkdir -p "$BUILD/base/payload"
tar -xf "$BUILD/base/entries/9.bin" -C "$BUILD/base/payload"
find "$BUILD/base/payload" -maxdepth 1 -type f -printf '%f\n' | sort
cp "$BUILD/base/payload/"*.so* "$BUILD/sysroot/lib/"
arm-linux-gnueabihf-readelf -d "$BUILD/base/payload/wampy" | tee "$BUILD/logs/upstream-elf.txt" || true

# Only the SDK consumes Sony's public firmware; no firmware image or vendor
# library is included in build artifacts. The device installer is assembled
# from the existing upstream service package, not from this system image.
fetch 'https://walkman.update.sony.net/fw/pc/A50/J/NW-A50_V1_02.exe' "$BUILD/base/sony-a50.exe"
mkdir -p "$BUILD/sony" "$BUILD/sony/entries"
python3 scripts/arm/extract-sony.py "$BUILD/base/sony-a50.exe" "$BUILD/sony"
while IFS= read -r -d '' cabinet; do
  7z x -y -o"$BUILD/sony/cab" "$cabinet"
done < <(find "$BUILD/sony" -name '*.cab' -print0)
SONY_UPG=$(find "$BUILD/sony" -name NW_WM_FW.UPG | head -1)
test -n "$SONY_UPG"
"$UPGTOOL" -m nw-a50 -e -z 6 -o "$BUILD/sony/entries/" "$SONY_UPG"
SONY_IMAGE="$BUILD/sony/entries/6.bin"
for name in libMali_linux.so libasound.so libsqlite3.so; do
  debugfs -R "dump /lib/$name $BUILD/sysroot/lib/$name" "$SONY_IMAGE"
done
for name in libicudata.so.61 libicuuc.so.61 libicui18n.so.61; do
  debugfs -R "dump /vendor/sony/lib/$name $BUILD/sysroot/lib/$name" "$SONY_IMAGE"
  ln -sf "$name" "$BUILD/sysroot/lib/${name%.61}"
done
python3 scripts/arm/extract-rootfs.py "$BUILD/sony/entries/2.bin" "$BUILD/sony/rootfs.cpio"
mkdir -p "$BUILD/sony/rootfs"
(cd "$BUILD/sony/rootfs"; cpio -idm --no-absolute-filenames 'lib/*' < "$BUILD/sony/rootfs.cpio")
cp -a "$BUILD/sony/rootfs/lib/"* "$BUILD/sysroot/lib/"
for pair in 'libc.so.6 libc.so' 'libpthread.so.0 libpthread.so' 'libdl.so.2 libdl.so' 'librt.so.1 librt.so'; do
  read -r target link <<< "$pair"
  ln -sf "$target" "$BUILD/sysroot/lib/$link"
done
file "$BUILD/sysroot/lib/"*.so*
# Linker names for the unmodified upstream service dependencies.
for pair in 'libMagick++-7.Q8HDRI.so libMagick++-7.Q8HDRI.so' 'libprotobuf.so.32.0.12 libprotobuf.so' 'libjpeg.so.62.4.0 libjpeg.so' 'libshine.so.3 libshine.so'; do
  read -r target link <<< "$pair"
  if [[ "$target" != "$link" ]]; then ln -sf "$target" "$BUILD/sysroot/lib/$link"; fi
done
fetch 'https://github.com/unicode-org/icu/releases/download/release-61-2/icu4c-61_2-src.tgz' "$CACHE/icu61.tgz"
mkdir -p "$BUILD/icu"
tar -xf "$CACHE/icu61.tgz" -C "$BUILD/icu" --strip-components=1
fetch 'https://prodgpl.blob.core.windows.net/download/Audio/NW-A57/alsa-lib-1.0.29.tar.gz' "$CACHE/alsa.tar.gz"
mkdir -p "$BUILD/alsa"
tar -xf "$CACHE/alsa.tar.gz" -C "$BUILD/alsa" --strip-components=1
(cd "$BUILD/alsa"; CC=arm-linux-gnueabihf-gcc ./configure --host=arm-linux-gnueabihf --disable-python)
fetch 'https://github.com/toots/shine/releases/download/3.1.1/shine-3.1.1.tar.gz' "$CACHE/shine.tgz"
mkdir -p "$BUILD/shine"
tar -xf "$CACHE/shine.tgz" -C "$BUILD/shine" --strip-components=1
mkdir -p "$BUILD/include/shine"
find "$BUILD/shine" -name layer3.h -exec cp {} "$BUILD/include/shine/" \;
mkdir -p "$BUILD/sqlite"
(cd "$BUILD/sqlite"; CC=arm-linux-gnueabihf-gcc "$ROOT/libs/sqlite/configure" --host=arm-linux-gnueabihf; make sqlite3.h)
(cd libs/ImageMagick; CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ ./configure --host=arm-linux-gnueabihf --without-x --without-zlib --with-quantum-depth=8 --enable-hdri --disable-docs --without-perl)
cmake -S libs/glfw -B "$BUILD/glfw" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$BUILD/toolchain.cmake" -DGLFW_BUILD_X11=OFF -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF
cmake --build "$BUILD/glfw" -j2
cmake -S libs/zlib -B "$BUILD/zlib" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$BUILD/toolchain.cmake" -DZLIB_BUILD_EXAMPLES=OFF
cmake --build "$BUILD/zlib" --target zlibstatic -j2
