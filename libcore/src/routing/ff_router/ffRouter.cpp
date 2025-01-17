/**
 * \file        ffRouter.h
 * \date        Feb 19, 2016
 * \version     v0.8
 * \copyright   <2016-2022> Forschungszentrum Jülich GmbH. All rights reserved.
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
 * This router is an update of the former Router.{cpp, h} - Global-, Quickest
 * Router System. In the __former__ version, a graph was created with doors and
 * hlines as nodes and the distances of (doors, hlines), connected with a line-
 * of-sight, was used as edge-costs. If there was no line-of-sight, there was no
 * connecting edge. On the resulting graph, the Floyd-Warshall algorithm was
 * used to find any paths. In the "quickest-___" variants, the edge cost was not
 * determined by the distance, but by the distance multiplied by a speed-
 * estimate, to find the path with minimum travel times. This whole construct
 * worked pretty well, but dependend on hlines to create paths with line-of-
 * sights to the next target (hline/door).
 *
 * In the ffRouter, we want to overcome hlines by using floor fields to
 * determine the distances. A line of sight is not required any more. We hope to
 * reduce the graph complexity and the preparation-needs for new geometries.
 *
 * To find a counterpart for the "quickest-____" router, we can either use
 * __special__ floor fields, that respect the travel time in the input-speed map,
 * or take the distance-floor field and multiply it by a speed-estimate (analog
 * to the former construct.
 *
 * We will derive from the <Router> class to fit the interface.
 *
 **/
#include "ffRouter.hpp"

#include "direction/DirectionManager.hpp"
#include "direction/walking/DirectionStrategy.hpp"
#include "geometry/SubRoom.hpp"
#include "geometry/WaitingArea.hpp"
#include "math/OperationalModel.hpp"
#include "pedestrian/Pedestrian.hpp"
#include "routing/ff_router/mesh/RectGrid.hpp"

#include <Logger.hpp>
#include <stdexcept>

FFRouter::FFRouter(Configuration* config, Building* building, DirectionManager* directionManager)
    : _config(config), _directionManager(directionManager), _building(building)
{
    // depending on exit_strat 8 => false, depending on exit_strat 9 => true;
    _targetWithinSubroom = (false);
    CalculateFloorFields();
}

FFRouter::~FFRouter()
{
    // delete localffs
    std::map<int, UnivFFviaFM*>::reverse_iterator delIter;
    for(delIter = _floorfieldByRoomID.rbegin(); delIter != _floorfieldByRoomID.rend(); ++delIter) {
        delete(*delIter).second;
    }
}

bool FFRouter::ReInit()
{
    CalculateFloorFields();
    _needsRecalculation = false;
    return true;
}

