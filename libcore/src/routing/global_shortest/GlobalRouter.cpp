/**
 * \file        GlobalRouter.cpp
 * \date        Dec 15, 2010
 * \version     v0.7
 * \copyright   <2009-2015> Forschungszentrum J�lich GmbH. All rights reserved.
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
 *
 *
 **/
#include "GlobalRouter.hpp"

#include "AccessPoint.hpp"
#include "geometry/DTriangulation.hpp"
#include "geometry/Line.hpp"
#include "geometry/SubRoom.hpp"
#include "geometry/Wall.hpp"
#include "pedestrian/Pedestrian.hpp"

#include <Logger.hpp>
#include <tinyxml.h>

GlobalRouter::GlobalRouter(Building* building, const GlobalRouterParameters& parameters)
    : _useMeshForLocalNavigation(parameters.useMeshForLocalNavigation)
    , _generateNavigationMesh(parameters.generateNavigationMesh)
    , _minDistanceBetweenTriangleEdges(parameters.minDistanceBetweenTriangleEdges)
    , _minAngleInTriangles(parameters.minAngleInTriangles)
    , _building(building)
{
    _accessPoints = std::map<int, AccessPoint*>();
    _map_id_to_index = std::map<int, int>();
    _map_index_to_id = std::map<int, int>();
    _distMatrix = nullptr;
    _pathsMatrix = nullptr;
    _edgeCost = 100;
    _exitsCnt = -1;
    if(const auto success = init(); !success) {
        throw std::runtime_error("Error creating GlobalRouter");
    }
}

GlobalRouter::~GlobalRouter()
{
    if(_distMatrix && _pathsMatrix) {
        for(int p = 0; p < _exitsCnt; ++p) {
            delete[] _distMatrix[p];
            delete[] _pathsMatrix[p];
        }

        delete[] _distMatrix;
        delete[] _pathsMatrix;
    }

    std::map<int, AccessPoint*>::const_iterator itr;
    for(itr = _accessPoints.begin(); itr != _accessPoints.end(); ++itr) {
        delete itr->second;
    }
    _accessPoints.clear();
}

