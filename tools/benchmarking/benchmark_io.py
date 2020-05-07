import xml.dom.minidom as md

def record_run_result(res, filename):
    xml_rec = md.Document()
    root = xml_rec.createElement("root")
    xml_rec.appendChild(root)
    for job in res:
        print(job)
        id, name = job
        result_job = xml_rec.createElement("job")
        result_job.setAttribute("id", id)
        result_job.setAttribute("name", name)

        root.appendChild(result_job)
        for i in range(len(res[job])):
            run = xml_rec.createElement("run")
            run.setAttribute("id", str(i))
            run.setAttribute("runtime", str(res[job][i]))
            result_job.appendChild(run)

    xml_str = xml_rec.toprettyxml(indent="    ")
    with open(filename, "w") as f:
        f.write(xml_str)


def read_run_result(filename):
    jobs = md.parse(filename).getElementsByTagName("job")
    result = {}
    for job in jobs:
        id, name = job.getAttribute("id"), job.getAttribute("name")
        result[(id, name)] = []
        runs = job.getElementsByTagName("run")
        for run in runs:
            result[(id, name)].append(run.getAttribute("runtime"))
    return result
