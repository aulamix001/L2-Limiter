#!/usr/bin/env bash
# Rootless setup for the Debian 13 cloud image. Uses verified APT packages.
set -euo pipefail
repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tools_dir=/workspace/tooling
mkdir -p "$tools_dir/apt/lists/partial" "$tools_dir/apt/cache/archives/partial" "$tools_dir/sysroot"
if [[ ! -x "$tools_dir/venv/bin/cmake" ]]; then
 python3 -m venv "$tools_dir/venv"
 "$tools_dir/venv/bin/pip" install cmake==4.4.5 ninja==1.13.2
fi
cat > "$tools_dir/apt/sources.list" <<'SOURCES'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian-security trixie-security main
SOURCES
apt_args=(-o "Dir::Etc::sourcelist=$tools_dir/apt/sources.list" -o Dir::Etc::sourceparts=- -o "Dir::State::lists=$tools_dir/apt/lists" -o "Dir::Cache=$tools_dir/apt/cache" -o Debug::NoLocking=1)
/usr/bin/apt-get "${apt_args[@]}" update
/usr/bin/apt-get "${apt_args[@]}" --download-only --no-install-recommends install -y libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev
(cd "$tools_dir/apt/cache/archives" && /usr/bin/apt-get "${apt_args[@]}" download libbz2-1.0 zlib1g-dev libbrotli-dev)
for package in "$tools_dir"/apt/cache/archives/*.deb; do dpkg-deb -x "$package" "$tools_dir/sysroot"; done
python3 - <<'PY'
from pathlib import Path
root=Path('/workspace/tooling/sysroot/usr/lib/x86_64-linux-gnu')
for p in root.iterdir():
 if p.is_symlink() and not p.exists():
  candidate=Path('/usr/lib/x86_64-linux-gnu')/p.readlink()
  if candidate.exists():
   p.unlink()
   p.symlink_to(candidate)
PY
if [[ ! -d "$tools_dir/JUCE/.git" ]]; then
 git clone --branch 7.0.9 --depth 1 https://github.com/juce-framework/JUCE.git "$tools_dir/JUCE"
fi
[[ "$(git -C "$tools_dir/JUCE" rev-parse HEAD)" == d054f0d14dcac387aebda44ce5d792b5e7a625b3 ]]
cat > "$tools_dir/activate.sh" <<'ACTIVATE'
export PATH=/workspace/tooling/venv/bin:$PATH
export CPLUS_INCLUDE_PATH=/workspace/tooling/sysroot/usr/include:/workspace/tooling/sysroot/usr/include/freetype2
export LIBRARY_PATH=/workspace/tooling/sysroot/usr/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu
export PKG_CONFIG_PATH=/workspace/tooling/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:/workspace/tooling/sysroot/usr/share/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=/workspace/tooling/sysroot
export CMAKE_BUILD_PARALLEL_LEVEL=2
ACTIVATE
source "$tools_dir/activate.sh"
cmake -S "$repo_dir" -B "$repo_dir/build" -G Ninja -U 'pkgcfg_lib_*' -DCMAKE_MAKE_PROGRAM="$tools_dir/venv/bin/ninja" -DCMAKE_BUILD_TYPE=Release -DFETCHCONTENT_SOURCE_DIR_JUCE="$tools_dir/JUCE" -DCMAKE_PREFIX_PATH="$tools_dir/sysroot/usr" -DCMAKE_CXX_FLAGS="-I$tools_dir/sysroot/usr/include" -DCMAKE_LIBRARY_PATH="$tools_dir/sysroot/usr/lib/x86_64-linux-gnu"
cmake --build "$repo_dir/build" --parallel 2
ctest --test-dir "$repo_dir/build" --output-on-failure