bool GlobalRouter::init()
{
    // necessary if the init is called several times during the simulation
    Reset();
    LOG_DEBUG("Init the Global Router Engine");

    if(_generateNavigationMesh) {
        TriangulateGeometry();
    }

    // initialize the distances matrix for the floydwahrshall
    _exitsCnt = _building->GetNumberOfGoals() + _building->GetAllGoals().size();

    _distMatrix = new double*[_exitsCnt];
    _pathsMatrix = new int*[_exitsCnt];

    for(int i = 0; i < _exitsCnt; ++i) {
        _distMatrix[i] = new double[_exitsCnt];
        _pathsMatrix[i] = new int[_exitsCnt];
    }

    // Initializing the values
    // all nodes are disconnected
    for(int p = 0; p < _exitsCnt; ++p) {
        for(int r = 0; r < _exitsCnt; ++r) {
            _distMatrix[p][r] = (r == p) ? 0.0 : FLT_MAX; /*0.0*/
            _pathsMatrix[p][r] = p; /*0.0*/
        }
    }

    // init the access points
    int index = 0;
    for(const auto& itr : _building->GetAllHlines()) {
        // int door=itr->first;
        int door = itr.second->GetUniqueID();
        Hline* cross = itr.second;
        Point centre = cross->GetCentre();
        double center[2] = {centre.x, centre.y};

        AccessPoint* ap = new AccessPoint(door, center);
        ap->SetNavLine(cross);
        char friendlyName[1024];
        sprintf(
            friendlyName,
            "hline_%d_room_%d_subroom_%d",
            cross->GetID(),
            cross->GetRoom1()->GetID(),
            cross->GetSubRoom1()->GetSubRoomID());
        ap->SetFriendlyName(friendlyName);

        // save the connecting sub/rooms IDs
        int id1 = -1;
        if(cross->GetSubRoom1()) {
            id1 = cross->GetSubRoom1()->GetUID();
        }

        ap->setConnectingRooms(id1, id1);
        _accessPoints[door] = ap;

        // very nasty
        _map_id_to_index[door] = index;
        _map_index_to_id[index] = door;
        index++;
    }

    for(const auto& itr : _building->GetAllCrossings()) {
        int door = itr.second->GetUniqueID();
        Crossing* cross = itr.second;
        const Point& centre = cross->GetCentre();
        double center[2] = {centre.x, centre.y};

        AccessPoint* ap = new AccessPoint(door, center);
        ap->SetNavLine(cross);
        char friendlyName[1024];
        sprintf(
            friendlyName,
            "cross_%d_room_%d_subroom_%d",
            cross->GetID(),
            cross->GetRoom1()->GetID(),
            cross->GetSubRoom1()->GetSubRoomID());
        ap->SetFriendlyName(friendlyName);

        //          ap->SetClosed(cross->IsClose());
        ap->SetState(cross->GetState());
        // save the connecting sub/rooms IDs
        int id1 = -1;
        if(cross->GetSubRoom1()) {
            id1 = cross->GetSubRoom1()->GetUID();
        }

        int id2 = -1;
        if(cross->GetSubRoom2()) {
            id2 = cross->GetSubRoom2()->GetUID();
        }

        ap->setConnectingRooms(id1, id2);
        _accessPoints[door] = ap;

        // very nasty
        _map_id_to_index[door] = index;
        _map_index_to_id[index] = door;
        index++;
    }

    for(const auto& itr : _building->GetAllTransitions()) {
        int door = itr.second->GetUniqueID();
        Transition* cross = itr.second;
        const Point& centre = cross->GetCentre();
        double center[2] = {centre.x, centre.y};

        AccessPoint* ap = new AccessPoint(door, center);
        ap->SetNavLine(cross);
        char friendlyName[1024];
        sprintf(
            friendlyName,
            "trans_%d_room_%d_subroom_%d",
            cross->GetID(),
            cross->GetRoom1()->GetID(),
            cross->GetSubRoom1()->GetSubRoomID());
        ap->SetFriendlyName(friendlyName);

        //          ap->SetClosed(cross->IsClose());
        ap->SetState(cross->GetState());
        // save the connecting sub/rooms IDs
        int id1 = -1;
        if(cross->GetSubRoom1()) {
            id1 = cross->GetSubRoom1()->GetUID();
        }

        int id2 = -1;
        if(cross->GetSubRoom2()) {
            id2 = cross->GetSubRoom2()->GetUID();
        }

        ap->setConnectingRooms(id1, id2);
        _accessPoints[door] = ap;

        // set the final destination
        if(cross->IsExit() && !cross->IsClose()) {
            ap->SetFinalExitToOutside(true);
            LOG_INFO("Exit to outside found: {:d} [{}]", ap->GetID(), ap->GetFriendlyName());
        } else if((id1 == -1) && (id2 == -1)) {
            LOG_INFO("Final destination outside the geometry was found");
            ap->SetFinalExitToOutside(true);
        } else if(cross->GetRoom1()->GetCaption() == "outside") {
            ap->SetFinalExitToOutside(true);
        }

        // very nasty
        _map_id_to_index[door] = index;
        _map_index_to_id[index] = door;
        index++;
    }

    // populate the subrooms at the elevation
    for(auto&& itroom : _building->GetAllRooms()) {
        auto&& room = (std::shared_ptr<Room> &&) itroom.second;
        for(const auto& it_sub : room->GetAllSubRooms()) {
            auto&& sub = (std::shared_ptr<SubRoom> &&) it_sub.second;
            // maybe truncate the elevation.
            //  because using a double as key to map is not exact
            // double elevation =  ceilf(sub->GetMaxElevation() * 100) / 100;
            //_subroomsAtElevation[elevation].push_back(sub.get());
            _subroomsAtElevation[sub->GetElevation(sub->GetCentroid())].push_back(sub.get());
        }
    }

    // loop over the rooms
    // loop over the subrooms
    // get the transitions in the subrooms
    // and compute the distances

    for(auto&& itroom : _building->GetAllRooms()) {
        auto&& room = (std::shared_ptr<Room> &&) itroom.second;
        for(const auto& it_sub : room->GetAllSubRooms()) {
            // The penalty factor should discourage pedestrians to evacuation through rooms
            auto&& sub = (std::shared_ptr<SubRoom> &&) it_sub.second;
            double penalty = 1.0;
            if((sub->GetType() != SubroomType::FLOOR) && (sub->GetType() != SubroomType::DA)) {
                penalty = _edgeCost;
            }

            // collect all navigation objects
            std::vector<Hline*> allGoals;
            const auto& crossings = sub->GetAllCrossings();
            allGoals.insert(allGoals.end(), crossings.begin(), crossings.end());
            const auto& transitions = sub->GetAllTransitions();
            allGoals.insert(allGoals.end(), transitions.begin(), transitions.end());
            const auto& hlines = sub->GetAllHlines();
            allGoals.insert(allGoals.end(), hlines.begin(), hlines.end());

            // process the hlines
            // process the crossings
            // process the transitions
            for(unsigned int n1 = 0; n1 < allGoals.size(); n1++) {
                Hline* nav1 = allGoals[n1];
                AccessPoint* from_AP = _accessPoints[nav1->GetUniqueID()];
                int from_door = _map_id_to_index[nav1->GetUniqueID()];
                if(from_AP->IsClosed())
                    continue;

                for(unsigned int n2 = 0; n2 < allGoals.size(); n2++) {
                    Hline* nav2 = allGoals[n2];
                    AccessPoint* to_AP = _accessPoints[nav2->GetUniqueID()];
                    if(to_AP->IsClosed())
                        continue;

                    if(n1 == n2)
                        continue;
                    if(nav1->operator==(*nav2))
                        continue;

                    double elevation = sub->GetElevation(sub->GetCentroid());
                    // special case for stairs and for convex rooms

                    if(_building->IsVisible(
                           nav1->GetCentre(),
                           nav2->GetCentre(),
                           _subroomsAtElevation[elevation],
                           true)) {
                        int to_door = _map_id_to_index[nav2->GetUniqueID()];
                        _distMatrix[from_door][to_door] =
                            penalty * (nav1->GetCentre() - nav2->GetCentre()).Norm();
                        from_AP->AddConnectingAP(_accessPoints[nav2->GetUniqueID()]);
                    }
                }
            }
        }
    }

    // complete the matrix with the final distances between the exits to the outside and the
    // final marked goals

    for(const auto& [_, goal] : _building->GetAllGoals()) {
        const Wall& line = goal->GetAllWalls()[0];
        double center[2] = {goal->GetCentroid().x, goal->GetCentroid().y};

        AccessPoint* to_AP = new AccessPoint(line.GetUniqueID(), center);
        to_AP->SetFinalGoalOutside(true);
        Line tmpline(line);
        to_AP->SetNavLine(&tmpline);

        char friendlyName[1024];
        sprintf(friendlyName, "finalGoal_%d_located_outside", goal->GetId());
        to_AP->SetFriendlyName(friendlyName);
        to_AP->AddFinalDestination(FINAL_DEST_OUT, 0.0);
        to_AP->AddFinalDestination(goal->GetId(), 0.0);
        _accessPoints[to_AP->GetID()] = to_AP;

        // very nasty
        _map_id_to_index[to_AP->GetID()] = index;
        _map_index_to_id[index] = to_AP->GetID();
        index++;

        // only make a connection to final exit to outside
        for(const auto& itr1 : _accessPoints) {
            AccessPoint* from_AP = itr1.second;
            if(!from_AP->GetFinalExitToOutside())
                continue;
            if(from_AP->GetID() == to_AP->GetID())
                continue;
            from_AP->AddConnectingAP(to_AP);
            int from_door = _map_id_to_index[from_AP->GetID()];
            int to_door = _map_id_to_index[to_AP->GetID()];
            // I assume a direct line connection between every exit connected to the outside and
            // any final goal also located outside
            _distMatrix[from_door][to_door] =
                _edgeCost * from_AP->GetNavLine()->DistTo(goal->GetCentroid());

            // add a penalty for goals outside due to the direct line assumption while computing the
            // distances
            if(_distMatrix[from_door][to_door] > 10.0)
                _distMatrix[from_door][to_door] *= 100;
        }
    }

    // run the floyd warshall algorithm
    FloydWarshall();

    // set the configuration for reaching the outside
    // set the distances to all final APs

    for(const auto& itr : _accessPoints) {
        AccessPoint* from_AP = itr.second;
        int from_door = _map_id_to_index[itr.first];
        if(from_AP->GetFinalGoalOutside())
            continue;

        // maybe put the distance to FLT_MAX
        if(from_AP->IsClosed())
            continue;

        double tmpMinDist = FLT_MAX;
        int tmpFinalGlobalNearestID = from_door;

        for(const auto& itr1 : _accessPoints) {
            AccessPoint* to_AP = itr1.second;
            if(from_AP->GetID() == to_AP->GetID())
                continue;
            if(from_AP->GetFinalExitToOutside())
                continue;
            // if(from_AP->GetFinalGoalOutside()) continue;

            if(to_AP->GetFinalExitToOutside()) {
                int to_door = _map_id_to_index[itr1.first];
                if(from_door == to_door)
                    continue;

                // cout <<" checking final destination: "<< pAccessPoints[j]->GetID()<<endl;
                double dist = _distMatrix[from_door][to_door];
                if(dist < tmpMinDist) {
                    tmpFinalGlobalNearestID = to_door;
                    tmpMinDist = dist;
                }
            }
        }

        // in the case it is the final APs
        if(tmpFinalGlobalNearestID == from_door)
            tmpMinDist = 0.0;

        if(tmpMinDist == FLT_MAX) {
            LOG_ERROR(
                "GlobalRouter: There is no visibility path from [{}] to the outside. You can "
                "solve this by enabling triangulation.",
                from_AP->GetFriendlyName());
            return false;
        }

        // set the distance to the final destination ( OUT )
        from_AP->AddFinalDestination(FINAL_DEST_OUT, tmpMinDist);

        // set the intermediate path to global final destination
        GetPath(from_door, tmpFinalGlobalNearestID);

        if(_tmpPedPath.size() >= 2) {
            from_AP->AddTransitAPsTo(
                FINAL_DEST_OUT, _accessPoints[_map_index_to_id[_tmpPedPath[1]]]);
        } else {
            if((!from_AP->GetFinalExitToOutside()) && (!from_AP->IsClosed())) {
                LOG_ERROR(
                    "GlobalRouter: There is no visibility path from {} to the outside. You can "
                    "solve this by enabling triangulation.",
                    from_AP->GetFriendlyName());
                return false;
            }
        }
        _tmpPedPath.clear();
    }

    // set the configuration to reach the goals specified in the ini file
    // set the distances to alternative destinations

    for(const auto& [id, goal] : _building->GetAllGoals()) {
        int to_door_uid = goal->GetAllWalls()[0].GetUniqueID();

        // thats probably a goal located outside the geometry or not an exit from the geometry
        if(to_door_uid == -1) {
            LOG_ERROR(
                "GlobalRouter: there is something wrong with the final destination [{:d}]", id);
            return false;
        }

        int to_door_matrix_index = _map_id_to_index[to_door_uid];

        for(const auto& itr : _accessPoints) {
            AccessPoint* from_AP = itr.second;
            if(from_AP->GetFinalGoalOutside())
                continue;
            if(from_AP->IsClosed())
                continue;
            int from_door_matrix_index = _map_id_to_index[itr.first];

            // comment this if you want infinite as distance to unreachable destinations
            double dist = _distMatrix[from_door_matrix_index][to_door_matrix_index];
            from_AP->AddFinalDestination(id, dist);

            // set the intermediate path
            // set the intermediate path to global final destination
            GetPath(from_door_matrix_index, to_door_matrix_index);
            if(_tmpPedPath.size() >= 2) {
                from_AP->AddTransitAPsTo(id, _accessPoints[_map_index_to_id[_tmpPedPath[1]]]);
            } else {
                if(((!from_AP->IsClosed()))) {
                    LOG_ERROR(
                        "GlobalRouter: There is no visibility path from [{}] to goal [{:d}]. You "
                        "can solve this by enabling triangulation.",
                        from_AP->GetFriendlyName(),
                        id);
                    return false;
                }
            }
            _tmpPedPath.clear();
        }
    }

    LOG_DEBUG("Done with the Global Router Engine!");
    return true;
}