void FFRouter::CalculateFloorFields()
{
    // cleanse maps
    _distMatrix.clear();
    _pathsMatrix.clear();

    // clear all maps
    _allDoorUIDs.clear();
    _exitsByUID.clear();
    _doorByUID.clear();

    // get all door UIDs
    std::vector<std::pair<int, int>> roomAndCroTrVector;

    for(const auto& [_, trans] : _building->GetAllTransitions()) {
        _allDoorUIDs.emplace_back(trans->GetUniqueID());
        _doorByUID.insert(std::make_pair(trans->GetUniqueID(), trans));
        if(trans->IsExit()) {
            _exitsByUID.insert(std::make_pair(trans->GetUniqueID(), trans));
        }
        Room* room1 = trans->GetRoom1();
        if(room1 != nullptr) {
            roomAndCroTrVector.emplace_back(std::make_pair(room1->GetID(), trans->GetUniqueID()));
        }
        Room* room2 = trans->GetRoom2();
        if(room2 != nullptr) {
            roomAndCroTrVector.emplace_back(std::make_pair(room2->GetID(), trans->GetUniqueID()));
        }
    }

    for(const auto& [_, cross] : _building->GetAllCrossings()) {
        _allDoorUIDs.emplace_back(cross->GetUniqueID());
        _doorByUID.insert(std::make_pair(cross->GetUniqueID(), cross));
        Room* room1 = cross->GetRoom1();
        if(room1 != nullptr) {
            roomAndCroTrVector.emplace_back(std::make_pair(room1->GetID(), cross->GetUniqueID()));
        }
    }

    for(auto const& [goalID, goal] : _building->GetAllGoals()) {
        // TODO add handling for doors with (almost) same distance
        //  ========      =========      =========
        //
        //       ------------------------------
        //       |           goal             |
        //       ------------------------------
        // if waiting area, add all transition and crossings (if needed)
        if(auto* waitingArea = dynamic_cast<WaitingArea*>(goal); waitingArea != nullptr) {
            auto* room = _building->GetRoom(waitingArea->GetRoomID());
            std::set<int> doorUIDs;

            if(!_targetWithinSubroom) {
                // candidates of current room (ID) (provided by Room)
                for(auto transUID : room->GetAllTransitionsIDs()) {
                    doorUIDs.emplace(transUID);
                }
                for(const auto& [_, subroom] : room->GetAllSubRooms()) {
                    for(const auto* cross : subroom->GetAllCrossings()) {
                        doorUIDs.emplace(cross->GetUniqueID());
                    }
                }
            } else {
                // candidates of current subroom only
                auto subroom = room->GetSubRoom(waitingArea->GetSubRoomID());
                for(const auto& crossing : subroom->GetAllCrossings()) {
                    doorUIDs.emplace(crossing->GetUniqueID());
                }
                for(const auto& transition : subroom->GetAllTransitions()) {
                    doorUIDs.emplace(transition->GetUniqueID());
                }
            }

            _doorsToGoalUID.emplace(goalID, doorUIDs);
        } else {
            // if goal find only closest exit
            double minDist = std::numeric_limits<double>::max();
            int minID = -1;

            for(const auto& [exitID, exit] : _exitsByUID) {
                double dist = goal->GetDistance(exit->GetCentre());
                if(dist < minDist) {
                    minDist = dist;
                    minID = exitID;
                }
            }
            _doorsToGoalUID.emplace(goalID, std::set<int>{minID}); // = minID;
        }
    }

    // make unique
    std::sort(_allDoorUIDs.begin(), _allDoorUIDs.end());
    _allDoorUIDs.erase(std::unique(_allDoorUIDs.begin(), _allDoorUIDs.end()), _allDoorUIDs.end());

    // init, yet no distances, only Create map entries
    for(auto& id1 : _allDoorUIDs) {
        for(auto& id2 : _allDoorUIDs) {
            std::pair<int, int> key = std::make_pair(id1, id2);
            // distMatrix[i][j] = 0,   if i==j
            // distMatrix[i][j] = max, else
            double value = (id1 == id2) ? 0.0 : std::numeric_limits<double>::infinity();
            _distMatrix.insert(std::make_pair(key, value));

            // pathsMatrix[i][j] = i or j ? (follow wiki:path_reconstruction, it should be j)
            _pathsMatrix.insert(std::make_pair(key, id2));
        }
    }

    // prepare all room-floor-fields-objects (one room = one instance)
    _floorfieldByRoomID.clear();
    for(const auto& [id, room] : _building->GetAllRooms()) {
        UnivFFviaFM* floorfield = new UnivFFviaFM{room.get(), 0.125, 0.0, false};

        floorfield->SetUser(DISTANCE_MEASUREMENTS_ONLY);
        floorfield->SetMode(CENTERPOINT);
        floorfield->SetSpeedMode(FF_HOMO_SPEED);
        floorfield->AddAllTargetsParallel();
        LOG_INFO("Adding distances in Room {:d} to matrix.", id);
        _floorfieldByRoomID.insert(std::make_pair(id, floorfield));
    }

    // nowait, because the parallel region ends directly afterwards
    //#pragma omp for nowait
    //@todo: @ar.graf: it would be easier to browse thru doors of each field directly after
    //"AddAllTargetsParallel" as
    //                 we do only want doors of same subroom anyway. BUT the router would have to
    //                 switch from room-scope to subroom-scope. Nevertheless, we could omit the room
    //                 info (used to access correct field), if we do it like in "ReInit()".
    for(const auto& [roomID1, doorUID1] : roomAndCroTrVector) {
        // loop over upper triangular matrix (i,j) and write to (j,i) as well
        for(const auto& [roomID2, doorUID2] : roomAndCroTrVector) {
            if(roomID2 != roomID1)
                continue; // we only want doors with one room in common
            if(doorUID2 <= doorUID1)
                continue; // calculate every path only once
            // if we exclude otherDoor.second == rctIt->second, the program loops forever

            // if the two doors are not within the same subroom, do not consider (ar.graf)
            // should fix problems of oscillation caused by doorgaps in the distancegraph
            int thisUID1 = (_doorByUID.at(doorUID1)->GetSubRoom1()) ?
                               _doorByUID.at(doorUID1)->GetSubRoom1()->GetUID() :
                               -10;
            int thisUID2 = (_doorByUID.at(doorUID1)->GetSubRoom2()) ?
                               _doorByUID.at(doorUID1)->GetSubRoom2()->GetUID() :
                               -20;
            int otherUID1 = (_doorByUID.at(doorUID2)->GetSubRoom1()) ?
                                _doorByUID.at(doorUID2)->GetSubRoom1()->GetUID() :
                                -30;
            int otherUID2 = (_doorByUID.at(doorUID2)->GetSubRoom2()) ?
                                _doorByUID.at(doorUID2)->GetSubRoom2()->GetUID() :
                                -40;

            if((thisUID1 != otherUID1) && (thisUID1 != otherUID2) && (thisUID2 != otherUID1) &&
               (thisUID2 != otherUID2)) {
                continue;
            }

            UnivFFviaFM* locffptr = _floorfieldByRoomID[roomID1];
            double tempDistance = locffptr->GetDistanceBetweenDoors(doorUID1, doorUID2);

            if(tempDistance < locffptr->GetGrid()->Gethx()) {
                LOG_WARNING(
                    "Ignoring distance of doors {:d} and {:d} because it is too small: "
                    "{:.2f}.",
                    doorUID1,
                    doorUID2,
                    tempDistance);
                continue;
            }

            //
            std::pair<int, int> key_ij = std::make_pair(doorUID2, doorUID1);
            std::pair<int, int> key_ji = std::make_pair(doorUID1, doorUID2);

            if(_distMatrix.at(key_ij) > tempDistance) {
                _distMatrix.erase(key_ij);
                _distMatrix.erase(key_ji);
                _distMatrix.insert(std::make_pair(key_ij, tempDistance));
                _distMatrix.insert(std::make_pair(key_ji, tempDistance));
            }
        } // otherDoor
    } // roomAndCroTrVector

    // penalize directional escalators
    std::vector<std::pair<int, int>> penaltyList;

    if(_config->hasDirectionalEscalators) {
        _directionalEscalatorsUID.clear();
        penaltyList.clear();
        for(const auto& room : _building->GetAllRooms()) {
            for(const auto& [_, subroom] : room.second->GetAllSubRooms()) {
                if((subroom->GetType() == SubroomType::ESCALATOR_UP) ||
                   (subroom->GetType() == SubroomType::ESCALATOR_DOWN)) {
                    _directionalEscalatorsUID.emplace_back(subroom->GetUID());
                }
            }
        }
        for(int subUID : _directionalEscalatorsUID) {
            auto* escalator = (Escalator*) _building->GetSubRoomByUID(subUID);
            std::vector<int> lineUIDs = escalator->GetAllGoalIDs();
            assert(lineUIDs.size() == 2);
            if(escalator->IsEscalatorUp()) {
                if(_doorByUID[lineUIDs[0]]->IsInLineSegment(escalator->GetUp())) {
                    penaltyList.emplace_back(std::make_pair(lineUIDs[0], lineUIDs[1]));
                } else {
                    penaltyList.emplace_back(std::make_pair(lineUIDs[1], lineUIDs[0]));
                }
            } else { // IsEscalatorDown
                if(_doorByUID[lineUIDs[0]]->IsInLineSegment(escalator->GetUp())) {
                    penaltyList.emplace_back(std::make_pair(lineUIDs[1], lineUIDs[0]));
                } else {
                    penaltyList.emplace_back(std::make_pair(lineUIDs[0], lineUIDs[1]));
                }
            }
        }
    }

    // penalize closed doors
    for(auto doorID : _allDoorUIDs) {
        if(auto door = dynamic_cast<Crossing*>(_building->GetTransOrCrossByUID(doorID))) {
            if(door->IsClose()) {
                for(auto doorID2 : _allDoorUIDs) {
                    if(doorID == doorID2) {
                        continue;
                    }
                    std::pair<int, int> connection1 = std::make_pair(doorID, doorID2);
                    std::pair<int, int> connection2 = std::make_pair(doorID2, doorID);

                    if(_distMatrix.find(connection1) != _distMatrix.end()) {
                        penaltyList.emplace_back(connection1);
                        penaltyList.emplace_back(connection2);
                    }
                }
            }
        }
    }

    for(auto key : penaltyList) {
        _distMatrix.erase(key);
        _distMatrix.insert(std::make_pair(key, std::numeric_limits<double>::max()));
    }

    FloydWarshall();
}

