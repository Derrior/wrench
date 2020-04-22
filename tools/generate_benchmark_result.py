import xml.dom.minidom as md
from argparse import ArgumentParser
import random

class distribution:
    def get_time(self, job):
        return abs(float(job.getAttribute("runtime")) + random.normalvariate(2, 3))


def get_job_list(filename):
    tree = md.parse(filename).getElementsByTagName("adag")[0]
    return tree.getElementsByTagName("job")

def get_distributions(filename):
    return [distribution()]   

parser = ArgumentParser()
parser.add_argument("-w", "--workflow-file", dest="workflow",
                    help="path to workflow description in dax format")

parser.add_argument("-n", "--run-number", dest="runs",
                    help="number of runs for each distribution")

parser.add_argument("-o", "--output", dest="output",
                    help="output file name")

args = parser.parse_args()
jobs = get_job_list(args.workflow)
result = md.Document()
root = result.createElement("root")
result.appendChild(root)
for job in jobs:
    result_job = result.createElement("job")
    result_job.setAttribute("id", job.getAttribute("id"))
    result_job.setAttribute("name", job.getAttribute("name"))

    distr = distribution()
    root.appendChild(result_job)
    for i in range(int(args.runs)):
        run = result.createElement("run")
        run.setAttribute("id", str(i))
        run.setAttribute("runtime", str(distr.get_time(job)))
        result_job.appendChild(run)

xml_str = result.toprettyxml(indent="    ")
with open(args.output, "w") as f:
    f.write(xml_str)