void GlobalRouter::Reset()
{ // clean all allocated spaces
    if(_distMatrix && _pathsMatrix) {
        for(int p = 0; p < _exitsCnt; ++p) {
            delete[] _distMatrix[p];
            delete[] _pathsMatrix[p];
        }

        delete[] _distMatrix;
        delete[] _pathsMatrix;
    }

    for(auto itr = _accessPoints.begin(); itr != _accessPoints.end(); ++itr) {
        delete itr->second;
    }

    _accessPoints.clear();
    _tmpPedPath.clear();
    _map_id_to_index.clear();
    _map_index_to_id.clear();
    _mapIdToFinalDestination.clear();
}

void GlobalRouter::GetPath(int i, int j)
{
    if(_distMatrix[i][j] == FLT_MAX)
        return;
    if(i != j)
        GetPath(i, _pathsMatrix[i][j]);
    _tmpPedPath.push_back(j);
}

bool GlobalRouter::GetPath(Pedestrian* ped, std::vector<Line*>& path)
{
    std::vector<AccessPoint*> aps_path;

    bool done = false;
    int currentNavLine = ped->GetDestination();
    if(currentNavLine == -1) {
        currentNavLine = GetBestDefaultRandomExit(ped);
    }
    aps_path.push_back(_accessPoints[currentNavLine]);

    int loop_count = 1;
    do {
        const auto& ap = aps_path.back();
        int next_dest = ap->GetNearestTransitAPTO(ped->GetFinalDestination());

        if(next_dest == -1)
            break; // we are done

        auto& next_ap = _accessPoints[next_dest];

        if(next_ap->GetFinalExitToOutside()) {
            done = true;
        }

        if(!IsElementInVector(aps_path, next_ap)) {
            aps_path.push_back(next_ap);
        } else {
            LOG_WARNING("Line is already included in the path.");
        }

        // work arround to detect a potential infinte loop.
        if(loop_count++ > 1000) {
            LOG_ERROR(
                "A path could not be found for pedestrian [{}] going to destination [{:d}]. "
                "Stuck in an infinite loop [{:d}]",
                ped->GetUID(),
                ped->GetFinalDestination(),
                loop_count);
            return false;
        }

    } while(!done);

    for(const auto& aps : aps_path)
        path.push_back(aps->GetNavLine());

    return true;
}

