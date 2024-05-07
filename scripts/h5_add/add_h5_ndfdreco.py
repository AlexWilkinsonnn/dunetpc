# Add reco datasets from ndfd paired hdf5 files stored on /pnfs.
# Requires ifdhc setup

import argparse, subprocess, os

import h5py
import numpy as np

def main(args):
    f_out = h5py.File(args.output_file, "w")
    
    for input_dir in args.input_dir.split(","):
        ret = subprocess.run(["ifdh", "ls", input_dir], stdout=subprocess.PIPE)
        f_paths = ret.stdout.decode("utf-8").strip("\n").split("\n")[1:] # first is directory
        fill_h5(f_paths, f_out)

    f_out.close()

def fill_h5(f_paths, f_out):
    for f_path in f_paths:
        f_name = os.path.basename(f_path)

        proc = subprocess.Popen(["ifdh", "cp", f_path, f_name], stdout=subprocess.PIPE)
        proc.wait()

        group_name = f_path.replace("/", "#")

        with h5py.File(f_name) as f_in:
            if "fd_reco" in f_in.keys() and "nd_paramreco" in f_in.keys():
                nd_reco = f_in["nd_paramreco"]
                fd_reco = f_in["fd_reco"]
                fd_vertices = f_in["fd_vertices"]
                
                f_out.create_group(group_name)
                f_out[group_name].create_dataset("nd_paramreco", data=nd_reco)
                f_out[group_name].create_dataset("fd_reco", data=fd_reco)
                f_out[group_name].create_dataset("fd_vertices", data=fd_vertices)
    
                print(f"Added {group_name}")
            else:
                print(f"Missing fd_reco or nd_paramreco for {f_path}:\n({f_in.keys()})")

        proc = subprocess.Popen(["rm", "-v", f_name], stdout=subprocess.PIPE)
        proc.wait()

def parse_arguments():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "input_dir", type=str, help="Can be comma separated if files in multiple dirs"
    )
    parser.add_argument("output_file")

    args = parser.parse_args()

    return args

if __name__ == "__main__":
    args = parse_arguments()
    main(args)
