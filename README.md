# TDR era dunetpc (tag v07\_06\_02 [redmine](https://cdcvs.fnal.gov/redmine/projects/dunetpc/repository/revisions/v07\_06\_02/show))

Note: the root directory must be named `dunetpc` or the build breaks :)

This is the dunetpc of the MCC11 production. Code for making paired ND-FD data has been added in
`dune/NDFDPairs`. This code relies on the `HighFive` library which is not available pre-built on
ups for this old `dunetpc`. It has been added as a header only library at `dune/NDFDPairs/highfive`
instead. The CAFs made from the MCC11 data and used in lbl studies seem to have been made with a
slightly newer FD CAFMaker. The CAFMaker and CVN from dunetpc tag v07\_09\_00 have been swapping in
and the originals moved to `dune/CAFMaker_old` and `dune/CVN_old`.

Building requires some weird steps, these are the only way I could build without errors. On a dunegpvm:
```
mkdir larsoft_area
cd larsoft_area
source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
mrb newDev -v v07_06_02 -q e17:prof
cd srcs
git clone git@github.com:AlexWilkinsonnn/dunetpc.git
cd ../
cp srcs/dunetpc/scripts/* .
# update paths in setup.sh
source setup.sh
mrb uc
source build_ninja.sh # this will fail and is a required step
source setup_old_cmake.sh
source build_ninja.sh # this should now build
```

In a new shell do `source setup.sh` to use the code

For development, I found `source install_ninja.sh` to not work. `source buid_ninja.sh` does work but complains about cmake version, so I am doing `unsetup cmake; setup cmake v3_21_4` before building after a change...

`source build_ninja.sh` and `source install_ninja.sh` should work normally. I f

If you do a `mrb zd` at any point you will probably need to repeat some of this weird failing the build and then setting up old smake business

