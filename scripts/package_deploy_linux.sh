#!/bin/bash
# package_deploy_linux.sh
# Creates a distributable package from Linux build artifacts.
#
# Usage: package_deploy_linux.sh <distro> <config> <customer>
#   distro:   ubuntu | alpine
#   config:   debug | release
#   customer: customer profile name (must match deploy/config/<customer>.json)

set -euo pipefail

DISTRO="${1:-ubuntu}"
CONFIG="${2:-release}"
CUSTOMER="${3:-}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_ARTIFACTS="${PROJECT_ROOT}/build/linux-${DISTRO}-${CONFIG}/artifacts"
DEPLOY_OUT="${PROJECT_ROOT}/deploy/out/${CUSTOMER}/linux-${DISTRO}/${CONFIG}"
PACKAGE_NAME="myproject_${CUSTOMER}_linux-${DISTRO}_${CONFIG}"

if [[ ! -d "${BUILD_ARTIFACTS}" ]]; then
    echo "[ERROR] Build artifacts not found: ${BUILD_ARTIFACTS}"
    echo "        Run the Linux ${DISTRO} ${CONFIG} build first."
    exit 1
fi

echo "[INFO] Creating package: ${PACKAGE_NAME}"
echo "[INFO] Source: ${BUILD_ARTIFACTS}"
echo "[INFO] Target: ${DEPLOY_OUT}"

# Clean and recreate output directory
rm -rf "${DEPLOY_OUT}"
mkdir -p "${DEPLOY_OUT}"

# Copy binaries
cp -r "${BUILD_ARTIFACTS}"/. "${DEPLOY_OUT}/"

# Make binaries executable
chmod +x "${DEPLOY_OUT}"/* 2>/dev/null || true

# Create tarball
PACKAGE_ARCHIVE="${PROJECT_ROOT}/deploy/out/${PACKAGE_NAME}.tar.gz"
tar -czf "${PACKAGE_ARCHIVE}" -C "$(dirname "${DEPLOY_OUT}")" "$(basename "${DEPLOY_OUT}")"

echo "[OK] Package created: ${PACKAGE_ARCHIVE}"
