/**
 * \file        WaitingRandom.cpp
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
#include "WaitingRandom.hpp"

#include "geometry/SubRoom.hpp"
#include "pedestrian/Pedestrian.hpp"

double fRand(double fMin, double fMax)
{
    double f = static_cast<double>(std::rand()) / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

Point WaitingRandom::GetWaitingPosition(const Room* room, const Pedestrian* ped, double time)
{
    // Polygon of either subroom or waiting area
    std::vector<Point> polygon;

    // checks if ped is inside waiting area
    if(ped->IsInsideWaitingAreaWaiting(time)) {
        polygon = ped->GetBuilding()->GetFinalGoal(ped->GetLastGoalID())->GetPolygon();
    } else {
        SubRoom* subRoom = room->GetSubRoom(ped->GetPos());
        polygon = subRoom->GetPolygon();
    }

    double xMin = std::numeric_limits<double>::max(), xMax = std::numeric_limits<double>::min(),
           yMin = std::numeric_limits<double>::max(), yMax = std::numeric_limits<double>::min();

    // get the bounding box of subroom/waiting area
    for(const auto& poly : polygon) {
        xMin = (xMin <= poly.x) ? (xMin) : (poly.x);
        xMax = (xMax >= poly.x) ? (xMax) : (poly.x);

        yMin = (yMin <= poly.y) ? (yMin) : (poly.y);
        yMax = (yMax >= poly.y) ? (yMax) : (poly.y);
    }

    Point target;

    // generate random point inside subroom/waiting area
    if(ped->IsInsideWaitingAreaWaiting(time)) {
        auto goal = ped->GetBuilding()->GetFinalGoal(ped->GetLastGoalID());

        do {
            target.x = fRand(xMin, xMax);
            target.y = fRand(yMin, yMax);
        } while(!goal->IsInsideGoal(target));

    } else {
        SubRoom* subRoom = room->GetSubRoom(ped->GetPos());

        do {
            target.x = fRand(xMin, xMax);
            target.y = fRand(yMin, yMax);
        } while(!subRoom->IsInSubRoom(target));
    }

    return target;
}
