#!/usr/bin/env bash

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PRESET="clang-ninja-release"

readonly OPTIONS=("ubuntu20" "ubuntu22")

print_options() {
    echo "Available legacy build targets:"
    for option in "${OPTIONS[@]}"; do
        echo "  ${option}"
    done
    echo
    echo "Usage: ./legacy-build.sh <target>"
}

if [[ $# -eq 0 ]]; then
    print_options
    exit 0
fi

if [[ $# -ne 1 ]]; then
    echo "Expected exactly one target." >&2
    echo >&2
    print_options >&2
    exit 1
fi

target="$1"
case "${target}" in
    ubuntu20 | ubuntu22)
        ;;
    *)
        echo "Unknown legacy build target: ${target}" >&2
        echo >&2
        print_options >&2
        exit 1
        ;;
esac

dockerfile="${SCRIPT_DIR}/docker/Dockerfile.${target}"
image="soldankpp-${target}-build"

docker build \
    -t "${image}" \
    -f "${dockerfile}" \
    "${SCRIPT_DIR}"

docker run \
    --rm \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -v "${SCRIPT_DIR}:/work" \
    -w /work \
    "${image}" \
    bash -lc "cmake --preset ${PRESET} \
                -DVCPKG_OVERLAY_TRIPLETS=/work/cmake/vcpkg-triplets \
                -DVCPKG_TARGET_TRIPLET=x64-linux-libcxx \
                -DVCPKG_HOST_TRIPLET=x64-linux-libcxx \
                -DCMAKE_CXX_FLAGS='-stdlib=libc++ -pthread' \
                -DCMAKE_EXE_LINKER_FLAGS='-stdlib=libc++ -lc++abi -pthread' \
                -DCMAKE_SHARED_LINKER_FLAGS='-stdlib=libc++ -lc++abi -pthread' && \
              cmake --build --preset ${PRESET} && \
              cmake -E copy_if_different /lib/x86_64-linux-gnu/libc++.so.1 /work/build/${PRESET}/bin/Release && \
              cmake -E copy_if_different /lib/x86_64-linux-gnu/libc++abi.so.1 /work/build/${PRESET}/bin/Release"
