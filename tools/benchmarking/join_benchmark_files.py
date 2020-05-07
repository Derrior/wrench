#!/usr/bin/python3

from benchmark_io import record_run_result, read_run_result
from argparse import ArgumentParser

parser = ArgumentParser()

parser.add_argument("input", nargs='+', help='one or many input files')
parser.add_argument("-o", "--output", dest="output", required=True,
                    help="path to destination file")


args = parser.parse_args()
inputs = args.input
print(inputs)
output = args.output
result = {}
for i in inputs:
    current_result = read_run_result(i)
    for key, value in current_result.items():
        result[key] = result.get(key, [])
        result[key].extend(value)

record_run_result(result, output)
