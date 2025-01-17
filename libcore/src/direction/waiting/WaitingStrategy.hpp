/**
 * \file        WaitingStrategy.h
 * \date        May 13, 2019
 * \version     v0.8.1
 * \copyright   <2009-2025> Forschungszentrum Jülich GmbH. All rights reserved.
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
 *
 * \section Description
 * Interface of a waiting strategy:
 *  - A desired walking direction at a certain time is computed
 *  - A desired waiting position is computed
 **/
#pragma once

#include "general/Macros.hpp"
#include "geometry/Building.hpp"

class Room;

class Pedestrian;

class Point;

class WaitingStrategy
{
public:
    virtual ~WaitingStrategy() = default;

    /**
     * Returns the desired walking direction of a pedestrian.
     * @param room Room the pedestrian is in
     * @param ped Pedestrians, whose walking direction is determined
     * @return Desired walking direction of ped
     */
    Point GetTarget(const Room* room, const Pedestrian* ped, double time);

    /**
     * Returns the desired waiting position of a pedestrian.
     * @param room Room the pedestrian is in
     * @param ped Pedestrians, whose walking direction is determined
     * @return Desired waiting position of ped
     */
    virtual Point GetWaitingPosition(const Room* room, const Pedestrian* ped, double time) = 0;
};