/*
 floyd_warshall()
 after calling this function dist[i][j] will the the minimum distance
 between i and j if it exists (i.e. if there's a path between i and j)
 or 0, otherwise
 */
void GlobalRouter::FloydWarshall()
{
    const int n = _building->GetNumberOfGoals() + _building->GetAllGoals().size();
    for(int k = 0; k < n; k++) {
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                if(_distMatrix[i][k] + _distMatrix[k][j] < _distMatrix[i][j]) {
                    _distMatrix[i][j] = _distMatrix[i][k] + _distMatrix[k][j];
                    _pathsMatrix[i][j] = _pathsMatrix[k][j];
                }
            }
        }
    }
}

int GlobalRouter::FindExit(Pedestrian* ped)
{
    if(!_useMeshForLocalNavigation) {
        std::vector<Line*> path;
        GetPath(ped, path);
        SubRoom* sub = _building->GetSubRoom(ped->GetPos());

        // return the next path which is an exit
        for(const auto& navLine : path) {
            // TODO: only set if the pedestrian is already in the subroom.
            //  cuz all lines are returned
            if(IsCrossing(*navLine, {sub}) || IsTransition(*navLine, {sub})) {
                int nav_id = navLine->GetUniqueID();
                ped->SetDestination(nav_id);
                ped->SetExitLine(navLine);
                return nav_id;
            }
        }
        auto [room_id, subroom_id, _] = _building->GetRoomAndSubRoomIDs(ped->GetPos());
        // something bad happens
        LOG_ERROR(
            "Cannot find a valid destination for ped {} located in room {:d} subroom {:d} going "
            "to destination {:d}",
            ped->GetUID(),
            room_id,
            subroom_id,
            ped->GetFinalDestination());
        return -1;
    }
    // else proceed as usual and return the closest navigation line
    int nextDestination = ped->GetDestination();

    if(nextDestination == -1) {
        return GetBestDefaultRandomExit(ped);

    } else {
        SubRoom* sub = _building->GetSubRoom(ped->GetPos());

        for(const auto& apID : sub->GetAllGoalIDs()) {
            AccessPoint* ap = _accessPoints[apID];
            const Point& pt3 = ped->GetPos();
            double distToExit = ap->GetNavLine()->DistTo(pt3);

            if(distToExit > J_EPS_DIST)
                continue;

            // continue until my target is reached
            if(apID != ped->GetDestination())
                continue;

            // one AP is near actualize destination:
            nextDestination = ap->GetNearestTransitAPTO(ped->GetFinalDestination());

            // if(ped->GetID()==6) {ap->Dump();getc(stdin);}
            if(nextDestination == -1) {
                // we are almost at the exit
                // so keep the last destination
                return ped->GetDestination();
            } else {
                // check that the next destination is in the actual room of the pedestrian
                if(!_accessPoints[nextDestination]->isInRange(sub->GetUID())) {
                    // return the last destination if defined
                    int previousDestination = ped->GetDestination();

                    // we are still somewhere in the initialization phase
                    if(previousDestination == -1) {
                        ped->SetDestination(apID);
                        ped->SetExitLine(_accessPoints[apID]->GetNavLine());
                        return apID;
                    } else { // we are still having a valid destination, don't change
                        return previousDestination;
                    }
                } else { // we have reached the new room
                    ped->SetDestination(nextDestination);
                    ped->SetExitLine(_accessPoints[nextDestination]->GetNavLine());
                    return nextDestination;
                }
            }
        }

        // still have a valid destination, so return it
        return nextDestination;
    }
}

