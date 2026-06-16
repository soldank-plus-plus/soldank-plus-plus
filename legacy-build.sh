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
    bash -lc "cmake --preset ${PRESET} && cmake --build --preset ${PRESET}"
