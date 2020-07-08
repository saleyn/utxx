#!/bin/bash
#-------------------------------------------------------------------------------
# This script creates a version include file
#
# Author:  Serge Aleynikov <saleyn@gmail.com>
#-------------------------------------------------------------------------------
# Created: 2015-04-05
#-------------------------------------------------------------------------------

DATE=$(TZ=GMT date +'%Y%m%d-%H:%M:%S %z')
DIR=$(readlink -f $0)
DIR=${DIR%/bin/*}
FILE=${DIR}/include/utxx/version.hpp
GIT_VER=$(git --version | awk '{sub(/\.[0-9]+\.[0-9]+$/, "", $3); print $3}')
VER_CMD="git describe --dirty --abbrev=7 --tags --always --long"
REBUILD=${2:-OFF}
REBUILD=${REBUILD^^}

[ "${GIT_VER}" \> "2.0" ] && VER_CMD+=" --first-parent"

if   [ "$1" = "-h" -o "${2}" = "--help" ]; then
    echo "${0##*/} [-d ON|OFF] [-v]"
elif [ "$1" = "-d" -a "${REBUILD}" == "OFF" -a -f "${FILE}" ]; then
    true
elif [ "$1" = "-v" ]; then
    echo "Version: ${DATE} ($(${VER_CMD} 2>/dev/null))"
    echo
    git update-index --refresh --unmerged -q
elif [ ! -f ${FILE} ] || ! git -c diff.autorefreshindex=true diff --quiet; then
    VER=$(${VER_CMD} 2>/dev/null)
    cat > ${FILE} <<EOL
//------------------------------------------------------------------------------
// Version.hpp
//------------------------------------------------------------------------------
// This file is auto-generated!  DON'T MODIFY!
//------------------------------------------------------------------------------
#pragma once

constexpr const char* VERSION() {
    return "Version: ${DATE} (${VER})";
}
EOL

fi
