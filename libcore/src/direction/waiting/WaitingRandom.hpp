/**
 * \file        WaitingRandom.h
 * \date        May 14, 2019
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
 * Implementation of a waiting strategy:
 * Goal of this strategy is a random point of the subroom or waiting area the pedestrian is in.
 **/
#pragma once

#include "WaitingStrategy.hpp"
#include "geometry/Building.hpp"

#include <cstdlib>
#include <ctime>

class WaitingRandom : public WaitingStrategy
{
public:
    WaitingRandom() { std::srand(std::time(0)); }

private:
    Point GetWaitingPosition(const Room* room, const Pedestrian* ped, double time) override;
};