int GlobalRouter::GetBestDefaultRandomExit(Pedestrian* ped)
{
    // get the relevant opened exits
    std::vector<AccessPoint*> relevantAPs;
    GetRelevantRoutesTofinalDestination(ped, relevantAPs);
    // in the case there is only one alternative
    // save some computation
    if(relevantAPs.size() == 1) {
        auto&& ap = (AccessPoint * &&) relevantAPs[0];
        ped->SetDestination(ap->GetID());
        ped->SetExitLine(ap->GetNavLine());
        return ap->GetID();
    }

    int bestAPsID = -1;
    double minDistGlobal = FLT_MAX;
    // double minDistLocal = FLT_MAX;

    // get the opened exits
    SubRoom* sub = _building->GetSubRoom(ped->GetPos());

    for(unsigned int g = 0; g < relevantAPs.size(); g++) {
        AccessPoint* ap = relevantAPs[g];

        if(!ap->isInRange(sub->GetUID()))
            continue;
        // check if that exit is open.
        if(ap->IsClosed())
            continue;

        // the line from the current position to the centre of the nav line.
        //  at least the line in that direction minus EPS
        const Point& posA = ped->GetPos();
        const Point& posB = ap->GetNavLine()->GetCentre();
        const Point& posC = (posB - posA).Normalized() * ((posA - posB).Norm() - J_EPS) + posA;

        // check if visible
        // only if the room is convex
        // otherwise check all rooms at that level
        if(!_building->IsVisible(
               posA, posC, _subroomsAtElevation[sub->GetElevation(sub->GetCentroid())], true)) {
            continue;
        }
        double dist1 = ap->GetDistanceTo(ped->GetFinalDestination());
        double dist2 = ap->DistanceTo(posA.x, posA.y);
        double dist = dist1 + dist2;

        if(dist < minDistGlobal) {
            bestAPsID = ap->GetID();
            minDistGlobal = dist;
            // minDistLocal=dist2;
        }
    }

    if(bestAPsID != -1) {
        ped->SetDestination(bestAPsID);
        ped->SetExitLine(_accessPoints[bestAPsID]->GetNavLine());
        return bestAPsID;
    } else {
        if(_building->GetRoom(ped->GetPos())->GetCaption() != "outside" && relevantAPs.size() > 0) {
            // FIXME: assign the nearest and not only a random one
            //{

            relevantAPs[0]->GetID();
            ped->SetDestination(relevantAPs[0]->GetID());
            ped->SetExitLine(relevantAPs[0]->GetNavLine());
            return relevantAPs[0]->GetID();
        }
        return -1;
    }
}

