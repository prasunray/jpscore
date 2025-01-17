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
#include "WaitingStrategy.hpp"

#include "geometry/Point.hpp"
#include "geometry/Room.hpp"
#include "geometry/SubRoom.hpp"
#include "pedestrian/Pedestrian.hpp"

Point WaitingStrategy::GetTarget(const Room* room, const Pedestrian* ped, double time)
{
    Point waitingPos = ped->GetWaitingPos();
    Point target;

    // check if waiting pos is set
    if(waitingPos.x == std::numeric_limits<double>::max() &&
       waitingPos.y == std::numeric_limits<double>::max()) {
        SubRoom* subroom = ped->GetBuilding()->GetSubRoom(ped->GetPos());
        do {
            target = GetWaitingPosition(room, ped, time);
        } while(!subroom->IsInSubRoom(target));
    }
    // check if in close range to desired position, hard coded!
    else if((ped->GetWaitingPos() - ped->GetPos()).Norm() <= 0.1 && ped->GetV0Norm() < 0.5) {
        target = ped->GetPos();
    }
    // head to desired waiting position
    else {
        target = ped->GetWaitingPos();
    }

    return target;
}
