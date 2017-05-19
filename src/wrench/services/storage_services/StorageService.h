/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STORAGESERVICE_H
#define WRENCH_STORAGESERVICE_H


#include <string>
#include <set>

#include <services/Service.h>
#include <workflow_execution_events/WorkflowExecutionFailureCause.h>

namespace wrench {

    /***********************/
    /** \cond DEVELOPER   **/
    /***********************/

    class Simulation;
    class WorkflowFile;
    class WorkflowExecutionFailureCause;

    /**
     * @brief Abstract implementation of a storage service.
     */
    class StorageService : public Service {

    public:

        void stop();

        virtual double howMuchFreeSpace();

        virtual bool lookupFile(WorkflowFile *file);

        virtual void downloadFile(WorkflowFile *file);

        virtual void deleteFile(WorkflowFile *file);

        virtual void uploadFile(WorkflowFile *file);

        virtual void copyFile(WorkflowFile *file, StorageService *src);

        static void downloadFiles(std::set<WorkflowFile *> files,
                                  std::map<WorkflowFile *, StorageService *> file_locations,
                                  StorageService *default_storage_service);

        static void uploadFiles(std::set<WorkflowFile *> files,
                                  std::map<WorkflowFile *, StorageService *> file_locations,
                                  StorageService *default_storage_service);

        static void deleteFiles(std::set<WorkflowFile *>files,
                                std::map<WorkflowFile *, StorageService *> file_locations,
                                StorageService *default_storage_service);


        /***********************/
        /** \cond INTERNAL    **/
        /***********************/


        StorageService(std::string service_name,
                       std::string data_mailbox_name_prefix,
                       double capacity);

    protected:

        friend class Simulation;

        void addFileToStorage(WorkflowFile *);

        void removeFileFromStorage(WorkflowFile *);

        std::set<WorkflowFile *> stored_files;
        double capacity;
        double occupied_space = 0;

    private:

        enum Action {
            DOWNLOAD,
            UPLOAD,
        };

        static void uploadOrDownloadFiles(Action action, std::set<WorkflowFile *> files,
                                  std::map<WorkflowFile *, StorageService *> file_locations,
                                  StorageService *default_storage_service);


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H
