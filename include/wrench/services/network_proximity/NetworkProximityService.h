/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_NETWORKPROXIMITYSERVICE_H
#define WRENCH_NETWORKPROXIMITYSERVICE_H

#include <complex>
#include "wrench/services/Service.h"
#include "wrench/services/network_proximity/NetworkProximityServiceProperty.h"
#include "wrench/services/network_proximity/NetworkProximityDaemon.h"

namespace wrench{

    /**
     * @brief A network proximity service
     */
    class NetworkProximityService: public Service {

    private:
        std::map<std::string, std::string> default_property_values =
                {{NetworkProximityServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD,                "1024"},
                 {NetworkProximityServiceProperty::DAEMON_STOPPED_MESSAGE_PAYLOAD,             "1024"},
                 {NetworkProximityServiceProperty::NETWORK_DB_LOOKUP_MESSAGE_PAYLOAD,          "1024"},
                 {NetworkProximityServiceProperty::NETWORK_DAEMON_CONTACT_ANSWER_PAYLOAD,      "1024"},
                 {NetworkProximityServiceProperty::LOOKUP_OVERHEAD,                            "0.0"},
                 {NetworkProximityServiceProperty::NETWORK_PROXIMITY_SERVICE_TYPE,             "ALLTOALL"}, // how are we error checking bad properties?
                 {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MESSAGE_SIZE,             "1024"}, // how are we error checking bad properties?
                 {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD,       "10"}, // how are we error checking bad properties?
                 {NetworkProximityServiceProperty::NETWORK_PROXIMITY_MEASUREMENT_PERIOD_MAX_NOISE, "20"}, // how are we error checking bad properties?
                 {NetworkProximityServiceProperty::NETWORK_DAEMON_COMMUNICATION_COVERAGE,    "1.0"}, // how are we error checking bad properties?
                };

    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        ~NetworkProximityService();

        /***********************/
        /** \endcond           */
        /***********************/

        NetworkProximityService(std::string db_hostname,
                                std::vector<std::string> hosts_in_network,
                                std::map<std::string, std::string> = {});

        void start();

        double query(std::pair<std::string, std::string> hosts);

        // TODO: add function to return coordinate of desired network daemon
        std::pair<double, double> getCoordinate(std::string);

    private:

        friend class Simulation;

        std::vector<std::shared_ptr<NetworkProximityDaemon>> network_daemons;
        std::vector<std::string> hosts_in_network;

        int main();

        bool processNextMessage();

        void addEntryToDatabase(std::pair<std::string,std::string> pair_hosts,double proximity_value);

        std::map<std::pair<std::string,std::string>,double> entries;

        std::map<std::string, std::complex<double>> coordinate_lookup_table;

        std::shared_ptr<NetworkProximityDaemon> getCommunicationPeer(std::string);

        void validateProperties();
    };
}




#endif //WRENCH_NETWORKPROXIMITYSERVICE_H
