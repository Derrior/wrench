#include "wrench/services/compute/noisy_cloud/NoisyCloudComputeService.h"

namespace wrench {

    NoisyCloudComputeService::EnvironmentInstability NoisyCloudComputeService::ComputeInstability(std::string filename, std::string env_name) {
        pugi::xml_document benchmarks_tree;
        benchmarks_tree.load_file(filename.c_str());
        pugi::xml_node root = benchmarks_tree.child("root");
        EnvironmentInstability result;
        std::map<std::pair<size_t, size_t>, double> deviations_by_test_case;
        for (pugi::xml_node job = root.child("job"); job; job = job.next_sibling("job")) {
            std::vector<double>& runtimes_list = result.tasks_runtime[job.attribute("id").value()];

            for (pugi::xml_node env = job.child("env"); env; env = env.next_sibling("env")) {
                if (env.attribute("name").value() == env_name) {
                    for (pugi::xml_node run = env.child("run"); run; run = run.next_sibling("run")) {
                        runtimes_list.push_back(std::stod(run.attribute("runtime").value()));
                    }
                }
            }
            double task_mean_deviation = 0;
            double std_deviation = 0;
            for (size_t i = 0; i < runtimes_list.size(); i++) {
                double first_time = runtimes_list[i];
                for (size_t j = i + 1; j < runtimes_list.size(); j++) {
                    double second_time = runtimes_list[j];
                    double dev = abs(first_time - second_time) / max(first_time, second_time);
                    task_mean_deviation += dev;
                    deviations_by_test_case[std::make_pair(i, j)] += dev;
                }
            }
            double pair_amount = runtimes_list.size() * (runtimes_list.size() - 1) / 2;
            task_mean_deviation /= pair_amount;
            result.mean += task_mean_deviation;
        }

        result.mean /= result.tasks_runtime.size();
        for (auto& test_case_dev : deviations_by_test_case) {
            test_case_dev.second /= result.tasks_runtime.size();
        }
        for (auto test_case_dev1 : deviations_by_test_case) {
            for (auto test_case_dev2 : deviations_by_test_case) {
                double second_momentum = (test_case_dev1.second - test_case_dev2.second);
                second_momentum *= second_momentum;

                result.deviation += second_momentum;
            }
        }
        std::cerr << "Mean relative deviation from runs: " << result.mean << std::endl;
        std::cerr << "Standard deviation of mean relative deviation from runs: " << result.deviation << std::endl;
        return result;
    }

    void NoisyCloudComputeService::processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                              std::map<std::string, std::string> &service_specific_args) {
        const auto& vm_name = service_specific_args["vm_name"];
        if (!vm_list.count(vm_name)) {
            throw std::invalid_argument("NoisyCloudComputeService::submitStandardJob(): Unknown VM name '" + vm_name + "'");
        }
        auto vm_ptr = vm_list[vm_name];
        std::unique_ptr<StandardJob> newJob = mutateJob(job);
 
        job_copies.push_back(std::move(newJob));
    }

    std::unique_ptr<StandardJob> NoisyCloudComputeService::mutateJob(StandardJob *job) {
        std::vector<WorkflowTask *> old_tasks = job->getTasks();
        std::vector<WorkflowTask *> new_tasks;
        for (auto task_ptr : old_tasks) {
            std::unique_ptr<WorkflowTask> new_task(new WorkflowTask(
                task_ptr->getID(),
                mutateFlops(task_ptr->getFlops()),
                task_ptr->getMinNumCores(),
                task_ptr->getMaxNumCores(),
                task_ptr->getParallelEfficiency(),
                task_ptr->getMemoryRequirement()
            ));
            new_tasks.push_back(new_task.get());
            task_copies.push_back(std::move(new_task));
        }
        std::unique_ptr<StandardJob> new_job(
                    new StandardJob(
                        job->workflow, 
                        new_tasks, 
                        job->file_locations,
                        job->pre_file_copies,
                        job->post_file_copies,
                        job->cleanup_file_deletions
                    ));
        
        job_copies.push_back(std::move(new_job));
    }

    double NoisyCloudComputeService::mutateFlops(double flops) {
        return (1 + instability.mean) * flops + instability_distribution(generator);
    }
}