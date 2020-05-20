#!/usr/pin/python3

import os
import sys
import xml.dom.minidom as md
from argparse import ArgumentParser
from benchmark_io import record_run_result
import time

def get_job_list(filename):
    tree = md.parse(filename).getElementsByTagName("adag")[0]
    return tree.getElementsByTagName("job")

def build_command(job, bin_root):
    binary = bin_root + "/" + job.getAttribute("name")
    argument_line = job.getElementsByTagName("argument")[0]
    c = argument_line.firstChild
    result = [binary]
    while c:
        if (type(c) is md.Text):
            result.append(c.data)
        else:
            result.append(c.getAttribute("name"))
        c = c.nextSibling
    return ' '.join(result)
 
def get_job_deps(job):
    files = job.getElementsByTagName('uses')
    result = []
    for i in files:
        if i.getAttribute('link') == 'input':
            result.append(i.getAttribute('name'))
    return result

def get_job_outputs(job):
    files = job.getElementsByTagName('uses')
    result = []
    for i in files:
        if i.getAttribute('link') == 'output':
            result.append(i.getAttribute('name'))
    return result

def execute(cmd, job_key, benchmark_result):
    print("Measuring of execution", cmd)
    start_time = time.time()
    os.system(cmd)
    end_time = time.time()
    benchmark_result[job_key] = benchmark_result.get(job_key, [])
    benchmark_result[job_key].append(end_time - start_time)

    
parser = ArgumentParser()
parser.add_argument("-w", "--workflow-file", dest="workflow", required=True,
                    help="path to workflow description in dax format")

parser.add_argument("-n", "--run-number", dest="runs", default='1',
                    help="number of runs for each task")

parser.add_argument("-d", "--data-folder", dest="data", default='data',
                    help="data storage for files")

parser.add_argument("-o", "--output", dest="output", default='benchmark.xml',
                    help="output file name")

parser.add_argument("-b", "--binary-root", dest="bin_root", default='.',
                    help="where have you located your Montage executables")

args = parser.parse_args()
runs_amount = int(args.runs)
jobs = get_job_list(args.workflow)
bin_root = os.path.abspath(args.bin_root)
os.chdir(args.data)

job_counter = {}
finished_job_amount = 0
ready_files = set()
file_list = os.listdir('.')
benchmark_result = {}

for i in file_list:
    ready_files.add(i)
print(ready_files)

while finished_job_amount < len(jobs):
    for job in jobs:
        job_id = job.getAttribute("id")
        already_runned = job_counter.get(job_id, 0) 
        if already_runned >= runs_amount:
            continue # skip job, we've already run it enough times
        cmd = build_command(job, bin_root)
        if not already_runned:
            input_files = get_job_deps(job)
            ready = True
            for i in input_files:
                if i not in ready_files:
                    print(i, "not in", ready_files)
                    ready = False
            if (ready):
                execute(cmd, (job_id, job.getAttribute("name")), benchmark_result)
                output_files = get_job_outputs(job)
                for i in output_files:
                    ready_files.add(i)

        else:
            execute(cmd, (job_id, job.getAttribute("name")), benchmark_result)
        job_counter[job_id] = already_runned + 1
        if job_counter[job_id] == runs_amount:
            finished_job_amount += 1

record_run_result(benchmark_result, args.output)
