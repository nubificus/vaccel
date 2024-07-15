#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

SRC_DIR=$1
DEFAULT_VERSION="v0.0.0"

[ -n "${SRC_DIR}" ] || SRC_DIR="$(dirname "$0")/.."

if [ -f "${SRC_DIR}/.version" ]; then
	cat "${SRC_DIR}/.version"
	exit 0
fi

if [ ! -d "${SRC_DIR}/.git" ]; then
	echo ${DEFAULT_VERSION}
	exit 1
fi

VERSION=$(git describe --abbrev=8 --tags --match "v[0-9]*" 2>/dev/null)

if [ -z "${VERSION}" ]; then
	SHA=$(git describe --abbrev=8 --always 2>/dev/null)
	COMMITS=$(git log --oneline | wc -l 2>/dev/null)
	VERSION="v0.0.0-${COMMITS}-g${SHA}"
fi

DIRTY=$(git diff --quiet || echo '-dirty')
for i in "$@"; do
	if [ "$i" = "--no-dirty" ]; then
		DIRTY=""
		break
	fi
done

VERSION=$(echo "${VERSION}${DIRTY}" | sed -e 's/-g/-/' | cut -c 2-)

echo "${VERSION}"
