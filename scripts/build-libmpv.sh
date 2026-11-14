#!/bin/bash
# Rebuild third_party/libmpv/lib/libmpv.2.dylib from source (macOS x86_64).
#
# The prebuilt dylib is committed for convenience; this script reproduces it.
# It builds a lean libmpv (shared) with ffmpeg/libplacebo/libass statically
# linked, so the result depends only on macOS system frameworks.
#
# Prerequisites: Xcode CLT, git, python3, and on PATH:
#   meson (pip install meson), ninja, nasm, pkg-config (pkgconf).
#
# Key lessons (see HANDOVER pitfalls):
#  - mpv >= 0.42 requires libass unconditionally -> build fribidi/freetype/
#    harfbuzz/libass (CoreText, no fontconfig) first.
#  - Disable cocoa/swift (macOS Swift VO fails to build on recent SDKs);
#    video goes through the render API (vo=libmpv), not the native VO.
#  - The app uses the SW render API + mpv log level "info": the OpenGL render
#    API deadlocks coreaudio, and verbose logging triggers a null-deref in
#    mpv's coreaudio device enumeration.
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT=$(pwd)
WORK=${WORK:-/tmp/mpv-build}
PREFIX=$WORK/build_libs
export PATH="$HOME/Library/Python/3.9/bin:$HOME/mpv-tools/bin:$PATH"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
mkdir -p "$WORK"; cd "$WORK"

clone() { [ -d "$1/.git" ] || git clone --depth 1 "$2" "$1"; }
clone fribidi    https://github.com/fribidi/fribidi.git
clone freetype   https://github.com/freetype/freetype.git
clone harfbuzz   https://github.com/harfbuzz/harfbuzz.git
clone libass     https://github.com/libass/libass.git
clone libplacebo https://github.com/haasn/libplacebo.git
clone ffmpeg     https://github.com/FFmpeg/FFmpeg.git
clone mpv        https://github.com/mpv-player/mpv.git

meson_build() { # dir, extra options...
  local d=$1; shift
  ( cd "$d" && rm -rf build && meson setup build -Ddefault_library=static \
      --prefix="$PREFIX" --libdir="$PREFIX/lib" "$@" && \
    meson compile -C build && meson install -C build )
}

meson_build fribidi  -Dtests=false -Ddocs=false
meson_build freetype -Dzlib=disabled -Dbzip2=disabled -Dpng=disabled -Dbrotli=disabled -Dharfbuzz=disabled
meson_build harfbuzz -Dfreetype=enabled -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dicu=disabled -Dtests=disabled -Ddocs=disabled -Dutilities=disabled
meson_build libass   -Dcoretext=enabled -Dfontconfig=disabled -Dtest=disabled -Dprofile=disabled
( cd libplacebo && git submodule update --init --depth 1 )
meson_build libplacebo -Dvulkan=disabled -Ddemos=false -Dtests=false -Dglslang=disabled

# ffmpeg (static, PIC) via its own configure
( cd ffmpeg && ./configure --prefix="$PREFIX" --enable-gpl --disable-debug \
    --disable-doc --enable-static --disable-shared --enable-pic \
    --disable-programs && make -j"$(sysctl -n hw.ncpu)" && make install )

# mpv as a shared libmpv with the static deps above
( cd mpv && rm -rf build && meson setup build -Dbuildtype=release \
    -Dlibmpv=true -Ddefault_library=shared -Dgpl=false -Dcplayer=false \
    -Dlua=disabled -Djavascript=disabled -Dlibarchive=disabled -Duchardet=disabled \
    -Dvapoursynth=disabled -Drubberband=disabled -Dlcms2=disabled \
    -Dcocoa=disabled -Dswift-build=disabled -Dmacos-cocoa-cb=disabled \
    -Dmacos-media-player=disabled -Dmacos-touchbar=disabled && \
  meson compile -C build )

mkdir -p "$ROOT/third_party/libmpv/lib"
cp mpv/build/libmpv.2.dylib "$ROOT/third_party/libmpv/lib/"
( cd "$ROOT/third_party/libmpv/lib" && ln -sf libmpv.2.dylib libmpv.dylib )
echo "Built: $ROOT/third_party/libmpv/lib/libmpv.2.dylib"