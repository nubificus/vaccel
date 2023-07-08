#!/bin/sh

# generate .version file
cd ${MESON_SOURCE_ROOT} || exit 1
VERSION=$(sh ./scripts/generate-version.sh)
echo ${VERSION} > "${MESON_DIST_ROOT}/.version"

# build .deb packages
# (needs: meson dist --include-subprojects)
cd ${MESON_DIST_ROOT}
rm -f "../*${VERSION}.orig.tar.xz"
rm -rf "${MESON_DIST_ROOT}/.git*"
PKG_NAME=$1 && shift
MESON_ARGS=""                                                                   
c=$((0))                                                                        
for v in $@                                                                     
do                                                                              
	if [ $((c % 2)) -eq 0 ]                                                     
	then                                                                        
		MESON_ARGS="${MESON_ARGS}-D$v="                                         
	else                                                                        
		MESON_ARGS="${MESON_ARGS}$v "                                           
	fi                                                                          
	c=$((c+1))                                                                  
done
USER="$(whoami)" \
	DEBFULLNAME="Anastassios Nanos" DEBEMAIL="ananos@nubificus.co.uk" \
	dh_make -s -y -c apache -p "${PKG_NAME}_${VERSION}" --createorig
echo "override_dh_auto_configure:\n\tdh_auto_configure --buildsystem=meson -- ${MESON_ARGS}" \
	>> debian/rules
dpkg-buildpackage -us -uc
rm -r obj-* debian
