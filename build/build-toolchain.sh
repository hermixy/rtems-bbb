#! /usr/bin/env sh

# be more verbose
set -x
# exit on wrong command and undefined variables
set -e -u

# find out own directory
SCRIPTDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECTDIR="${SCRIPTDIR}/../"

# configuration
. "${SCRIPTDIR}/configuration.sh"
LOGDIR="${PROJECTDIR}/build/"
NOW="$(date +%Y%m%d_%H%M%S)"
export PATH="${PREFIX}/bin:${PATH}"

# Build tools using source builder
cd "${RSB_DIR}"
./source-builder/sb-check
if [ $? -ne 0 ]
then
	echo "ERROR: Please check your build environment."
	exit 1
fi

cd "${RSB_DIR}/rtems"
../source-builder/sb-set-builder \
	--log="${LOGDIR}/rsb-${RTEMS_CPU}-${NOW}.log" \
	--prefix="${PREFIX}" \
	--without-rtems \
	"${RTEMS_VERSION}/rtems-${RTEMS_CPU}"
#	--no-clean \
