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

}