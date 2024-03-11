source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
setup python v3_9_2
setup ifdhc
python -m venv .venv_3_9_2_h5py
source .venv_3_9_2_h5py/bin/activate
pip install numpy h5py
