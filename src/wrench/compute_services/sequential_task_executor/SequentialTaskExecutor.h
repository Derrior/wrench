/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::SequentialTaskExecutor class implements a simple
 *  sequential Compute Service abstraction.
 */

#ifndef SIMULATION_SEQUENTIALTASKEXECUTOR_H
#define SIMULATION_SEQUENTIALTASKEXECUTOR_H

#include <compute_services/ComputeService.h>
#include "compute_services/ComputeService.h"
#include "SequentialTaskExecutorDaemon.h"

namespace wrench {

	class Simulation;

	class SequentialTaskExecutor : ComputeService {

	public:
		SequentialTaskExecutor(std::string hostname, Simulation *simulation);
		SequentialTaskExecutor(std::string hostname);
		void stop();
		void kill();
		int runTask(WorkflowTask *task);
		bool hasIdleCore();

	private:
		std::string hostname;
		std::unique_ptr<SequentialTaskExecutorDaemon> daemon;

	};
};


#endif //SIMULATION_SEQUENTIALTASKEXECUTOR_H