void GlobalRouter::GetRelevantRoutesTofinalDestination(
    Pedestrian* ped,
    std::vector<AccessPoint*>& relevantAPS)
{
    auto [room, sub] = _building->GetRoomAndSubRoom(ped->GetPos());

    // This is best implemented by closing one door and checking if there is still a path to outside
    // and itereating over the others.
    // It might be time consuming, you many pre compute and cache the results.
    if(sub->GetAllHlines().size() == 0) {
        const std::vector<int>& goals = sub->GetAllGoalIDs();
        // filter to keep only the emergencies exits.

        for(unsigned int g1 = 0; g1 < goals.size(); g1++) {
            AccessPoint* ap = _accessPoints[goals[g1]];
            bool relevant = true;
            for(unsigned int g2 = 0; g2 < goals.size(); g2++) {
                if(goals[g2] == goals[g1])
                    continue; // always skip myself
                if(ap->GetNearestTransitAPTO(ped->GetFinalDestination()) == goals[g2]) {
                    // crossings only
                    relevant = false;
                    break;
                }
            }
            if(relevant) {
                // only if not closed
                if(ap)
                    if(!ap->IsClosed())
                        relevantAPS.push_back(ap);
            }
        }
    }
    // quick fix for extra hlines
    //  it should be safe now to delete the first preceding if block
    else {
        const std::vector<int>& goals = sub->GetAllGoalIDs();
        for(unsigned int g1 = 0; g1 < goals.size(); g1++) {
            AccessPoint* ap = _accessPoints[goals[g1]];

            // check for visibility
            // the line from the current position to the centre of the nav line.
            //  at least the line in that direction minus EPS
            const Point& posA = ped->GetPos();
            const Point& posB = ap->GetNavLine()->GetCentre();
            const Point& posC = (posB - posA).Normalized() * ((posA - posB).Norm() - J_EPS) + posA;

            // check if visible
            if(!_building->IsVisible(
                   posA, posC, _subroomsAtElevation[sub->GetElevation(sub->GetCentroid())], true))
            // if (sub->IsVisible(posA, posC, true) == false)
            {
                continue;
            }

            bool relevant = true;
            for(unsigned int g2 = 0; g2 < goals.size(); g2++) {
                if(goals[g2] == goals[g1])
                    continue; // always skip myself
                if(ap->GetNearestTransitAPTO(ped->GetFinalDestination()) == goals[g2]) {
                    // pointing only to the one i dont see
                    // the line from the current position to the centre of the nav line.
                    //  at least the line in that direction minus EPS
                    AccessPoint* ap2 = _accessPoints[goals[g2]];
                    const Point& posA_ = ped->GetPos();
                    const Point& posB_ = ap2->GetNavLine()->GetCentre();
                    const Point& posC_ =
                        (posB_ - posA_).Normalized() * ((posA_ - posB_).Norm() - J_EPS) + posA_;

                    // it points to a destination that I can see anyway
                    if(_building->IsVisible(
                           posA_,
                           posC_,
                           _subroomsAtElevation[sub->GetElevation(sub->GetCentroid())],
                           true))
                    // if (sub->IsVisible(posA_, posC_, true) == true)
                    {
                        relevant = false;
                    }

                    break;
                }
            }
            if(relevant) {
                if(!ap->IsClosed())
                    relevantAPS.push_back(ap);
            }
        }
    }

    // fallback
    if(relevantAPS.size() == 0) {
        // fixme: this should also never happened. But happen due to previous bugs..
        const std::vector<int>& goals = sub->GetAllGoalIDs();
        for(unsigned int g1 = 0; g1 < goals.size(); g1++) {
            relevantAPS.push_back(_accessPoints[goals[g1]]);
        }
    }
}

