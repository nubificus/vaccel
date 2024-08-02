#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

# generate .version file
cd "${MESON_SOURCE_ROOT}" || exit 1
VERSION="$(sh ./scripts/generate-version.sh "" --no-dirty)"
echo "${VERSION}" >"${MESON_DIST_ROOT}/.version"

# parse script args
PKG_NAME=$1 && shift
PKG_BUILDTYPE=$1 && shift
MESON_ARGS="--buildtype=${PKG_BUILDTYPE} "
c=$((0))
for v in "$@"; do
	if [ $((c % 2)) -eq 0 ]; then
		MESON_ARGS="${MESON_ARGS}-D$v="
	else
		MESON_ARGS="${MESON_ARGS}$v "
	fi
	c=$((c + 1))
done

cd "${MESON_DIST_ROOT}" || exit 1

# generate binary dist
# (needs: meson dist --include-subprojects)
BIN_NAME="${PKG_NAME}-${VERSION}"
BIN_TAR_NAME="${BIN_NAME}-bin.tar.gz"
BIN_PREFIX="${MESON_DIST_ROOT}/build/${BIN_NAME}"
rm -rf ../"${BIN_TAR_NAME}" build
eval meson setup "${MESON_ARGS}" \
	--prefix="${BIN_PREFIX}/usr" \
	build &&
	meson compile -C build &&
	meson install -C build &&
	tar cfz ../"${BIN_TAR_NAME}" -C build "${BIN_NAME}"
rm -rf build

# generate .deb packages
# (needs: meson dist --include-subprojects)
for p in "build-essential" "dh-make" "git-buildpackage"; do
	if [ -z "$(dpkg -l | awk "/^ii  $p/")" ]; then
		echo "Not building a deb package: Package $p missing"
		return
	fi
done
rm -f ../*"${VERSION}".orig.tar.xz
rm -rf "${MESON_DIST_ROOT}"/.git*
DEBFULLNAME="Anastassios Nanos"
export DEBFULLNAME
DEBEMAIL="ananos@nubificus.co.uk"
export DEBEMAIL

USER="$(whoami)" \
	dh_make -s -y -c apache -p "${PKG_NAME}_${VERSION}" --createorig

# debian/rules
printf "%s\n" "export DEB_LDFLAGS_MAINT_STRIP = -Wl,-Bsymbolic-functions" \
	>>debian/rules
printf "%s\n\t%s" "override_dh_auto_configure:" \
	"dh_auto_configure --buildsystem=meson -- ${MESON_ARGS}" \
	>>debian/rules

# debian/copyright
sed -i "s/Upstream-Contact.*/Upstream-Contact: ${DEBFULLNAME} <${DEBEMAIL}>/g" \
	debian/copyright
sed -i "s/Source.*/Source: https:\/\/github.com\/nubificus\/vaccel/g" \
	debian/copyright
sed -i "s/Upstream-Contact.*/Upstream-Contact: ${DEBFULLNAME} <${DEBEMAIL}>/g" \
	debian/copyright
sed -i "s/Copyright.*/Copyright: 2020-$(date +"%Y") Nubificus LTD <info@nubificus.co.uk>/g" \
	debian/copyright
sed -i "/<years>/d" debian/copyright
sed -i "/^#.*/d" debian/copyright

# debian/control
sed -i "s/Homepage.*/Homepage: https:\/\/vaccel.org/g" debian/control
sed -i "s/Section.*/Section: libs/g" debian/control
sed -i "s/Description.*/Description: Hardware Acceleration for Serverless Computing/g" \
	debian/control
sed -i "/^#.*/d" debian/control
sed -i "/<insert/d" debian/control

# debian/changelog
echo "Generating changelog..."
sed -i "2d;3d" debian/changelog
cp -r "${MESON_SOURCE_ROOT}"/.git* ./
TAG=$(git describe --abbrev=0 --tags --match "v[0-9]*" 2>/dev/null)
TAG_DIFF=$(git describe --abbrev=8 --tags --match "v[0-9]*" 2>/dev/null)
ORIG_COMMIT=$(git rev-parse HEAD)
for t in $(git tag --sort=v:refname | grep "v[0-9]*"); do
	CUR_VERSION="$(echo "$t" | cut -c 2-)-1"
	git checkout "$t" 1>/dev/null 2>&1
	PREV_TAG=$(git describe --abbrev=0 --tags --match "v[0-9]*" \
		--exclude="$t" 2>/dev/null) ||
		PREV_TAG=$(git rev-list --max-parents=0 HEAD | tail -n 1)
	gbp dch --since="${PREV_TAG}" --ignore-branch --release \
		--spawn-editor=never --new-version="${CUR_VERSION}" \
		--dch-opt="-b" --dch-opt="--check-dirname-level=0" \
		--dch-opt="-p" \
		--git-log="--invert-grep --grep=Signed-off-by:.*github-actions" \
		1>/dev/null 2>&1
done
if [ "${TAG}" != "${TAG_DIFF}" ]; then
	git checkout "${ORIG_COMMIT}" 1>/dev/null 2>&1
	PREV_TAG=$(git describe --abbrev=0 --tags --match "v[0-9]*" \
		2>/dev/null) ||
		PREV_TAG=$(git rev-list --max-parents=0 HEAD | tail -n 1)
	CUR_VERSION="${VERSION}-1"
	gbp dch --since="${PREV_TAG}" --ignore-branch --release \
		--spawn-editor=never --new-version="${CUR_VERSION}" \
		--dch-opt="-b" --dch-opt="--check-dirname-level=0" \
		--dch-opt="-p" \
		--git-log="--invert-grep --grep=Signed-off-by:.*github-actions" \
		1>/dev/null 2>&1
fi
rm -rf "${MESON_DIST_ROOT}"/.git*

rm -rf debian/*.ex debian/*.EX debian/*.docs debian/README*
dpkg-buildpackage -us -uc
rm -rf obj-* debian
