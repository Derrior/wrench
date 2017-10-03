/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_MULTICORECOMPUTESERVICE_H
#define WRENCH_MULTICORECOMPUTESERVICE_H


#include <queue>

#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/compute/standard_job_executor/StandardJobExecutor.h"
#include "wrench/services/compute/MulticoreComputeServiceProperty.h"

namespace wrench {

    class Simulation;

    class StorageService;

    class FailureCause;

    /**  @brief A ComputeService implementation that
     *   runs on a multi-core host and can execute sets of independent SINGLE-CORE tasks
     */
    class MulticoreComputeService : public ComputeService {

    private:

        std::map<std::string, std::string> default_property_values = {
                {MulticoreComputeServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                    "1024"},
                {MulticoreComputeServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,                 "1024"},
                {MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                {MulticoreComputeServiceProperty::SUBMIT_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                {MulticoreComputeServiceProperty::JOB_TYPE_NOT_SUPPORTED_MESSAGE_PAYLOAD,         "1024"},
                {MulticoreComputeServiceProperty::NOT_ENOUGH_CORES_MESSAGE_PAYLOAD,               "1024"},
                {MulticoreComputeServiceProperty::STANDARD_JOB_DONE_MESSAGE_PAYLOAD,              "1024"},
                {MulticoreComputeServiceProperty::STANDARD_JOB_FAILED_MESSAGE_PAYLOAD,            "1024"},
                {MulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_REQUEST_MESSAGE_PAYLOAD, "1024"},
                {MulticoreComputeServiceProperty::TERMINATE_STANDARD_JOB_ANSWER_MESSAGE_PAYLOAD,  "1024"},
                {MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,       "1024"},
                {MulticoreComputeServiceProperty::SUBMIT_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,        "1024"},
                {MulticoreComputeServiceProperty::PILOT_JOB_STARTED_MESSAGE_PAYLOAD,              "1024"},
                {MulticoreComputeServiceProperty::PILOT_JOB_EXPIRED_MESSAGE_PAYLOAD,              "1024"},
                {MulticoreComputeServiceProperty::PILOT_JOB_FAILED_MESSAGE_PAYLOAD,               "1024"},
                {MulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_REQUEST_MESSAGE_PAYLOAD,    "1024"},
                {MulticoreComputeServiceProperty::TERMINATE_PILOT_JOB_ANSWER_MESSAGE_PAYLOAD,     "1024"},
                {MulticoreComputeServiceProperty::NUM_IDLE_CORES_REQUEST_MESSAGE_PAYLOAD,         "1024"},
                {MulticoreComputeServiceProperty::NUM_IDLE_CORES_ANSWER_MESSAGE_PAYLOAD,          "1024"},
                {MulticoreComputeServiceProperty::NUM_CORES_REQUEST_MESSAGE_PAYLOAD,              "1024"},
                {MulticoreComputeServiceProperty::NUM_CORES_ANSWER_MESSAGE_PAYLOAD,               "1024"},
                {MulticoreComputeServiceProperty::TTL_REQUEST_MESSAGE_PAYLOAD,                    "1024"},
                {MulticoreComputeServiceProperty::TTL_ANSWER_MESSAGE_PAYLOAD,                     "1024"},
                {MulticoreComputeServiceProperty::FLOP_RATE_REQUEST_MESSAGE_PAYLOAD,              "1024"},
                {MulticoreComputeServiceProperty::FLOP_RATE_ANSWER_MESSAGE_PAYLOAD,               "1024"},
                {MulticoreComputeServiceProperty::THREAD_STARTUP_OVERHEAD,                        "0.0"},
                {MulticoreComputeServiceProperty::CORE_ALLOCATION_POLICY,                         "aggressive"}
        };

    public:

        // Public Constructor
        MulticoreComputeService(std::string hostname,
                                bool supports_standard_jobs,
                                bool supports_pilot_jobs,
                                StorageService *default_storage_service,
                                std::map<std::string, std::string> plist = {});



        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        // Running jobs
        void submitStandardJob(StandardJob *job, std::map<std::string, std::string> service_specific_args) override;
        void submitPilotJob(PilotJob *job, std::map<std::string, std::string> service_specific_args) override;

        // Terminating jobs
        void terminateStandardJob(StandardJob *job) override;

        void terminatePilotJob(PilotJob *job) override;


        // Getting information
        double getTTL() override;

        double getCoreFlopRate() override;


        /***********************/
        /** \endcond           */
        /***********************/

    private:

        friend class Simulation;

        unsigned long job_id = 0;

//        MulticoreComputeService();

        // Low-level Constructor
        MulticoreComputeService(std::string hostname,
                                bool supports_standard_jobs,
                                bool supports_pilot_jobs,
                                std::map<std::string, std::string>,
                                unsigned long num_cores,
                                double ttl,
                                PilotJob *pj, std::string suffix,
                                StorageService *default_storage_service);

        std::string hostname;
        unsigned long num_available_cores;

        double ttl;
        bool has_ttl;
        double death_date;
        PilotJob *containing_pilot_job;

        // Vector of standard job executors
        std::set<StandardJobExecutor *> standard_job_executors;

        // Set of running jobs
        std::set<WorkflowJob *> running_jobs;

        // Queue of pending jobs (standard or pilot) that haven't begun executing
        std::deque<WorkflowJob *> pending_jobs;


        int main() override;

        // Helper functions to make main() a bit more palatable

        void terminate(bool notify_pilot_job_submitters);

        void terminateAllPilotJobs();

        void failCurrentStandardJobs(std::shared_ptr<FailureCause> cause);

        void processStandardJobCompletion(StandardJobExecutor *executor, StandardJob *job);

        void processStandardJobFailure(StandardJobExecutor *executor,
                                       StandardJob *job,
                                       std::shared_ptr<FailureCause> cause);

        void processPilotJobCompletion(PilotJob *job);

        void processStandardJobTerminationRequest(StandardJob *job, std::string answer_mailbox);

        void processPilotJobTerminationRequest(PilotJob *job, std::string answer_mailbox);

        bool processNextMessage(double timeout);

        bool dispatchNextPendingJob();

        void createWorkForNewlyDispatchedJob(StandardJob *job);

        void terminateRunningStandardJob(StandardJob *job);

        void terminateRunningPilotJob(PilotJob *job);

        void failPendingStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);

        void failRunningStandardJob(StandardJob *job, std::shared_ptr<FailureCause> cause);

        void processGetNumCores(std::string &answer_mailbox) override;

        void processGetNumIdleCores(std::string &answer_mailbox) override;

        void processSubmitStandardJob(std::string &answer_mailbox, StandardJob *job) override;
    };
};


#endif //WRENCH_MULTICORECOMPUTESERVICE_H