void GlobalRouter::TriangulateGeometry()
{
    LOG_INFO("Using the triangulation in the global router");
    for(auto&& itr_room : _building->GetAllRooms()) {
        for(auto&& itr_subroom : itr_room.second->GetAllSubRooms()) {
            auto&& subroom = (std::shared_ptr<SubRoom> &&) itr_subroom.second;
            auto&& room = (std::shared_ptr<Room> &&) itr_room.second;
            auto&& obstacles = (const std::vector<Obstacle*>&&) subroom->GetAllObstacles();
            if(!subroom->IsAccessible())
                continue;

            // Triangulate if obstacle or concave and no hlines ?
            // if(subroom->GetAllHlines().size()==0)
            if((obstacles.size() > 0) || !subroom->IsConvex()) {
                std::vector<p2t::Triangle*> triangles = subroom->GetTriangles();

                for(const auto& tr : triangles) {
                    Point P0 = Point(tr->GetPoint(0)->x, tr->GetPoint(0)->y);
                    Point P1 = Point(tr->GetPoint(1)->x, tr->GetPoint(1)->y);
                    Point P2 = Point(tr->GetPoint(2)->x, tr->GetPoint(2)->y);
                    std::vector<Line> edges;
                    edges.push_back(Line(P0, P1));
                    edges.push_back(Line(P1, P2));
                    edges.push_back(Line(P2, P0));

                    for(const auto& line : edges) {
                        // reduce edge that are too close 50 cm is assumed
                        if(MinDistanceToHlines(line.GetCentre(), *subroom) <
                           _minDistanceBetweenTriangleEdges)
                            continue;

                        if(MinAngle(P0, P1, P2) < _minAngleInTriangles)
                            continue;

                        if((IsWall(line, {subroom.get()}) == false) &&
                           (IsCrossing(line, {subroom.get()}) == false) &&
                           (IsTransition(line, {subroom.get()}) == false) &&
                           (IsHline(line, {subroom.get()}) == false)) {
                            // add as a Hline
                            int id = _building->GetAllHlines().size();
                            Hline* h = new Hline();
                            h->SetID(id);
                            h->SetPoint1(line.GetPoint1());
                            h->SetPoint2(line.GetPoint2());
                            h->SetRoom1(room.get());
                            h->SetSubRoom1(subroom.get());
                            subroom->AddHline(h);
                            _building->AddHline(h);
                        }
                    }
                }
            }
        }
    }
    LOG_INFO("INFO:\tDone...");
}