int FFRouter::FindExit(Pedestrian* p)
{
    auto [ped_roomid, ped_subroomid, _] = _building->GetRoomAndSubRoomIDs(p->GetPos());
    double minDist = std::numeric_limits<double>::infinity();
    int bestDoor = -1;

    int goalID = p->GetFinalDestination();

    if(auto* wa = dynamic_cast<WaitingArea*>(_building->GetFinalGoal(goalID))) {
        // if not subroom wise, check if correct room then direct to waiting area
        if(!_targetWithinSubroom && wa->GetRoomID() == ped_roomid) {
            bestDoor = wa->GetCentreCrossing()->GetUniqueID();
            p->SetDestination(bestDoor);
            p->SetExitLine(_doorByUID.at(bestDoor));
            return bestDoor;
        }
        // if subroom wise, check if correct subroom then direct to waiting area
        if(_targetWithinSubroom && wa->GetRoomID() == ped_roomid &&
           wa->GetSubRoomID() == ped_subroomid) {
            bestDoor = wa->GetCentreCrossing()->GetUniqueID();
            p->SetDestination(bestDoor);
            p->SetExitLine(_doorByUID.at(bestDoor));
            return bestDoor;
        }
    }

    std::vector<int> validFinalDoor; // UIDs of doors

    if(goalID == -1) {
        for(auto& pairDoor : _exitsByUID) {
            // we add all open/temp_close exits
            if(_building->GetTransitionByUID(pairDoor.first)->IsOpen() ||
               _building->GetTransitionByUID(pairDoor.first)->IsTempClose()) {
                validFinalDoor.emplace_back(pairDoor.first); // UID
            }
        }
    } else { // only one specific goal, goalToLineUIDmap gets
        // populated in Init()
        if((_doorsToGoalUID.count(goalID) == 0) || (_doorsToGoalUID.at(goalID).empty())) {
            LOG_ERROR("ffRouter: unknown/unreachable goalID: {:d} in FindExit(Ped)", goalID);
        } else {
            std::copy(
                std::begin(_doorsToGoalUID.at(goalID)),
                std::end(_doorsToGoalUID.at(goalID)),
                std::back_inserter(validFinalDoor));
        }
    }

    std::vector<int> DoorUIDsOfRoom;

    if(!_targetWithinSubroom) {
        // candidates of current room (ID) (provided by Room)
        for(auto transUID : _building->GetRoom(ped_roomid)->GetAllTransitionsIDs()) {
            if(_doorByUID.count(transUID) != 0) {
                DoorUIDsOfRoom.emplace_back(transUID);
            }
        }
        for(auto& subIPair : _building->GetRoom(ped_roomid)->GetAllSubRooms()) {
            for(auto& crossI : subIPair.second->GetAllCrossings()) {
                DoorUIDsOfRoom.emplace_back(crossI->GetUniqueID());
            }
        }
    } else {
        // candidates of current subroom only
        for(auto& crossI :
            _building->GetRoom(ped_roomid)->GetSubRoom(ped_subroomid)->GetAllCrossings()) {
            DoorUIDsOfRoom.emplace_back(crossI->GetUniqueID());
        }

        for(auto& transI :
            _building->GetRoom(ped_roomid)->GetSubRoom(ped_subroomid)->GetAllTransitions()) {
            if(transI->IsOpen() || transI->IsTempClose()) {
                DoorUIDsOfRoom.emplace_back(transI->GetUniqueID());
            }
        }
    }

    int bestFinalDoor = -1; // to silence the compiler
    for(int finalDoor : validFinalDoor) {
        // with UIDs, we can ask for shortest path
        for(int doorUID : DoorUIDsOfRoom) {
            double locDistToDoor =
                _directionManager->GetDirectionStrategy().GetDistance2Target(p, doorUID);

            if(locDistToDoor < -J_EPS) { // for old ff: //this can happen, if the point is not
                                         // reachable and therefore has init val -7
                continue;
            }
            std::pair<int, int> key = std::make_pair(doorUID, finalDoor);
            // only consider, if paths exists
            if(_pathsMatrix.count(key) == 0) {
                LOG_ERROR("ffRouter: no key for {:d} {:d}", key.first, key.second);
                continue;
            }

            if((_distMatrix.count(key) != 0) &&
               (_distMatrix.at(key) != std::numeric_limits<double>::infinity())) {
                if((_distMatrix.at(key) + locDistToDoor) < minDist) {
                    minDist = _distMatrix.at(key) + locDistToDoor;
                    bestDoor = key.first; // doorUID
                    auto subroomDoors = _building->GetSubRoom(p->GetPos())->GetAllGoalIDs();
                    if(std::find(subroomDoors.begin(), subroomDoors.end(), _pathsMatrix[key]) !=
                       subroomDoors.end()) {
                        bestDoor = _pathsMatrix[key]; //@todo: @ar.graf: check this hack
                    }
                    bestFinalDoor = key.second;
                }
            }
        }
    }

    // at this point, bestDoor is either a crossing or a transition
    if((!_targetWithinSubroom) && (_doorByUID.count(bestDoor) != 0)) {
        while(!_doorByUID[bestDoor]->IsTransition()) {
            std::pair<int, int> key = std::make_pair(bestDoor, bestFinalDoor);
            bestDoor = _pathsMatrix[key];
        }
    }

    if(_doorByUID.count(bestDoor)) {
        p->SetDestination(bestDoor);
        p->SetExitLine(_doorByUID.at(bestDoor));
    }
    return bestDoor; //-1 if no way was found, doorUID of best, if path found
}

