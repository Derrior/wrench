/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <wrench/wms/WMS.h>

#include "wrench/exceptions/WorkflowExecutionException.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/managers/JobManager.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/services/ServiceMessage.h"
#include "wrench/services/compute/ComputeServiceMessage.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_Simulation.h"
#include "wrench/simulation/SimulationMessage.h"
#include "wrench/workflow/WorkflowTask.h"
#include "wrench/workflow/job/StandardJob.h"
#include "wrench/workflow/job/PilotJob.h"
#include "wrench/wms/WMS.h"
#include "JobManagerMessage.h"


WRENCH_LOG_NEW_DEFAULT_CATEGORY(job_manager, "Log category for Job Manager");

namespace wrench {

    /**
     * @brief Constructor
     *
     * @param wms: the wms for which this manager is working
     */
    JobManager::JobManager(std::shared_ptr<WMS> wms) :
            Service(wms->hostname, "job_manager", "job_manager") {

        this->wms = wms;

    }

    /**
     * @brief Destructor, which kills the daemon (and clears all the jobs)
     */
    JobManager::~JobManager() {
        this->jobs.clear();
    }

    /**
     * @brief Kill the job manager (brutally terminate the daemon, clears all jobs)
     */
    void JobManager::kill() {
        this->killActor();
        this->jobs.clear();
    }

