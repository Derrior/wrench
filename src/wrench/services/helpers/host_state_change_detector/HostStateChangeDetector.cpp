/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundatioff, either versioff 3 of the License, or
 * (at your optioff) any later versioff.
 */

#include "wrench/services/helpers/HostStateChangeDetector.h"
#include "wrench/services/helpers/HostStateChangeDetectorMessage.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>
#include <wrench/services/helpers/HostStateChangeDetector.h>


XBT_LOG_NEW_DEFAULT_CATEGORY(host_state_change_detector, "Log category for HostStateChangeDetector");


void wrench::HostStateChangeDetector::cleanup(bool has_terminated_cleanly, int return_value) {
    // Unregister the callbacks! (otherwise we'd get a segfault after destruction of this service)
    WRENCH_INFO("IN HOST STATE CHANGE DETECTOR DESTRUCTOR");
    simgrid::s4u::Host::on_state_change.disconnect_slots();
    simgrid::s4u::VirtualMachine::on_shutdown.disconnect_slots();
    simgrid::s4u::VirtualMachine::on_start.disconnect_slots();
}



/**
 * @brief Constructor
 * @param host_on_which_to_run: hosts on which this service runs
 * @param hosts_to_monitor: the list of hosts to monitor
 * @param notify_when_turned_on: whether to send a notifications when hosts turn on
 * @param notify_when_turned_off: whether to send a notifications when hosts turn off
 * @param mailbox_to_notify: the mailbox to notify
 * @param property_list: a property list
 *
 */
wrench::HostStateChangeDetector::HostStateChangeDetector(std::string host_on_which_to_run,
                                                         std::vector<std::string> hosts_to_monitor,
                                                         bool notify_when_turned_on, bool notify_when_turned_off,
                                                         std::shared_ptr<S4U_Daemon> creator,
                                                         std::string mailbox_to_notify,
                                                         std::map<std::string, std::string> property_list) :
        Service(host_on_which_to_run, "host_state_change_detector", "host_state_change_detector") {
    this->hosts_to_monitor = hosts_to_monitor;
    this->notify_when_turned_on = notify_when_turned_on;
    this->notify_when_turned_off = notify_when_turned_off;
    this->mailbox_to_notify = mailbox_to_notify;
    this->creator = creator;

    // Set default and specified properties
    this->setProperties(this->default_property_values, std::move(property_list));

    WRENCH_INFO("IN HOST STATE CHANGE CONSTRUCTOR!");
    // Connect my member method to the on_state_change signal from SimGrid regarding Hosts
    simgrid::s4u::Host::on_state_change.connect([this](simgrid::s4u::Host const &h) {this->hostChangeCallback(h.get_name(), h.is_on(), "HOST STATE CHANGE");});
    // Connect my member method to the on_state_change signal from SimGrid regarding VMs
//    simgrid::s4u::VirtualMachine::on_shutdown.connect([this](simgrid::s4u::VirtualMachine const &h) -> void {this->hostChangeCallback(h.get_name(),false, "VM SHUTDOWN");});
//    simgrid::s4u::VirtualMachine::on_start.connect([this](simgrid::s4u::VirtualMachine const &h) {this->hostChangeCallback(h.get_name(), true, "VM START");});
}

