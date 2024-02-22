# TDR era dunetpc (tag v07\_06\_02 [redmine](https://cdcvs.fnal.gov/redmine/projects/dunetpc/repository/revisions/v07\_06\_02/show)

Note: the root directory must be named `dunetpc` or the build breaks :)

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

`source build_ninja.sh` and `source install_ninja.sh` should work normally

If you do a `mrb zd` at any point you will probably need to repeat some of this weird failing the build and then setting up old smake business

