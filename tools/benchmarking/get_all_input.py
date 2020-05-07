#!/usr/bin/python3
import os
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-r', dest='resources', help='resource description', required=True)
parser.add_argument('-d', dest='dest', help='destination folder', required=True)
args = parser.parse_args()


resources = args.resources
f = open(resources).readlines()

dest = args.dest
os.system("mkdir -p " + dest)

for s in f:
    name, ref = s.split()[:2]
    print(ref)
    if "http" in ref:
        print("here")
        os.system("wget -O %s %s" % (dest + "/" + name, ref))
