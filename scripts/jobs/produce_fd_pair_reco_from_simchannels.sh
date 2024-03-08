#!/bin/bash
################################################################################
# Script to take ND-FD pair HDF5 with truth information for ND + FD and
# SimChannel information from FD. Loads FD SimChannels into larsoft. Runs
# detsim and reco. Resulting FD reco is written back to the HDF5 file.
################################################################################
# Options

FDRECO_PAIR_OUTPUT="/pnfs/dune/scratch/users/awilkins/larbath_ndfd_pairs/tdr_sample/fdreco_artroot"
COMPLETE_PAIR_OUTPUT="/pnfs/dune/scratch/users/awilkins/larbath_ndfd_pairs/tdr_sample/pair_complete_ndfd"

SAVE_FDRECO=false
SAVE_COMPLETE_PAIR=true # turn this on if not testing!

INTERACTIVE=false # not running on grid to test (run from inside dir that would be in tarball)

INPUT_PAIR_H5_DIR=$1

################################################################################

if [ "$INTERACTIVE" = true ]; then
  PROCESS=0 # or 1?
  export INPUT_TAR_DIR_LOCAL=${PWD}
else
  echo "Running on $(hostname) at ${GLIDEIN_Site}. GLIDEIN_DUNESite = ${GLIDEIN_DUNESite}. At ${PWD}"
fi

echo $INPUT_TAR_DIR_LOCAL

# Setup env
${INPUT_TAR_DIR_LOCAL}/srcs/dunetpc/scripts/jobs/make_setup_grid.sh ${INPUT_TAR_DIR_LOCAL}/localProducts_larsoft_*/setup \
                                                                    setup-grid
source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
source setup-grid
unsetup mrb
setup mrb v4_04_06
mrbslp

# Don't try over and over again to copy a file when it isn't going to work
export IFDH_CP_UNLINK_ON_ERROR=1
export IFDH_CP_MAXRETRIES=1
export IFDH_DEBUG=0

input_file=$(ifdh ls $INPUT_PAIR_H5_DIR | head -n $((PROCESS+2)) | tail -n -1)
input_name=${input_file##*/}

echo "input_file is ${input_file}"

ifdh cp $input_file $input_name
input_file_local=$PWD/$input_name

# Prepare fcls
cp ${INPUT_TAR_DIR_LOCAL}/srcs/dunetpc/dune/NDFDPairs/run_fcls/*.fcl .
sed -i "s#physics.producers.largeant.NDFDH5FileLoc: \"\"#physics.producers.largeant.NDFDH5FileLoc: \"${input_file_local}\"#" run_LoadFDSimChannels.fcl
sed -i "s#physics.analyzers.addreco.NDFDH5FileLoc: \"\"#physics.analyzers.addreco.NDFDH5FileLoc: \"${input_file_local}\"#" run_AddFDReco.fcl

ls -lrth

num_events=$(h5ls $input_name | sed -n "s/^vertices.*Dataset {\([0-9]\+\).*/\1/p")
echo "$input_name has $num_events events"

# CVN stuff needs a libgtk-3.so that is not found on the grid (fine on the gpvms :()
# We included in the tarball to use here
cp -r ${INPUT_TAR_DIR_LOCAL}/lib64 .
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PWD}/lib64

# Generate FD Reco from SimChannels
lar -c ./run_LoadFDSimChannels.fcl -n $num_events
lar -c standard_detsim_dune10kt_nooptdetsim_1x2x6.fcl -s LoadedFDSimChannels.root -n -1
lar -c standard_reco_dune10kt_nu_1x2x6.fcl -s LoadedFDSimChannels_detsimnoopt.root -n -1
lar -c select_ana_dune10kt_nu.fcl -s LoadedFDSimChannels_detsimnoopt_reco.root -n -1

# Add FD Reco to H5 file
lar -c ./run_AddFDReco.fcl -s *_merged.root -n -1

ls -lrth

echo "Copying files to dCache..."
if [ "$SAVE_FDRECO" = true ]; then
  ifdh cp *_merged.root ${FDRECO_PAIR_OUTPUT}/${input_name%.*}_LoadedFDSimChannels_detsimnoopt_reco_merged.root
fi
if [ "$SAVE_COMPLETE_PAIR" = true ]; then
  ifdh cp ${input_name} ${COMPLETE_PAIR_OUTPUT}/${input_name%.*}_fdreco.h5
fi

