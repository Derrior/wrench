/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_NOISYCLOUDSERVICE_H
#define WRENCH_NOISYCLOUDSERVICE_H

#include <map>
#include <simgrid/s4u/VirtualMachine.hpp>

#include "wrench/simulation/Simulation.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/services/compute/cloud/CloudComputeService.h"
#include "wrench/services/compute/cloud/CloudComputeServiceProperty.h"
#include "wrench/services/compute/cloud/CloudComputeServiceMessagePayload.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"
#include "wrench/workflow/job/PilotJob.h"

#include <pugixml.hpp>

namespace wrench {

    class Simulation;

    class BareMetalComputeService;

class NoisyCloudComputeService : public CloudComputeService {
    public:
        struct EnvironmentInstability {
            double deviation;
            double mean;
            std::map<std::string, std::vector<double>> tasks_runtime;
            EnvironmentInstability()
                : deviation(0)
                , mean(0) {
            }
        };

    private:
        EnvironmentInstability instability;
        std::random_device rd;
        std::mt19937 generator;
        std::normal_distribution<> instability_distribution;

        static EnvironmentInstability ComputeInstability(std::string filename, std::string env_name);


    public:

        NoisyCloudComputeService(const std::string &hostname,
                     std::vector<std::string> &execution_hosts,
                     std::string scratch_space_mount_point,
                     std::string noise_description_file,
                     std::string environment_name,
                     std::map<std::string, std::string> property_list = {},
                     std::map<std::string, double> messagepayload_list = {})
            : CloudComputeService(hostname, execution_hosts, scratch_space_mount_point, property_list, messagepayload_list) {
            instability = ComputeInstability(noise_description_file, environment_name);
            instability_distribution = std::normal_distribution<>(0, instability.deviation);
        }

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/
        /*
        virtual std::string createVM(unsigned long num_cores,
                                     double ram_memory,
                                     std::map<std::string, std::string> property_list = {},
                                     std::map<std::string, double> messagepayload_list = {});

        virtual std::string createVM(unsigned long num_cores,
                                     double ram_memory,
                                     std::string desired_vm_name,
                                     std::map<std::string, std::string> property_list = {},
                                     std::map<std::string, double> messagepayload_list = {});

        virtual void shutdownVM(const std::string &vm_name);

        virtual std::shared_ptr<BareMetalComputeService> startVM(const std::string &vm_name);

        virtual void suspendVM(const std::string &vm_name);

        virtual void resumeVM(const std::string &vm_name);

        virtual void destroyVM(const std::string &vm_name);

        virtual bool isVMRunning(const std::string &vm_name);
        virtual bool isVMSuspended(const std::string &vm_name);
        virtual bool isVMDown(const std::string &vm_name);
        


        std::vector<std::string> getExecutionHosts();
        */
        /***********************/
        /** \endcond          **/
        /***********************/

        /***********************/
        /** \cond INTERNAL    */
        /***********************/
        /*
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> &service_specific_args) override;

        void terminateStandardJob(StandardJob *job) override;
        void terminatePilotJob(PilotJob *job) override;

        void validateProperties();
        */
        ~NoisyCloudComputeService() = default;

        /***********************/
        /** \endcond          **/
        /***********************/

    protected:
        /***********************/
        /** \cond INTERNAL    */
        /***********************/

        /*
        int main() override;

        virtual bool processNextMessage();
        */

        // virtual void processGetResourceInformation(const std::string &answer_mailbox);

        // virtual void processGetExecutionHosts(const std::string &answer_mailbox);

        /*
        virtual void processCreateVM(const std::string &answer_mailbox,
                                     unsigned long requested_num_cores,
                                     double requested_ram,
                                     std::string desired_vm_name,
                                     std::map<std::string, std::string> property_list,
                                     std::map<std::string, double> messagepayload_list
        );
        */

        // virtual void processStartVM(const std::string &answer_mailbox, const std::string &vm_name, const std::string &pm_name);

        // virtual void processShutdownVM(const std::string &answer_mailbox, const std::string &vm_name);

        // virtual void processSuspendVM(const std::string &answer_mailbox, const std::string &vm_name);

        // virtual void processResumeVM(const std::string &answer_mailbox, const std::string &vm_name);

        // virtual void processDestroyVM(const std::string &answer_mailbox, const std::string &vm_name);

        virtual void processSubmitStandardJob(const std::string &answer_mailbox, StandardJob *job,
                                              std::map<std::string, std::string> &service_specific_args) override;

        /*
        virtual void processSubmitPilotJob(const std::string &answer_mailbox, PilotJob *job,
                                           std::map<std::string, std::string> &service_specific_args);

        */
        // virtual void processBareMetalComputeServiceTermination(std::shared_ptr<BareMetalComputeService> cs, int exit_code);

        /** \cond */
        static unsigned long VM_ID;
        /** \endcond */

        
        std::unique_ptr<StandardJob> mutateJob(StandardJob *job);
        double mutateFlops(double flops);

        std::vector<std::unique_ptr<StandardJob>> job_copies;
        std::vector<std::unique_ptr<WorkflowTask>> task_copies;
        /***********************/
        /** \endcond           */
        /***********************/
    };
}

#endif //WRENCH_NOISYCLOUDSERVICE_H
