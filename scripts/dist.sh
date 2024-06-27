#!/bin/sh

# generate .version file
cd "${MESON_SOURCE_ROOT}" || exit 1
VERSION="$(sh ./scripts/generate-version.sh "" --no-dirty)"
echo "${VERSION}" > "${MESON_DIST_ROOT}/.version"

# build .deb packages
# (needs: meson dist --include-subprojects)
for p in "build-essential" "dh-make" "git-buildpackage";
do
	if [ -z "$(dpkg -l | awk "/^ii  $p/")" ]; then
		echo "Not building a deb package: Package $p missing"
		return
	fi
done
cd "${MESON_DIST_ROOT}" || exit 1
rm -f ../*"${VERSION}".orig.tar.xz
rm -rf "${MESON_DIST_ROOT}"/.git*
DEBFULLNAME="Anastassios Nanos"
export DEBFULLNAME
DEBEMAIL="ananos@nubificus.co.uk"
export DEBEMAIL
PKG_NAME=$1 && shift

USER="$(whoami)" \
	dh_make -s -y -c apache -p "${PKG_NAME}_${VERSION}" --createorig

# debian/rules
MESON_ARGS=""
c=$((0))
for v in "$@"
do
	if [ $((c % 2)) -eq 0 ]
	then
		MESON_ARGS="${MESON_ARGS}-D$v="
	else
		MESON_ARGS="${MESON_ARGS}$v "
	fi
	c=$((c+1))
done
printf "%s\n" "export DEB_LDFLAGS_MAINT_STRIP = -Wl,-Bsymbolic-functions" \
	>> debian/rules
printf "%s\n\t%s" "override_dh_auto_configure:" \
	"dh_auto_configure --buildsystem=meson -- ${MESON_ARGS}" \
	>> debian/rules

# debian/copyright
sed -i "s/Upstream-Contact.*/Upstream-Contact: ${DEBFULLNAME} <${DEBEMAIL}>/g" debian/copyright
sed -i "s/Source.*/Source: https:\/\/github.com\/cloudkernels\/vaccelrt/g" debian/copyright
sed -i "s/Upstream-Contact.*/Upstream-Contact: ${DEBFULLNAME} <${DEBEMAIL}>/g" debian/copyright
sed -i "s/Copyright.*/Copyright: 2020-$(date +"%Y") Nubificus LTD <info@nubificus.co.uk>/g" debian/copyright
sed -i "/<years>/d" debian/copyright
sed -i "/^#.*/d" debian/copyright

# debian/control
sed -i "s/Homepage.*/Homepage: https:\/\/vaccel.org/g" debian/control
sed -i "s/Section.*/Section: libs/g" debian/control
sed -i "s/Description.*/Description: Hardware Acceleration for Serverless Computing/g" debian/control
sed -i "/^#.*/d" debian/control
sed -i "/<insert/d" debian/control

# debian/changelog
echo "Generating changelog..."
cp -r "${MESON_SOURCE_ROOT}"/.git* ./
TAG=$(git describe --abbrev=0 --tags --match "v[0-9]*" 2>/dev/null)
TAG_DIFF=$(git describe --abbrev=8 --tags --match "v[0-9]*" 2>/dev/null)
ORIG_COMMIT=$(git rev-parse HEAD)
for t in $(git tag --sort=v:refname | grep "v[0-9]*")
do
	CUR_VERSION="$(echo "$t" | cut -c 2-)-1"
	git checkout "$t" 1>/dev/null 2>&1
	PREV_TAG=$(git describe --abbrev=0 --tags --match "v[0-9]*" --exclude="$t" 2>/dev/null) || continue
	gbp dch --since="${PREV_TAG}" --ignore-branch --release --spawn-editor=never --new-version="${CUR_VERSION}" --dch-opt="-b" --dch-opt="--check-dirname-level=0" --dch-opt="-p" 1>/dev/null 2>&1
done
if [ "${TAG}" != "${TAG_DIFF}" ]
then
	git checkout "${ORIG_COMMIT}" 1>/dev/null 2>&1
	PREV_TAG=$(git describe --abbrev=0 --tags --match "v[0-9]*"  2>/dev/null)
	CUR_VERSION="${VERSION}-1"
	gbp dch --since="${PREV_TAG}" --ignore-branch --release --spawn-editor=never --new-version="${CUR_VERSION}" --dch-opt="-b" --dch-opt="--check-dirname-level=0" --dch-opt="-p" 1>/dev/null 2>&1
fi
head -n -6 debian/changelog > debian/changelog_ && rm debian/changelog && mv debian/changelog_ debian/changelog
rm -rf "${MESON_DIST_ROOT}"/.git*

rm -rf debian/*.ex debian/*.EX debian/*.docs debian/README*
dpkg-buildpackage -us -uc
rm -rf obj-* debian
