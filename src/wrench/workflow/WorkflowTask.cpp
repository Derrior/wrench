/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  @brief WRENCH::WorkflowFile represents a data file used in a WRENCH::Workflow.
 */

#include <lemon/list_graph.h>
#include "WorkflowTask.h"
#include "Workflow.h"

namespace wrench {

	/**
	 * @brief Constructor
	 *
	 * @param string is the task id
	 * @param t is the task execution time
	 * @param n is the number of processors for running the task
	 */
	WorkflowTask::WorkflowTask(const std::string string, const double t, const int n) {
		this->id = string;
		this->flops = t;
		this->number_of_processors = n;
		this->state = WorkflowTask::READY;
		this->job = nullptr;
	}

	/**
	 * @brief Add an input file to the task
	 *
	 * @param f is a pointer to the file
	 */
	void WorkflowTask::addInputFile(WorkflowFile *f) {
		addFileToMap(input_files, f);

		f->setInputOf(this);

		// Perhaps add a control dependency?
		if (f->getOutputOf()) {
			workflow->addControlDependency(f->getOutputOf(), this);
		}
	}

	/**
	 * @brief Add an output file to the task
	 *
	 * @param f is a pointer to the file
	 */
	void WorkflowTask::addOutputFile(WorkflowFile *f) {
		addFileToMap(output_files, f);
		f->setOutputOf(this);
		// Perhaps add control dependencies?
		for (auto const &x : f->getInputOf()) {
			workflow->addControlDependency(this, x.second);
		}

	}

	/**
	 * @brief Helper method to add a file to a map if necessary
	 *
	 * @param map is the map
	 * @param f is a pointer to a file
	 */
	void WorkflowTask::addFileToMap(std::map<std::string, WorkflowFile *> map,
	                                WorkflowFile *f) {
		map[f->id] = f;
	}

	/**
	 * @brief Get the id of the task
	 *
	 * @return the id
	 */
	std::string WorkflowTask::getId() {
		return this->id;
	}

	/**
	 * @brief Get the number of flops of the task
	 *
	 * @return the number of flops
	 */
	double WorkflowTask::getFlops() {
		return this->flops;
	}

	/**
	 * @brief Get the number of children of a task
	 *
	 * @return number of children
	 */
	int WorkflowTask::getNumberOfChildren() {
		int count = 0;
		for (ListDigraph::OutArcIt a(*DAG, DAG_node); a != INVALID; ++a) {
			++count;
		}
		return count;
	}

	/**
	 * @brief Get the number of parents of a task
	 *
	 * @return number of parents
	 */
	int WorkflowTask::getNumberOfParents() {
		int count = 0;
		for (ListDigraph::InArcIt a(*DAG, DAG_node); a != INVALID; ++a) {
			++count;
		}
		return count;
	}

	/**
	 * @brief Get the state of the task
	 *
	 * @return task state
	 */
	WorkflowTask::State WorkflowTask::getState() {
		return state;
	}

	/**
	 * @brief Set the state of the task
	 */
	void WorkflowTask::setState(WorkflowTask::State state) {
		this->state = state;
	}

	/**
	 * @brief Set the task to the ready state
	 * @param task
	 */
	void WorkflowTask::setReady() {
		this->workflow->updateTaskState(this, WorkflowTask::READY);
	}

	/**
	 * @brief Set the task to the running state
	 * @param task
	 */
	void WorkflowTask::setRunning() {
		this->workflow->updateTaskState(this, WorkflowTask::RUNNING);
	}

	/**
	 * @brief Set the task to the completed state
	 * @param task
	 */
	void WorkflowTask::setCompleted() {
		this->workflow->updateTaskState(this, WorkflowTask::COMPLETED);
	}


};