    /**
     * @brief Stop the job manager
     *
     * @throw WorkflowExecutionException
     * @throw std::runtime_error
     */
    void JobManager::stop() {
        try {
            S4U_Mailbox::putMessage(this->mailbox_name, new ServiceStopDaemonMessage("", 0.0));
        } catch (std::shared_ptr<NetworkError> &cause) {
            throw WorkflowExecutionException(cause);
        }
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the standard job)
     * @param file_locations: a map that specifies locations where input/output files should be read/written.
     *         When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     * @param pre_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         before task executions begin. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param post_file_copies: a vector of tuples that specify which file copy operations should be completed
     *                         after task executions end. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @param cleanup_file_deletions: a vector of file tuples that specify file deletion operations that should be completed
     *                                at the end of the job. The ComputeService::SCRATCH constant can be
     *                         used to mean "the scratch storage space of the ComputeService".
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks,
                                               std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations,
                                               std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> pre_file_copies,
                                               std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>, std::shared_ptr<FileLocation>  >> post_file_copies,
                                               std::vector<std::tuple<WorkflowFile *, std::shared_ptr<FileLocation>  >> cleanup_file_deletions) {

        // Do a sanity check of everything (looking for nullptr)
        for (auto t : tasks) {
            if (t == nullptr) {
                throw std::invalid_argument("JobManager::createStandardJob(): nullptr task in the task vector");
            }
        }

        for (auto fl : file_locations) {
            if (fl.first == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the file_locations map");
            }
            if (fl.second == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr storage service in the file_locations map");
            }
        }

        for (auto fc : pre_file_copies) {
            if (std::get<0>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the pre_file_copies set");
            }
            if (std::get<1>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr src storage service in the pre_file_copies set");
            }
            if (std::get<2>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr dst storage service in the pre_file_copies set");
            }
            if ((std::get<1>(fc) == FileLocation::SCRATCH) and (std::get<2>(fc) == FileLocation::SCRATCH)) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): cannot have FileLocation::SCRATCH as both source and destination in the pre_file_copies set");
            }
        }

        for (auto fc : post_file_copies) {
            if (std::get<0>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the post_file_copies set");
            }
            if (std::get<1>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr src storage service in the post_file_copies set");
            }
            if (std::get<2>(fc) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr dst storage service in the post_file_copies set");
            }
            if ((std::get<1>(fc) == FileLocation::SCRATCH) and (std::get<2>(fc) == FileLocation::SCRATCH)) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): cannot have FileLocation::SCRATCH as both source and destination in the pre_file_copies set");
            }
        }

        for (auto fd : cleanup_file_deletions) {
            if (std::get<0>(fd) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr workflow file in the cleanup_file_deletions set");
            }
            if (std::get<1>(fd) == nullptr) {
                throw std::invalid_argument(
                        "JobManager::createStandardJob(): nullptr storage service in the cleanup_file_deletions set");
            }
        }

        StandardJob *raw_ptr = new StandardJob(this->wms->getWorkflow(), tasks, file_locations, pre_file_copies,
                                               post_file_copies,
                                               cleanup_file_deletions);
        std::unique_ptr<WorkflowJob> job = std::unique_ptr<StandardJob>(raw_ptr);

        this->jobs.insert(std::make_pair(raw_ptr, std::move(job)));
        return raw_ptr;
    }

    /**
     * @brief Create a standard job
     *
     * @param tasks: a list of tasks  (which must be either READY, or children of COMPLETED tasks or
     *                                   of tasks also included in the list)
     * @param file_locations: a map that specifies locations where files should be read/written.
     *                        When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *JobManager::createStandardJob(std::vector<WorkflowTask *> tasks,
                                               std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations) {
        if (tasks.empty()) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments (empty tasks argument!)");
        }

        return this->createStandardJob(tasks, file_locations, {}, {}, {});
    }

    /**
     * @brief Create a standard job
     *
     * @param task: a task (which must be ready)
     * @param file_locations: a map that specifies locations where input/output files should be read/written.
     *                When unspecified, it is assumed that the ComputeService's scratch storage space will be used.
     *
     * @return the standard job
     *
     * @throw std::invalid_argument
     */
    StandardJob *
    JobManager::createStandardJob(WorkflowTask *task,
                                  std::map<WorkflowFile *, std::shared_ptr<FileLocation> > file_locations) {

        if (task == nullptr) {
            throw std::invalid_argument("JobManager::createStandardJob(): Invalid arguments");
        }

        std::vector<WorkflowTask *> tasks;
        tasks.push_back(task);
        return this->createStandardJob(tasks, file_locations);
    }

    /**
     * @brief Create a pilot job
     *
     * @return the pilot job
     *
     * @throw std::invalid_argument
     */
    PilotJob *JobManager::createPilotJob() {
        auto raw_ptr = new PilotJob(this->wms->workflow);
        auto job = std::unique_ptr<PilotJob>(raw_ptr);
        this->jobs[raw_ptr] = std::move(job);
        return raw_ptr;
    }


    /**
     * @brief Submit a job to compute service
     *
     * @param job: a workflow job
     * @param compute_service: a compute service
     * @param service_specific_args: arguments specific for compute services:
     *      - to a BareMetalComputeService: {{"taskID", "[hostname:][num_cores]}, ...}
     *           - If no value is not provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *           - If a "" value is provided for a task, then the service will choose a host and use as many cores as possible on that host.
     *           - If a "hostname" value is provided for a task, then the service will run the task on that
     *             host, using as many of its cores as possible
     *           - If a "num_cores" value is provided for a task, then the service will run that task with
     *             this many cores, but will choose the host on which to run it.
     *           - If a "hostname:num_cores" value is provided for a task, then the service will run that
     *             task with the specified number of cores on that host.
     *      - to a BatchComputeService: {{"-t":"<int>" (requested number of minutes)},{"-N":"<int>" (number of requested hosts)},{"-c":"<int>" (number of requested cores per host)}}
     *      - to a VirtualizedClusterComputeService: {} (jobs should not be submitted directly to the service)}
     *      - to a CloudComputeService: {} (jobs should not be submitted directly to the service)}
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void JobManager::submitJob(WorkflowJob *job, std::shared_ptr<ComputeService> compute_service,
                               std::map<std::string, std::string> service_specific_args) {

        if ((job == nullptr) || (compute_service == nullptr)) {
            throw std::invalid_argument("JobManager::submitJob(): Invalid arguments");
        }

        // Push back the mailbox_name of the manager,
        // so that it will getMessage the initial callback
        job->pushCallbackMailbox(this->mailbox_name);

        std::map<WorkflowTask *, WorkflowTask::State> original_states;

        // Update the job state and insert it into the pending list
        switch (job->getType()) {
            case WorkflowJob::STANDARD: {
                // Do a sanity check
                for (auto t : ((StandardJob *) job)->tasks) {
                    if ((t->getState() == WorkflowTask::State::COMPLETED) or
                        (t->getState() == WorkflowTask::State::PENDING)) {
                        throw std::invalid_argument("JobManager()::submitJob(): task " + t->getID() +
                                                    " cannot be submitted as part of a standard job because its state is " +
                                                    WorkflowTask::stateToString(t->getState()));
                    }
                }
                // Modify task states
                ((StandardJob *) job)->state = StandardJob::PENDING;
                for (auto t : ((StandardJob *) job)->tasks) {
                    original_states.insert(std::make_pair(t, t->getState()));
                    t->setState(WorkflowTask::State::PENDING);
                }

                this->pending_standard_jobs.insert((StandardJob *) job);
                break;
            }
            case WorkflowJob::PILOT: {
                ((PilotJob *) job)->state = PilotJob::PENDING;
                this->pending_pilot_jobs.insert((PilotJob *) job);
                break;
            }
        }

        // Submit the job to the service
        try {
            job->submit_date = Simulation::getCurrentSimulatedDate();
            job->service_specific_args = service_specific_args;
            compute_service->submitJob(job, service_specific_args);
            job->setParentComputeService(compute_service);

        } catch (WorkflowExecutionException &e) {

            // "Undo" everything
            job->popCallbackMailbox();
            switch (job->getType()) {
                case WorkflowJob::STANDARD: {
                    ((StandardJob *) job)->state = StandardJob::NOT_SUBMITTED;
                    for (auto t : ((StandardJob *) job)->tasks) {
                        t->setState(original_states[t]);
                    }
                    this->pending_standard_jobs.erase((StandardJob *) job);
                    break;
                }
                case WorkflowJob::PILOT: {
                    ((PilotJob *) job)->state = PilotJob::NOT_SUBMITTED;
                    this->pending_pilot_jobs.erase((PilotJob *) job);
                    break;
                }
            }
            throw;
        } catch (std::invalid_argument &e) {
            // "Undo" everything
            job->popCallbackMailbox();
            switch (job->getType()) {
                case WorkflowJob::STANDARD: {
                    ((StandardJob *) job)->state = StandardJob::NOT_SUBMITTED;
                    for (auto t : ((StandardJob *) job)->tasks) {
                        t->setState(original_states[t]);
                    }
                    this->pending_standard_jobs.erase((StandardJob *) job);
                    break;
                }
                case WorkflowJob::PILOT: {
                    ((PilotJob *) job)->state = PilotJob::NOT_SUBMITTED;
                    this->pending_pilot_jobs.erase((PilotJob *) job);
                    break;
                }
            }
            throw;
        }

    }

    /**
     * @brief Terminate a job (standard or pilot) that hasn't completed/expired/failed yet
     * @param job: the job to be terminated
     *
     * @throw WorkflowExecutionException
     * @throw std::invalid_argument
     * @throw std::runtime_error
     */
    void JobManager::terminateJob(WorkflowJob *job) {
        if (job == nullptr) {
            throw std::invalid_argument("JobManager::terminateJob(): invalid argument");
        }

        if (job->getParentComputeService() == nullptr) {
            std::string err_msg = "Job cannot be terminated because it doesn't  have a parent compute service";
            throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(nullptr, err_msg)));
        }

        try {
            job->getParentComputeService()->terminateJob(job);
        } catch (std::exception &e) {
            throw;
        }

        if (job->getType() == WorkflowJob::STANDARD) {
            ((StandardJob *) job)->state = StandardJob::State::TERMINATED;
            for (auto task : ((StandardJob *) job)->tasks) {
                switch (task->getInternalState()) {
                    case WorkflowTask::TASK_NOT_READY:
                        task->setState(WorkflowTask::State::NOT_READY);
                        break;
                    case WorkflowTask::TASK_READY:
                        task->setState(WorkflowTask::State::READY);
                        break;
                    case WorkflowTask::TASK_COMPLETED:
                        task->setState(WorkflowTask::State::COMPLETED);
                        break;
                    case WorkflowTask::TASK_RUNNING:
                    case WorkflowTask::TASK_FAILED:
                        task->setState(WorkflowTask::State::NOT_READY);
                        break;
                }
            }
            // Make second pass to fix NOT_READY states
            for (auto task : ((StandardJob *) job)->tasks) {
                if (task->getState() == WorkflowTask::State::NOT_READY) {
                    bool ready = true;
                    for (auto parent : task->getWorkflow()->getTaskParents(task)) {
                        if (parent->getState() != WorkflowTask::State::COMPLETED) {
                            ready = false;
                        }
                    }
                    if (ready) {
                        task->setState(WorkflowTask::State::READY);
                    }
                }
            }
        } else if (job->getType() == WorkflowJob::PILOT) {
            ((PilotJob *) job)->state = PilotJob::State::TERMINATED;
        }

    }

    /**
     * @brief Get the list of currently running pilot jobs
     * @return a set of pilot jobs
     */
    std::set<PilotJob *> JobManager::getRunningPilotJobs() {
        return this->running_pilot_jobs;
    }

    /**
     * @brief Get the list of currently pending pilot jobs
     * @return a set of pilot jobs
     */
    std::set<PilotJob *> JobManager::getPendingPilotJobs() {
        return this->pending_pilot_jobs;
    }

    /**
     * @brief Forget a job (to free memory, only once a job has completed or failed)
     *
     * @param job: a job to forget
     *
     * @throw std::invalid_argument
     * @throw WorkflowExecutionException
     */
    void JobManager::forgetJob(WorkflowJob *job) {

        if (job == nullptr) {
            throw std::invalid_argument("JobManager::forgetJob(): invalid argument");
        }

        // Check the job is somewhere
        if (this->jobs.find(job) == this->jobs.end()) {
            throw std::invalid_argument("JobManager::forgetJob(): unknown job");
        }

        if (job->getType() == WorkflowJob::STANDARD) {

            if ((this->pending_standard_jobs.find((StandardJob *) job) != this->pending_standard_jobs.end()) ||
                (this->running_standard_jobs.find((StandardJob *) job) != this->running_standard_jobs.end())) {
                std::string msg = "Job cannot be forgotten because it is pending or running";
                throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(job->getParentComputeService(), msg)));
            }
            if (this->completed_standard_jobs.find((StandardJob *) job) != this->completed_standard_jobs.end()) {
                this->completed_standard_jobs.erase((StandardJob *) job);
                this->jobs.erase(job);
                return;
            }
            if (this->failed_standard_jobs.find((StandardJob *) job) != this->failed_standard_jobs.end()) {
                this->failed_standard_jobs.erase((StandardJob *) job);
                this->jobs.erase(job);
                return;
            }
            // At this point, it's a job that was never submitted!
            if (this->jobs.find(job) != this->jobs.end()) {
                this->jobs.erase(job);
                return;
            }
            throw std::invalid_argument("JobManager::forgetJob(): unknown standard job");
        }

        if (job->getType() == WorkflowJob::PILOT) {
            if ((this->pending_pilot_jobs.find((PilotJob *) job) != this->pending_pilot_jobs.end()) ||
                (this->running_pilot_jobs.find((PilotJob *) job) != this->running_pilot_jobs.end())) {
                std::string msg = "Job cannot be forgotten because it is running or pending";
                throw WorkflowExecutionException(std::shared_ptr<FailureCause>(new NotAllowed(job->getParentComputeService(), msg)));
            }
            if (this->completed_pilot_jobs.find((PilotJob *) job) != this->completed_pilot_jobs.end()) {
                this->jobs.erase(job);
                return;
            }
            // At this point, it's a job that was never submitted!
            if (this->jobs.find(job) != this->jobs.end()) {
                this->jobs.erase(job);
                return;
            }
            throw std::invalid_argument("JobManager::forgetJob(): unknown pilot job");
        }

    }

    /**
     * @brief Main method of the daemon that implements the JobManager
     * @return 0 on success
     */
    int JobManager::main() {

        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_YELLOW);

        WRENCH_INFO("New Job Manager starting (%s)", this->mailbox_name.c_str());

        while (processNextMessage()) { }

        return 0;
    }

    /**
     * @brief Method to process an incoming message
     * @return
     */

    bool JobManager::processNextMessage() {

        std::shared_ptr<SimulationMessage> message = nullptr;
        try {
            message = S4U_Mailbox::getMessage(this->mailbox_name);
        } catch (std::shared_ptr<NetworkError> &cause) {
            WRENCH_INFO("Error while receiving message... ignoring");
            return true;
        }

        if (message == nullptr) {
            WRENCH_INFO("Got a NULL message... Likely this means we're all done. Aborting!");
            return false;
        }

        WRENCH_INFO("Job Manager got a %s message", message->getName().c_str());

        if (auto msg = std::dynamic_pointer_cast<ServiceStopDaemonMessage>(message)) {
            // There shouldn't be any need to clean up any state
            return false;
        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceStandardJobDoneMessage>(message)) {
            processStandardJobCompletion(msg->job, msg->compute_service);
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<ComputeServiceStandardJobFailedMessage>(message)) {
            processStandardJobFailure(msg->job, msg->compute_service, msg->cause);
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<ComputeServicePilotJobStartedMessage>(message)) {
            processPilotJobStart(msg->job, msg->compute_service);
            return true;
        } else if (auto msg = std::dynamic_pointer_cast<ComputeServicePilotJobExpiredMessage>(message)) {
            processPilotJobExpiration(msg->job, msg->compute_service);
            return true;
        } else {
            throw std::runtime_error("JobManager::main(): Unexpected [" + message->getName() + "] message");
        }
    }


    /**
     * @brief Process a standard job completion
     * @param job: the job that completed
     * @param compute_service: the compute service on which the job was executed
     */
    void JobManager::processStandardJobCompletion(StandardJob *job, std::shared_ptr<ComputeService> compute_service) {

        // update job state
        job->state = StandardJob::State::COMPLETED;

        // Determine all task state changes
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        for (auto task : job->tasks) {

            if (task->getInternalState() == WorkflowTask::InternalState::TASK_COMPLETED) {
                if (task->getUpcomingState() != WorkflowTask::State::COMPLETED) {
                    if (necessary_state_changes.find(task) == necessary_state_changes.end()) {
                        necessary_state_changes.insert(std::make_pair(task, WorkflowTask::State::COMPLETED));
                    } else {
                        necessary_state_changes[task] = WorkflowTask::State::COMPLETED;
                    }
                }
            } else {
                throw std::runtime_error("JobManager::main(): got a 'job done' message, but task " +
                                         task->getID() + " does not have a TASK_COMPLETED internal state (" +
                                         WorkflowTask::stateToString(task->getInternalState()) + ")");
            }
            auto children = task->getWorkflow()->getTaskChildren(task);
            for (auto child : children) {
                switch (child->getInternalState()) {
                    case WorkflowTask::InternalState::TASK_NOT_READY:
                        if (child->getState() != WorkflowTask::State::NOT_READY) {
                            throw std::runtime_error(
                                    "JobManager::main(): Child's internal state if NOT READY, but child's visible state is " +
                                    WorkflowTask::stateToString(child->getState()));
                        }
                    case WorkflowTask::InternalState::TASK_COMPLETED:
                        break;
                    case WorkflowTask::InternalState::TASK_FAILED:
                    case WorkflowTask::InternalState::TASK_RUNNING:
                        // no nothing
                        throw std::runtime_error("JobManager::main(): should never happen: " +
                                                 WorkflowTask::stateToString(child->getInternalState()));
                        break;
                    case WorkflowTask::InternalState::TASK_READY:
                        if (child->getState() == WorkflowTask::State::NOT_READY) {
                            bool all_parents_visibly_completed = true;
                            for (auto parent : child->getWorkflow()->getTaskParents(child)) {
                                if (parent->getState() == WorkflowTask::State::COMPLETED) {
                                    continue; // COMPLETED FROM BEFORE
                                }
                                if (parent->getUpcomingState() == WorkflowTask::State::COMPLETED) {
                                    continue; // COMPLETED FROM BEFORE, BUT NOT YET SEEN BY WMS
                                }
                                if ((necessary_state_changes.find(parent) != necessary_state_changes.end()) &&
                                    (necessary_state_changes[parent] == WorkflowTask::State::COMPLETED)) {
                                    continue; // ABOUT TO BECOME COMPLETED
                                }
                                all_parents_visibly_completed = false;
                                break;
                            }
                            if (all_parents_visibly_completed) {
                                if (necessary_state_changes.find(child) == necessary_state_changes.end()) {
                                    necessary_state_changes.insert(
                                            std::make_pair(child, WorkflowTask::State::READY));
                                } else {
                                    necessary_state_changes[child] = WorkflowTask::State::READY;
                                }
                            }
                        }
                        break;
                }
            }
        }

        // move the job from the "pending" list to the "completed" list
        this->pending_standard_jobs.erase(job);
        this->completed_standard_jobs.insert(job);

        /*
        WRENCH_INFO("HERE ARE NECESSARY STATE CHANGES");
        for (auto s : necessary_state_changes) {
          WRENCH_INFO("  STATE(%s) = %s", s.first->getID().c_str(),
          WorkflowTask::stateToString(s.second).c_str());
        }
        */

        for (auto s : necessary_state_changes) {
            s.first->setUpcomingState(s.second);
        }

        // Forward the notification along the notification chain
        std::string callback_mailbox = job->popCallbackMailbox();
        if (not callback_mailbox.empty()) {
            auto augmented_msg = new JobManagerStandardJobDoneMessage(
                    job, compute_service, necessary_state_changes);
            S4U_Mailbox::dputMessage(callback_mailbox, augmented_msg);
        }
    }


    /**
     * @brief Process a standard job failure
     * @param job: the job that failure
     * @param compute_service: the compute service on which the job has failed
     * @param failure_cause: the cause of the failure
     */
    void JobManager::processStandardJobFailure(StandardJob *job, std::shared_ptr<ComputeService> compute_service, std::shared_ptr<FailureCause> cause) {

        // update job state
        job->state = StandardJob::State::FAILED;

        // Determine all task state changes and failure count updates
        std::map<WorkflowTask *, WorkflowTask::State> necessary_state_changes;
        std::set<WorkflowTask *> necessary_failure_count_increments;

        for (auto t: job->getTasks()) {

            if (t->getInternalState() == WorkflowTask::InternalState::TASK_COMPLETED) {
                if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                    necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::COMPLETED));
                } else {
                    necessary_state_changes[t] = WorkflowTask::State::COMPLETED;
                }
                for (auto child : this->wms->getWorkflow()->getTaskChildren(t)) {
                    switch (child->getInternalState()) {
                        case WorkflowTask::InternalState::TASK_NOT_READY:
                        case WorkflowTask::InternalState::TASK_RUNNING:
                        case WorkflowTask::InternalState::TASK_FAILED:
                        case WorkflowTask::InternalState::TASK_COMPLETED:
                            // no nothing
                            break;
                        case WorkflowTask::InternalState::TASK_READY:
                            if (necessary_state_changes.find(child) == necessary_state_changes.end()) {
                                necessary_state_changes.insert(
                                        std::make_pair(child, WorkflowTask::State::READY));
                            } else {
                                necessary_state_changes[child] = WorkflowTask::State::READY;
                            }
                            break;
                    }
                }

            } else if (t->getInternalState() == WorkflowTask::InternalState::TASK_READY) {
                if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                    necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::READY));
                } else {
                    necessary_state_changes[t] = WorkflowTask::State::READY;
                }
                necessary_failure_count_increments.insert(t);

            } else if (t->getInternalState() == WorkflowTask::InternalState::TASK_FAILED) {
                bool ready = true;
                for (auto parent : this->wms->getWorkflow()->getTaskParents(t)) {
                    if ((parent->getInternalState() != WorkflowTask::InternalState::TASK_COMPLETED) and
                        (parent->getUpcomingState() != WorkflowTask::State::COMPLETED)) {
                        ready = false;
                    }
                }
                if (ready) {
                    t->setState(WorkflowTask::State::READY);
                    if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                        necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::READY));
                    } else {
                        necessary_state_changes[t] = WorkflowTask::State::READY;
                    }
                } else {
                    t->setState(WorkflowTask::State::NOT_READY);
                    if (necessary_state_changes.find(t) == necessary_state_changes.end()) {
                        necessary_state_changes.insert(std::make_pair(t, WorkflowTask::State::NOT_READY));
                    } else {
                        necessary_state_changes[t] = WorkflowTask::State::NOT_READY;
                    }
                }
            }
        }

        // remove the job from the "pending" list
        this->pending_standard_jobs.erase(job);
        // put it in the "failed" list
        this->failed_standard_jobs.insert(job);

        // Forward the notification along the notification chain
        JobManagerStandardJobFailedMessage *augmented_message =
                new JobManagerStandardJobFailedMessage(job, compute_service,
                                                       necessary_state_changes,
                                                       necessary_failure_count_increments,
                                                       std::move(cause));
        S4U_Mailbox::dputMessage(job->popCallbackMailbox(), augmented_message);
    }

    /**
     * @brief Process a pilot job starting
     * @param job: the pilot job that started
     * @param compute_service: the compute service on which it started
     */
    void JobManager::processPilotJobStart(PilotJob *job, std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = PilotJob::State::RUNNING;

        // move the job from the "pending" list to the "running" list
        this->pending_pilot_jobs.erase(job);
        this->running_pilot_jobs.insert(job);

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobStartedMessage(job, compute_service, 0.0));
    }

    /**
     * @brief Process a pilot job expiring
     * @param job: the pilot job that expired
     * @param compute_service: the compute service on which it was running
     */
    void JobManager::processPilotJobExpiration(PilotJob *job, std::shared_ptr<ComputeService> compute_service) {
        // update job state
        job->state = PilotJob::State::EXPIRED;

        // Remove the job from the "running" list and put it in the completed list
        this->running_pilot_jobs.erase(job);
        this->completed_pilot_jobs.insert(job);

        // Forward the notification to the source
        WRENCH_INFO("Forwarding to %s", job->getOriginCallbackMailbox().c_str());
        S4U_Mailbox::dputMessage(job->getOriginCallbackMailbox(),
                                 new ComputeServicePilotJobExpiredMessage(job, compute_service, 0.0));

    }


};
