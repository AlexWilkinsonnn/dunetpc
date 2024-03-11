# Add reco datasets from ndfd paired hdf5 files stored on /pnfs.
# Requires ifdhc setup

import argparse, subprocess, os

import h5py
import numpy as np

def main(args):
    ret = subprocess.run(["ifdh", "ls", args.input_dir], stdout=subprocess.PIPE)
    f_paths = ret.stdout.decode("utf-8").strip("\n").split("\n")[1:] # first is directory

    f_out = h5py.File(args.output_file, "w")

    for f_path in f_paths:
        f_name = os.path.basename(f_path)

        proc = subprocess.Popen(["ifdh", "cp", f_path, f_name], stdout=subprocess.PIPE)
        proc.wait()

        with h5py.File(f_name) as f_in:
            if "fd_reco" in f_in.keys():
                nd_reco = f_in["nd_paramreco"]
                fd_reco = f_in["fd_reco"]
                
                f_out.create_group(f_name)
                f_out[f_name].create_dataset("nd_paramreco", data=nd_reco)
                f_out[f_name].create_dataset("fd_reco", data=fd_reco)
    
                print(f"Added {f_name}")
            else:
                print(f"No FD reco for {f_name}")

        proc = subprocess.Popen(["rm", "-v", f_name], stdout=subprocess.PIPE)
        proc.wait()

    f_out.close()

def parse_arguments():
    parser = argparse.ArgumentParser()

    parser.add_argument("input_dir")
    parser.add_argument("output_file")

    args = parser.parse_args()

    return args

if __name__ == "__main__":
    args = parse_arguments()
    main(args)
