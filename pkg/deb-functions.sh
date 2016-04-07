#!/bin/bash

############ HOWTO compute deb version
# This document might help:
# http://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Version
# Essentially, these are examples of version numbers, from oldest to newest:
# * 2.0~rc1
# * 2.0~rc1+anystring
# * 2.0~rc2
# * 2.0+anystring
# * 2.0.0~rc1
# * 2.0.0
# * 2.0.1
###############################
function compute_deb_version(){
  # load the version identifiers
  if ! [[ -r ${WDIR}/src/version.sh ]] ; then
    echo "Cannot open versions file: ${WDIR}/src/version.sh"
    echo 'Please run cmake.'
    exit 2
  fi

  . ${WDIR}/src/version.sh

  # For tag names, we should use at most three numbers: major, minor
  # and revision. The revision number can be dropped for pre-releases.
  # Optionally, a release candidate number can be appended after a hyphen.
  if ! [[ "${GIT_TAG_LAST}" =~ ^v[0-9]+(\.[0-9]+){1,2}(|-rc[0-9]+)$ ]] ; then
    echo "The last tag name (${GIT_TAG_LAST}) is not in the expected format."
    echo 'Please retag.'
    exit 2
  fi

  # "debianize" the tag name: remove the leading 'v' and use a tilde as rc separator.
  # Please see again the comment at the top of this file about the choice of the tilde.
  PG_DEBVERSION=$(echo "${GIT_TAG_LAST:1}" | sed 's:-rc:~rc:')

  if [[ "${GIT_TAG_EXACT}" == "" ]] ; then
    # We have some more commits after the tag: add a suffix for them.
    # The suffix should preserve the ordering, so use the date first,
    # then Git's SHA-1.
    datever=$(date "+%Y%m%d.%H%M%S")
    PG_DEBVERSION="${PG_DEBVERSION}+git.${datever}.${GIT_SHA1_SHORT}"
  fi

  echo "Using Debian version: $PG_DEBVERSION"
}

function gen_control_file() {
  local PROVIDES=
  rm -rf ${BDIR_BASE}/DEBIAN/control
  mkdir -p ${BDIR_BASE}/DEBIAN
  cat > ${BDIR_BASE}/DEBIAN/control <<DELIM___
Package: $1
Version: $2
Section: base
Priority: optional
Architecture: $3
Depends: $5
Maintainer: Marco Leogrande <dark.knight.ita@gmail.com>
Description: $4
DELIM___
}

function build_package() {
  local script_loc=${WDIR}/pkg/debian-control

  if [[ -e ${script_loc}/${PKG_PREFIX}-postinst.sh ]]; then
    cp -pf ${script_loc}/${PKG_PREFIX}-postinst.sh ${BDIR_BASE}/DEBIAN/postinst
    chmod 555 ${BDIR_BASE}/DEBIAN/postinst
    echo "Found post-inst file ${script_loc}/${PKG_PREFIX}-postinst.sh"
  fi
  if [[ -e ${script_loc}/${PKG_PREFIX}-postrm.sh ]]; then
    cp -pf ${script_loc}/${PKG_PREFIX}-postrm.sh ${BDIR_BASE}/DEBIAN/postrm
    chmod 555 ${BDIR_BASE}/DEBIAN/postrm
    echo "Found post-rm file ${script_loc}/${PKG_PREFIX}-postrm.sh"
  fi
  if [[ "${PG_BUILD_SMALLER_DEB}" == "1" ]] ; then
    echo 'Using slower compression scheme for DEB generation'
    fakeroot dpkg-deb --build -Zxz -z9 ${BDIR_BASE}
    status=$?
  else
    fakeroot dpkg-deb --build ${BDIR_BASE}
    status=$?
  fi

  return $status
}

# cleanup function called on ctrl-c or other error
function cleanup() {
  if [[ -d ${BDIR_BASE} ]]; then
    rm -rf ${BDIR_BASE}
  fi

  return $?
}

# run if user hits control-c
function control_c() {
  echo -en "\n*** Ouch! Exiting ***\n"
  cleanup
  exit 1
}
