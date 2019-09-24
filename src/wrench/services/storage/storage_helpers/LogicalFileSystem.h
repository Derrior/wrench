/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_LOGICALFILESYSTEM_H
#define WRENCH_LOGICALFILESYSTEM_H

#include <string>
#include <map>
#include <set>

#include <wrench/workflow/WorkflowFile.h>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/


    class LogicalFileSystem {

    public:

        explicit LogicalFileSystem(std::string hostname, std::string mount_point);

        double getTotalCapacity();
        bool hasEnoughFreeSpace(double bytes);
        double getFreeSpace();
        void decreaseFreeSpace(double num_bytes);
        void increaseFreeSpace(double num_bytes);

        void createDirectory(std::string absolute_path);
        bool doesDirectoryExist(std::string absolute_path);
        bool isDirectoryEmpty(std::string absolute_path);
        void removeEmptyDirectory(std::string absolute_path);
        void storeFileInDirectory(WorkflowFile *file, std::string absolute_path);
        void removeFileFromDirectory(WorkflowFile *file, std::string absolute_path);
        void removeAllFilesInDirectory(std::string absolute_path);
        bool isFileInDirectory(WorkflowFile *file, std::string absolute_path);
        std::set<WorkflowFile *> listFilesInDirectory(std::string absolute_path);


    private:

        static std::set<std::string> mount_points;


        std::string mount_point;
        std::map<std::string, std::set<WorkflowFile*>> content;
        double total_capacity;
        double occupied_space;

        void assertDirectoryExist(std::string absolute_path) {
            if (not this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " does not exist");
            }
        }

        void assertDirectoryDoesNotExist(std::string absolute_path) {
            if (this->doesDirectoryExist(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryExists(): directory " + absolute_path + " already exists");
            }
        }

        void assertDirectoryIsEmpty(std::string absolute_path) {
            assertDirectoryExist(absolute_path);
            if (not this->isDirectoryEmpty(absolute_path)) {
                throw std::invalid_argument("LogicalFileSystem::assertDirectoryIsEmpty(): directory " + absolute_path + "is not empty");
            }
        }

        void assertFileIsInDirectory(WorkflowFile *file, std::string absolute_path) {
            assertDirectoryExist(absolute_path);
            if (this->content[absolute_path].find(file) == this->content[absolute_path].end()) {
                throw std::invalid_argument("LogicalFileSystem::assertFileIsInDirectory(): File " + file->getID() +
                " is not in directory " + absolute_path);
            }
        }

    };


    /***********************/
    /** \endcond           */
    /***********************/

}


#endif //WRENCH_LOGICALFILESYSTEM_H