bool GlobalRouter::IsWall(const Line& line, const std::vector<SubRoom*>& subrooms) const
{
    for(auto&& subroom : subrooms) {
        for(auto&& obst : subroom->GetAllObstacles()) {
            for(auto&& wall : obst->GetAllWalls()) {
                if(line.operator==(wall))
                    return true;
            }
        }
        for(auto&& wall : subroom->GetAllWalls()) {
            if(line.operator==(wall))
                return true;
        }
    }

    return false;
}

bool GlobalRouter::IsCrossing(const Line& line, const std::vector<SubRoom*>& subrooms) const
{
    for(auto&& subroom : subrooms) {
        for(const auto& crossing : subroom->GetAllCrossings()) {
            if(crossing->operator==(line))
                return true;
        }
    }
    return false;
}

bool GlobalRouter::IsTransition(const Line& line, const std::vector<SubRoom*>& subrooms) const
{
    for(auto&& subroom : subrooms) {
        for(const auto& transition : subroom->GetAllTransitions()) {
            if(transition->operator==(line))
                return true;
        }
    }
    return false;
}

bool GlobalRouter::IsHline(const Line& line, const std::vector<SubRoom*>& subrooms) const
{
    for(auto&& subroom : subrooms) {
        for(const auto& hline : subroom->GetAllHlines()) {
            if(hline->operator==(line))
                return true;
        }
    }
    return false;
}

double GlobalRouter::MinDistanceToHlines(const Point& point, const SubRoom& sub)
{
    double minDist = FLT_MAX;
    for(const auto& hline : sub.GetAllHlines()) {
        double dist = hline->DistTo(point);
        if(dist < minDist)
            minDist = dist;
    }
    for(const auto& cross : sub.GetAllCrossings()) {
        double dist = cross->DistTo(point);
        if(dist < minDist)
            minDist = dist;
    }
    for(const auto& trans : sub.GetAllTransitions()) {
        double dist = trans->DistTo(point);
        if(dist < minDist)
            minDist = dist;
    }
    for(const auto& wall : sub.GetAllWalls()) {
        double dist = wall.DistTo(point);
        if(dist < minDist)
            minDist = dist;
    }
    // also to the obstacles
    for(const auto& obst : sub.GetAllObstacles()) {
        for(const auto& wall : obst->GetAllWalls()) {
            double dist = wall.DistTo(point);
            if(dist < minDist)
                minDist = dist;
        }
    }

    return minDist;
}

double GlobalRouter::MinAngle(const Point& p1, const Point& p2, const Point& p3)
{
    double a = (p1 - p2).NormSquare();
    double b = (p1 - p3).NormSquare();
    double c = (p3 - p2).NormSquare();

    double alpha = acos((a + b - c) / (2 * sqrt(a) * sqrt(b)));
    double beta = acos((a + c - b) / (2 * sqrt(a) * sqrt(c)));
    double gamma = acos((c + b - a) / (2 * sqrt(c) * sqrt(b)));

    if(fabs(alpha + beta + gamma - M_PI) < J_EPS) {
        std::vector<double> vec = {alpha, beta, gamma};
        return *std::min_element(vec.begin(), vec.end()) * (180.0 / M_PI);
    } else {
        LOG_ERROR("Error in angle calculation");
        exit(EXIT_FAILURE);
    }

    return 0;
}
