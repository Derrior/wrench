/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */

#include "PilotJob.h"

namespace wrench {

		PilotJob::PilotJob(int num_cores, double duration) {
			this->state = SUBMITTED;
			this->compute_service = nullptr;
			this->num_cores = num_cores;
			this->duration = duration;
		}

		PilotJob::State PilotJob::getState() {
			return this->state;
		}

		ComputeService *PilotJob::getComputeService() {
			return this->compute_service;
		}

		/***********************************************************/
		/**	UNDOCUMENTED PUBLIC/PRIVATE  METHODS AFTER THIS POINT **/
		/***********************************************************/

		/*! \cond PRIVATE */

		void PilotJob::setComputeService(ComputeService* cs) {
			this->compute_service = cs;
		}
		/*! \endcond */


};