void FFRouter::FloydWarshall()
{
    bool change = false;
    int totalnum = _allDoorUIDs.size();
    for(int k = 0; k < totalnum; ++k) {
        for(int i = 0; i < totalnum; ++i) {
            for(int j = 0; j < totalnum; ++j) {
                std::pair<int, int> key_ij = std::make_pair(_allDoorUIDs[i], _allDoorUIDs[j]);
                std::pair<int, int> key_ik = std::make_pair(_allDoorUIDs[i], _allDoorUIDs[k]);
                std::pair<int, int> key_kj = std::make_pair(_allDoorUIDs[k], _allDoorUIDs[j]);
                if((_distMatrix[key_ik] < std::numeric_limits<double>::max()) &&
                   (_distMatrix[key_kj] < std::numeric_limits<double>::max()) &&
                   (_distMatrix[key_ik] + _distMatrix[key_kj] < _distMatrix[key_ij])) {
                    _distMatrix.erase(key_ij);
                    _distMatrix.insert(
                        std::make_pair(key_ij, _distMatrix[key_ik] + _distMatrix[key_kj]));
                    _pathsMatrix.erase(key_ij);
                    _pathsMatrix.insert(std::make_pair(key_ij, _pathsMatrix[key_ik]));
                    change = true;
                }
            }
        }
    }
    if(change) {
        FloydWarshall();
    } else {
        LOG_INFO("ffRouter: FloydWarshall done!");
    }
}

bool FFRouter::MustReInit()
{
    return _needsRecalculation;
}

void FFRouter::SetRecalc(double t)
{
    _timeToRecalculation = t + _recalculationInterval;
}

void FFRouter::Update()
{
    CalculateFloorFields();
}