void wrench::HostStateChangeDetector::hostChangeCallback(std::string const &name, bool is_on, std::string message) {
    WRENCH_INFO("************************************");
    WRENCH_INFO("***** %s : IN CALLBACK: %s : %s ", this->getName().c_str(),message.c_str(), name.c_str());
    WRENCH_INFO("************************************");
    for (const auto &pm : S4U_VirtualMachine::simgrid_vm_pm_map) {
        WRENCH_INFO("PM = %s (is_on = %d)", pm.first->get_cname(), pm.first->is_on());
        for (const auto &vm : pm.second) {
            WRENCH_INFO("   VM = %s (is_on = %d)", vm->get_cname(), vm->is_on());
        }
    }
//    WRENCH_INFO("************************************");
//    WRENCH_INFO("HOSTS I CARE ABOUT: -");
//    for (auto const &host : this->hosts_to_monitor) {
//        WRENCH_INFO("  --> %s", host.c_str());
//    }
    // Is this is a physical host I care about?
    bool this_is_a_pm_i_care_about = (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), name) != this->hosts_to_monitor.end());

    if (this_is_a_pm_i_care_about) {
        if (S4U_VirtualMachine::simgrid_vm_pm_map.find(simgrid::s4u::Host::by_name(name)) != S4U_VirtualMachine::simgrid_vm_pm_map.end()) {
            // This seems to be a SimGrid bug:the VM are not killed! Kill them all
            for (auto &vm : S4U_VirtualMachine::simgrid_vm_pm_map[simgrid::s4u::Host::by_name(name)]) {
                WRENCH_INFO("BY HAND TURNING OFF THE VM %s", vm->get_cname());
                vm->turn_off();
            }
            S4U_VirtualMachine::simgrid_vm_pm_map[simgrid::s4u::Host::by_name(name)].clear();
            this->hosts_that_have_recently_changed_state.push_back(std::make_pair(name, is_on));
            return;
        }
    }

    // Is this a VM I care about?
    for (const auto &pm : S4U_VirtualMachine::simgrid_vm_pm_map) {
        for (const auto &vm : pm.second) {
            if (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), vm->get_name()) != this->hosts_to_monitor.end()) {
                if (vm->is_on()) {
                    WRENCH_INFO("BY HAND TURNING OFF VM %s", vm->get_cname());
                    vm->turn_off();
                } else {
                    WRENCH_INFO("GOOD, VM IS ALREADY OFF");
                }
                this->hosts_that_have_recently_changed_state.push_back(std::make_pair(name, is_on));
                S4U_VirtualMachine::simgrid_vm_pm_map[pm.first].erase(vm);
                return;
            }
        }
    }


//    WRENCH_INFO("*************** AFTER *********************");
//    WRENCH_INFO("SIZE OF MAP = %lu", S4U_VirtualMachine::simgrid_vm_pm_map.size());
//    for (const auto &pm : S4U_VirtualMachine::simgrid_vm_pm_map) {
//        WRENCH_INFO("PM = %s", pm.first->get_cname());
//        for (const auto &vm : pm.second) {
//            WRENCH_INFO("   VM = %s", vm->get_cname());
//        }
//    }
//    WRENCH_INFO("************************************");


}


int wrench::HostStateChangeDetector::main() {

    WRENCH_INFO("Starting");
    while(true) {
        if (creator->getState() == State::DOWN) {
            WRENCH_INFO("My Creator has terminated/died, so must I...");
            break;
        }
        // Sleeping for my monitoring period
        Simulation::sleep(this->getPropertyValueAsDouble(HostStateChangeDetectorProperty::MONITORING_PERIOD));

        while (not this->hosts_that_have_recently_changed_state.empty()) {
            auto host_info = this->hosts_that_have_recently_changed_state.at(0);
            std::string hostname = std::get<0>(host_info);
            bool new_state_is_on = std::get<1>(host_info);
            bool new_state_is_off  = not new_state_is_on;
            this->hosts_that_have_recently_changed_state.erase(this->hosts_that_have_recently_changed_state.begin());

            HostStateChangeDetectorMessage *msg;

            if (this->notify_when_turned_on && new_state_is_on) {
                msg = new HostHasTurnedOnMessage(hostname);
            } else if (this->notify_when_turned_off && new_state_is_off) {
                msg = new HostHasTurnedOffMessage(hostname);
            } else {
                continue;
            }

            WRENCH_INFO("Notifying mailbox '%s' that host '%s' has changed state", this->mailbox_to_notify.c_str(),
                        hostname.c_str());
            try {
                S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
            } catch (std::shared_ptr<NetworkError> &e) {
                WRENCH_INFO("Network error '%s' while notifying mailbox of a host state change ... ignoring",
                            e->toString().c_str());
            }

        }
    }
    return 0;
}

/**
 * @brief Kill the service
 */
void wrench::HostStateChangeDetector::kill() {
    this->killActor();
}