/**
 * @file monitorfile.hpp
 * @brief MonitorFile is a C++ class to monitor a file in the filesystem and
 *        allows triggering an action when it changes.
 *
 * This software is distributed under the MIT License. See LICENSE.md for
 * details.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 */

#ifndef _MONITORFILE_HPP
#define _MONITORFILE_HPP

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

/**
 * @class MonitorFile
 * @brief Monitors a file for changes based on its last write time.
 */
class MonitorFile
{
public:
    /**
     * @brief Default constructor initializes timestamps.
     */
    MonitorFile() : org_time{}, new_time{} {}

    /**
     * @brief Initializes file monitoring for a specified file.
     * @param fileName The name of the file to monitor.
     * @throws std::runtime_error If the file does not exist.
     */
    void filemon(const std::string &fileName)
    {
        if (!fs::exists(fileName))
        {
            throw std::runtime_error("File does not exist: " + fileName);
        }

        file_name = fileName;
        start_monitoring();
    }

    /**
     * @brief Checks if the monitored file has been modified since the last check.
     * @return `true` if the file has changed; otherwise, `false`.
     * @throws std::runtime_error If the file is missing during a check.
     */
    bool changed()
    {
        check_file();

        if (new_time > org_time)
        {
            org_time = new_time; // Ensure updates are consistently detected
            return true;
        }
        return false;
    }

private:
    /**
     * @brief Initializes the monitoring process by recording the file's current last write time.
     */
    void start_monitoring()
    {
        org_time = fs::last_write_time(file_name);
    }

    /**
     * @brief Updates the recorded last write time of the monitored file.
     * @throws std::runtime_error If the file is missing during a check.
     */
    void check_file()
    {
        if (!fs::exists(file_name))
        {
            throw std::runtime_error("File no longer exists: " + file_name);
        }

        new_time = fs::last_write_time(file_name);
    }

    std::string file_name;       ///< Name of the file being monitored.
    fs::file_time_type org_time; ///< Original recorded last write time of the file.
    fs::file_time_type new_time; ///< Updated last write time of the file.
};

#endif // _MONITORFILE_HPP
