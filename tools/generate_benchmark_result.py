import xml.dom.minidom as md
from argparse import ArgumentParser
import random

class distribution:
    def get_time(self, job):
        return abs(float(job.getAttribute("runtime")) + random.normalvariate(0, 1))

    def get(self, name):
        return "standard normal"


def get_job_list(filename):
    tree = md.parse(filename).getElementsByTagName("adag")[0]
    return tree.getElementsByTagName("job")

def get_distributions(filename):
    return [distribution()]   

parser = ArgumentParser()
parser.add_argument("-w", "--workflow-file", dest="workflow",
                    help="path to workflow description in dax format")

parser.add_argument("-d", "--distribution-file", dest="distributions",
                    help="path to execution time' distributions description in xml format")
parser.add_argument("-n", "--run-number", dest="runs",
                    help="number of runs for each distribution")

parser.add_argument("-o", "--output", dest="output",
                    help="output file name")

args = parser.parse_args()
jobs = get_job_list(args.workflow)
distributions = get_distributions(args.distributions)
result = md.Document()
root = result.createElement("root")
result.appendChild(root)
for job in jobs:
    result_job = result.createElement("job")
    result_job.setAttribute("id", job.getAttribute("id"))
    result_job.setAttribute("name", job.getAttribute("name"))

    root.appendChild(result_job)
    for distr in distributions:
        env = result.createElement("env")
        env.setAttribute("name", distr.get("vm_type"))
        result_job.appendChild(env)
        for i in range(int(args.runs)):
            run = result.createElement("run")
            run.setAttribute("id", str(i))
            run.setAttribute("runtime", str(distr.get_time(job)))
            env.appendChild(run)

xml_str = result.toprettyxml(indent="    ")
with open(args.output, "w") as f:
    f.write(xml_str)

