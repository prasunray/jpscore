/**
 * Copyright (c) 2020 Forschungszentrum Jülich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 **/
#pragma once

#include "events/Event.hpp"
#include "general/Filesystem.hpp"

#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace EventFileParser
{
/// Custom Exception class to deliver all kinds of parsing errors.
class ParsingError : public std::runtime_error
{
public:
    explicit ParsingError(const char* msg);
    explicit ParsingError(const std::string& msg);
};

/**
 * Reads the events from a specific file and adds them directly to \p eventManager
 * @param eventFile File containing the events
 * @throws ParsingError on parsing error
 */
std::vector<DoorEvent> ParseDoorEvents(const fs::path& eventFile);

/**
 * Reads the events from a specific schedule file and adds them directly to \p eventManager
 * @param eventManager Manager for handling the events
 * @throws ParsingError on parsing error
 */
std::vector<DoorEvent> ParseSchedule(const fs::path& scheduleFile);

/**
 * Reads the max agents defintions from the schedule file and returns them as a map of transition ID
 * to maxAgents.
 * @param scheduleFile File containing the schedule
 * @return Map of max agents defintion which could be parsed from \p scheduleFile (ID to maxAgents)
 */
std::map<int, int> ParseMaxAgents(const fs::path& scheduleFile);
}; // namespace EventFileParser
