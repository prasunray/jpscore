/**
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
//
// Created by laemmel on 30.03.16.
//
#include "GeoFileParser.hpp"

#include "general/Filesystem.hpp"
#include "geometry/GeometryReader.hpp"
#include "geometry/SubRoom.hpp"
#include "geometry/SubroomType.hpp"
#include "geometry/WaitingArea.hpp"
#include "geometry/Wall.hpp"

#include <Logger.hpp>
#include <stdexcept>
#include <tinyxml.h>

GeoFileParser::GeoFileParser(Configuration* configuration) : _configuration(configuration)
{
}

void GeoFileParser::LoadBuilding(Building* building)
{
    // TODO what happens if any of these  methods failed (return false)? throw exception ?
    if(!LoadGeometry(building)) {
        LOG_ERROR("Could not load the geometry!");
        exit(EXIT_FAILURE);
    }

    if(!LoadRoutingInfo(building)) {
        LOG_ERROR("Could not load extra routing information!");
        exit(EXIT_FAILURE);
    }
    if(!LoadTrafficInfo(building)) {
        LOG_ERROR("Could not load extra traffic information!");
        exit(EXIT_FAILURE);
    }
}

bool GeoFileParser::LoadGeometry(Building* building)
{
    const fs::path& rootDir(_configuration->projectRootDir);

    const fs::path geoFilenameWithPath = rootDir / _configuration->geometryFile;
    LOG_INFO("LoadGeometry: file: {}", geoFilenameWithPath.string());

    TiXmlDocument docGeo(geoFilenameWithPath.string());
    if(!docGeo.LoadFile()) {
        LOG_ERROR("Could not parse the geometry file.");
        LOG_ERROR("{}", docGeo.ErrorDesc());
        return false;
    }

    TiXmlElement* xRootNode = docGeo.RootElement();
    if(!xRootNode) {
        LOG_ERROR("Root element does not exist");
        return false;
    }

    if(xRootNode->ValueStr() != "geometry") {
        LOG_ERROR("Root element value is not 'geometry'.");
        return false;
    }
    if(xRootNode->Attribute("unit"))
        if(std::string(xRootNode->Attribute("unit")) != "m") {
            LOG_ERROR("Only the unit m (meters) is supported.");
            LOG_ERROR("You supplied [{}]", xRootNode->Attribute("unit"));
            return false;
        }

    double version = xmltof(xRootNode->Attribute("version"), -1);

    if(version < std::stod(JPS_OLD_VERSION)) {
        LOG_ERROR("Wrong geometry version!");
        LOG_ERROR("Only version >= {:s} supported.", JPS_OLD_VERSION);
        LOG_ERROR("Please update the version of your geometry file to {:s}", JPS_OLD_VERSION);
        return false;
    }

    // processing the rooms node
    TiXmlNode* xRoomsNode = xRootNode->FirstChild("rooms");
    if(!xRoomsNode) {
        LOG_ERROR("The geometry should have at least one room and one subroom");
        return false;
    }

    for(TiXmlElement* xRoom = xRoomsNode->FirstChildElement("room"); xRoom;
        xRoom = xRoom->NextSiblingElement("room")) {
        Room* room = new Room();

        std::string room_id = xmltoa(xRoom->Attribute("id"), "-1");
        room->SetID(xmltoi(room_id.c_str(), -1));

        std::string caption = "room " + room_id;
        room->SetCaption(xmltoa(xRoom->Attribute("caption"), caption.c_str()));

        double position = xmltof(xRoom->Attribute("zpos"), 0.0);

        room->SetZPos(position);

        for(TiXmlElement* xSubRoom = xRoom->FirstChildElement("subroom"); xSubRoom;
            xSubRoom = xSubRoom->NextSiblingElement("subroom")) {
            int subroom_id = xmltoi(xSubRoom->Attribute("id"), -1);

            std::string SubroomClosed = xmltoa(xSubRoom->Attribute("closed"), "0");
            std::string type = xmltoa(xSubRoom->Attribute("class"), "subroom");

            // get the equation of the plane if any
            double A_x = xmltof(xSubRoom->Attribute("A_x"), 0.0);
            double B_y = xmltof(xSubRoom->Attribute("B_y"), 0.0);

            // assume either the old "C_z" or the new "C"
            double C_z = xmltof(xSubRoom->Attribute("C_z"), 0.0);
            C_z = xmltof(xSubRoom->Attribute("C"), C_z);

            SubRoom* subroom = nullptr;

            if(type == "stair" || type == "escalator" || type == "idle_escalator") {
                if(xSubRoom->FirstChildElement("up") == nullptr) {
                    LOG_ERROR("The attribute <up> and <down> are missing for the {:s}", type);
                    LOG_ERROR("Check your geometry file");
                    return false;
                }
                if(type == "escalator" || type == "idle_escalator")
                    LOG_WARNING(
                        "class: escalator and idle_escalator are deprecated. use escalator_up "
                        "or escalator_down and specify the speed of the escalator");
                double up_x = xmltof(xSubRoom->FirstChildElement("up")->Attribute("px"), 0.0);
                double up_y = xmltof(xSubRoom->FirstChildElement("up")->Attribute("py"), 0.0);
                double down_x = xmltof(xSubRoom->FirstChildElement("down")->Attribute("py"), 0.0);
                double down_y = xmltof(xSubRoom->FirstChildElement("down")->Attribute("py"), 0.0);
                subroom = new Stair();
                ((Stair*) subroom)->SetUp(Point(up_x, up_y));
                ((Stair*) subroom)->SetDown(Point(down_x, down_y));
            } else if(type == "escalator_up") {
                if(xSubRoom->FirstChildElement("up") == nullptr ||
                   xSubRoom->FirstChildElement("down") == nullptr) {
                    LOG_ERROR("The attribute <up> and/or <down> are missing for the {:s}", type);
                    LOG_ERROR("Check your geometry file");
                    return false;
                }
                double speed_up = xmltof(xSubRoom->Attribute("speed"), 0.0);
                double up_x = xmltof(xSubRoom->FirstChildElement("up")->Attribute("px"), 0.0);
                double up_y = xmltof(xSubRoom->FirstChildElement("up")->Attribute("py"), 0.0);
                double down_x = xmltof(xSubRoom->FirstChildElement("down")->Attribute("py"), 0.0);
                double down_y = xmltof(xSubRoom->FirstChildElement("down")->Attribute("py"), 0.0);
                subroom = new Escalator();
                ((Escalator*) subroom)->SetUp(Point(up_x, up_y));
                ((Escalator*) subroom)->SetDown(Point(down_x, down_y));
                ((Escalator*) subroom)->SetEscalatorUp();
                ((Escalator*) subroom)->SetEscalatorSpeed(speed_up);
                _configuration->hasDirectionalEscalators = true;
            } else if(type == "escalator_down") {
                if(xSubRoom->FirstChildElement("up") == nullptr ||
                   xSubRoom->FirstChildElement("down") == nullptr) {
                    LOG_ERROR("The attribute <up> and/or <down> are missing for the {:s}", type);
                    LOG_ERROR("Check your geometry file");
                    return false;
                }
                double speed_down = xmltof(xSubRoom->Attribute("speed"), 0.0);
                double up_x = xmltof(xSubRoom->FirstChildElement("up")->Attribute("px"), 0.0);
                double up_y = xmltof(xSubRoom->FirstChildElement("up")->Attribute("py"), 0.0);
                double down_x = xmltof(xSubRoom->FirstChildElement("down")->Attribute("py"), 0.0);
                double down_y = xmltof(xSubRoom->FirstChildElement("down")->Attribute("py"), 0.0);
                subroom = new Escalator();
                ((Escalator*) subroom)->SetUp(Point(up_x, up_y));
                ((Escalator*) subroom)->SetDown(Point(down_x, down_y));
                ((Escalator*) subroom)->SetEscalatorDown();
                ((Escalator*) subroom)->SetEscalatorSpeed(speed_down);
                _configuration->hasDirectionalEscalators = true;
            } else {
                // normal subroom or corridor
                subroom = new NormalSubRoom();
            }
            subroom->SetType(from_string<SubroomType>(type));
            subroom->SetPlanEquation(A_x, B_y, C_z);
            subroom->SetRoomID(room->GetID());
            subroom->SetSubRoomID(subroom_id);

            // looking for polygons (walls)
            for(TiXmlElement* xPolyVertices = xSubRoom->FirstChildElement("polygon"); xPolyVertices;
                xPolyVertices = xPolyVertices->NextSiblingElement("polygon")) {
                std::string wall_type = xmltoa(xPolyVertices->Attribute("type"), "wall");
                for(TiXmlElement* xVertex = xPolyVertices->FirstChildElement("vertex");
                    xVertex && xVertex != xPolyVertices->LastChild("vertex");
                    xVertex = xVertex->NextSiblingElement("vertex")) {
                    double x1 = xmltof(xVertex->Attribute("px"));
                    double y1 = xmltof(xVertex->Attribute("py"));
                    Point p1{x1, y1};

                    double x2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("px"));
                    double y2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("py"));
                    Point p2{x2, y2};

                    Wall wall{p1, p2, wall_type};
                    subroom->AddWall(wall);

                    if(wall_type == "track") {
                        int trackID = xmltoi(
                            xPolyVertices->Attribute("track_id"), std::numeric_limits<int>::min());
                        if(trackID < 0) {
                            LOG_WARNING(
                                "Track ID should be non-negative integer value but is {}. This "
                                "track will be ignored.",
                                trackID);
                            continue;
                        }
                        building->AddTrackWall(trackID, room->GetID(), subroom_id, wall);

                        std::string input;
                        if(const char* attribute = xVertex->Attribute("start"); attribute) {
                            input = attribute;
                            if(input == "true") {
                                building->AddTrackStart(trackID, p1);
                            }
                        } else if(const char* attribute =
                                      xVertex->NextSiblingElement("vertex")->Attribute("start");
                                  attribute) {
                            input = attribute;
                            if(input == "true") {
                                building->AddTrackStart(trackID, p2);
                            }
                        }
                    }
                }
            }

            // looking for obstacles
            for(TiXmlElement* xObstacle = xSubRoom->FirstChildElement("obstacle"); xObstacle;
                xObstacle = xObstacle->NextSiblingElement("obstacle")) {
                int id = xmltoi(xObstacle->Attribute("id"), -1);
                double height = xmltof(xObstacle->Attribute("height"), 0);
                std::string ObstCaption = xmltoa(xObstacle->Attribute("caption"), "-1");

                Obstacle* obstacle = new Obstacle();
                obstacle->SetId(id);
                obstacle->SetCaption(ObstCaption);
                obstacle->SetHeight(height);

                // looking for polygons (walls)
                for(TiXmlElement* xPolyVertices = xObstacle->FirstChildElement("polygon");
                    xPolyVertices;
                    xPolyVertices = xPolyVertices->NextSiblingElement("polygon")) {
                    for(TiXmlElement* xVertex = xPolyVertices->FirstChildElement("vertex");
                        xVertex && xVertex != xPolyVertices->LastChild("vertex");
                        xVertex = xVertex->NextSiblingElement("vertex")) {
                        double x1 = xmltof(xVertex->Attribute("px"));
                        double y1 = xmltof(xVertex->Attribute("py"));
                        double x2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("px"));
                        double y2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("py"));
                        obstacle->AddWall(Wall(Point(x1, y1), Point(x2, y2)));
                    }
                }
                subroom->AddObstacle(obstacle);
            }
            room->AddSubRoom(subroom);
        }
        // parsing the crossings
        TiXmlNode* xCrossingsNode = xRoom->FirstChild("crossings");
        if(xCrossingsNode)
            for(TiXmlElement* xCrossing = xCrossingsNode->FirstChildElement("crossing"); xCrossing;
                xCrossing = xCrossing->NextSiblingElement("crossing")) {
                int id = xmltoi(xCrossing->Attribute("id"), -1);
                int sub1_id = xmltoi(xCrossing->Attribute("subroom1_id"), -1);
                int sub2_id = xmltoi(xCrossing->Attribute("subroom2_id"), -1);

                double x1 = xmltof(xCrossing->FirstChildElement("vertex")->Attribute("px"));
                double y1 = xmltof(xCrossing->FirstChildElement("vertex")->Attribute("py"));
                double x2 = xmltof(xCrossing->LastChild("vertex")->ToElement()->Attribute("px"));
                double y2 = xmltof(xCrossing->LastChild("vertex")->ToElement()->Attribute("py"));

                Crossing* c = new Crossing();
                c->SetID(id);
                c->SetPoint1(Point(x1, y1));
                c->SetPoint2(Point(x2, y2));

                c->SetSubRoom1(room->GetSubRoom(sub1_id));
                c->SetSubRoom2(room->GetSubRoom(sub2_id));
                c->SetRoom1(room);
                building->AddCrossing(c);

                room->GetSubRoom(sub1_id)->AddCrossing(c);
                room->GetSubRoom(sub2_id)->AddCrossing(c);
            }

        building->AddRoom(room);
    }

    // all rooms are read, now proceed with transitions
    TiXmlNode* xTransNode = xRootNode->FirstChild("transitions");
    if(xTransNode) {
        for(TiXmlElement* xTrans = xTransNode->FirstChildElement("transition"); xTrans;
            xTrans = xTrans->NextSiblingElement("transition")) {
            Transition* t = parseTransitionNode(xTrans, building);
            building->AddTransition(t);
        }
        TiXmlNode* xNodeFile = xTransNode->FirstChild("file");
        if(xNodeFile) {
            fs::path p(_configuration->projectRootDir);
            std::string transFilename = xNodeFile->FirstChild()->ValueStr();
            p /= transFilename;
            transFilename = p.string();

            LOG_INFO("Parsing transition from file <{:s}>", transFilename);
            TiXmlDocument docTrans(transFilename);
            if(!docTrans.LoadFile()) {
                LOG_ERROR("Could not parse the transitions file.");
                LOG_ERROR("{:s}", docTrans.ErrorDesc());
                return false;
            }
            TiXmlElement* xRootNodeTrans = docTrans.RootElement();
            if(!xRootNodeTrans) {
                LOG_ERROR("Root element does not exist in transitions file.");
                return false;
            }
            if(xRootNodeTrans->ValueStr() != "JPScore") {
                LOG_ERROR("Parsing transitions file. Root element value is not 'JPScore'.");
                return false;
            }
            TiXmlNode* xTransNodeFile = xRootNodeTrans->FirstChild("transitions");
            if(!xTransNodeFile) {
                LOG_ERROR("No Transitions in file found.");
                return false;
            }
            for(TiXmlElement* xTrans = xTransNodeFile->FirstChildElement("transition"); xTrans;
                xTrans = xTrans->NextSiblingElement("transition")) {
                Transition* t = parseTransitionNode(xTrans, building);
                building->AddTransition(t);
            }
        } else {
            LOG_INFO("Not parsing transition from file");
        }
        LOG_INFO("Got {} transitions", building->GetAllTransitions().size());

    } // xTransNode
    LOG_INFO("Loading building file successful.");

    ParseExternalFiles(*xRootNode);
    // everything went fine
    return true;
}

bool GeoFileParser::LoadRoutingInfo(Building* building)
{
    // TODO read schedule

    TiXmlDocument docRouting(_configuration->iniFile.string());
    if(!docRouting.LoadFile()) {
        LOG_ERROR("{:s}", docRouting.ErrorDesc());
        LOG_ERROR("Could not parse the routing file");
        return false;
    }

    TiXmlElement* xRootNode = docRouting.RootElement();
    if(!xRootNode) {
        LOG_ERROR("Root element does not exist");
        return false;
    }

    if(!xRootNode->FirstChild("routing")) {
        return true; // no extra routing information
    }

    // load goals and routes
    TiXmlNode* xGoalsNode = xRootNode->FirstChild("routing")->FirstChild("goals");
    if(xGoalsNode) {
        // ---- parse goals from inifile
        for(TiXmlElement* e = xGoalsNode->FirstChildElement("goal"); e;
            e = e->NextSiblingElement("goal")) {
            Goal* goal = parseGoalNode(e);
            if(goal) {
                building->AddGoal(goal);
            }
        }

        // ---- parse waiting areas from inifile
        for(TiXmlElement* e = xGoalsNode->FirstChildElement("waiting_area"); e;
            e = e->NextSiblingElement("waiting_area")) {
            Goal* goal = parseWaitingAreaNode(e);
            if(goal) {
                building->AddGoal(goal);
            }
        }
        // ---- parse goals/waiting areas from external file
        TiXmlNode* xGoalsNodeFile = xGoalsNode->FirstChild("file");
        if(xGoalsNodeFile) {
            fs::path p(_configuration->projectRootDir);
            std::string goalFilename = xGoalsNodeFile->FirstChild()->ValueStr();
            p /= goalFilename;
            goalFilename = p.string();
            LOG_INFO("Goal file <{:s}> will be parsed.", goalFilename);
            TiXmlDocument docGoal(goalFilename);
            if(!docGoal.LoadFile()) {
                LOG_ERROR("{}", docGoal.ErrorDesc());
                LOG_ERROR("Could not parse the goal file.");
                return false;
            }
            TiXmlElement* xRootNodeGoal = docGoal.RootElement();
            if(!xRootNodeGoal) {
                LOG_ERROR("Root element does not exist in goal file.");
                return false;
            }
            if(xRootNodeGoal->ValueStr() != "JPScore") {
                LOG_ERROR("Parsing goal file. Root element value is not 'JPScore'.");
                return false;
            }
            TiXmlNode* xGoal = xRootNodeGoal->FirstChild("goals");
            if(!xGoal) {
                LOG_ERROR("No Goals in file found.");
                return false;
            }
            for(TiXmlElement* e = xGoal->FirstChildElement("goal"); e;
                e = e->NextSiblingElement("goal")) {
                Goal* goal = parseGoalNode(e);
                if(goal) {
                    building->AddGoal(goal);
                }
            }

            for(TiXmlElement* e = xGoal->FirstChildElement("waiting_area"); e;
                e = e->NextSiblingElement("waiting_area")) {
                Goal* goal = parseWaitingAreaNode(e);
                if(goal) {
                    building->AddGoal(goal);
                }
            }

        } else
            LOG_INFO("Goal file not parsed");
    } // xgoalsNode

    LOG_INFO("Done with loading extra routing information");
    return true;
}

bool GeoFileParser::parseDoorNode(TiXmlElement* xDoor, int id, Building* building)
{
    bool result = false; // this return value is not needed.
                         // maybe in the future it might be...
    LOG_INFO("Parsed Door:");
    LOG_INFO(">> ID: {}", id);
    //------------------ state
    std::string stateStr = xmltoa(xDoor->Attribute("state"), "open");
    DoorState state = from_string<DoorState>(stateStr);

    // store transition in a map and call getTransition/getCrossing
    switch(state) {
        case DoorState::OPEN:
            building->GetTransition(id)->Open(true);
            break;
        case DoorState::CLOSE:
            building->GetTransition(id)->Close(true);
            break;
        case DoorState::TEMP_CLOSE:
            building->GetTransition(id)->TempClose(true);
            break;
    }

    LOG_INFO(">> state: {}", stateStr);

    double outflow = xmltof(xDoor->Attribute("outflow"), -1.0);
    if(outflow >= 0) {
        building->GetTransition(id)->SetOutflowRate(outflow);
        LOG_INFO(">> ouflow: {:.2f}", outflow);
    }
    double DT = xmltof(xDoor->Attribute("dt"), -1.0);
    if(DT >= 0) {
        building->GetTransition(id)->SetDT(DT);
    }
    int DN = xmltof(xDoor->Attribute("dn"), -1.0);
    if(DN >= 0) {
        building->GetTransition(id)->SetDN(DN);
        LOG_INFO(">> dn: {}", DN);
    }

    int mdu = xmltof(xDoor->Attribute("max_agents"), -1);
    if(mdu >= 0) {
        building->GetTransition(id)->SetMaxDoorUsage(mdu);
        LOG_INFO(">> max_agents: {}", mdu);
    }
    result = true;
    return result;
}

bool GeoFileParser::LoadTrafficInfo(Building* building)
{
    LOG_INFO("Loading the traffic info");

    TiXmlDocument doc(_configuration->iniFile.string());
    if(!doc.LoadFile()) {
        LOG_ERROR("{}", doc.ErrorDesc());
        LOG_ERROR("Could not parse the project file");
        return false;
    }

    TiXmlNode* xRootNode = doc.RootElement()->FirstChild("traffic_constraints");
    if(!xRootNode) {
        LOG_WARNING("Could not find any traffic information");
        return true;
    }

    // processing the doors node
    TiXmlNode* xDoorsNode = xRootNode->FirstChild("doors");
    if(xDoorsNode) {
        bool res_parseDoor;
        for(TiXmlElement* xDoor = xDoorsNode->FirstChildElement("door"); xDoor;
            xDoor = xDoor->NextSiblingElement("door")) {
            int id = xmltoi(xDoor->Attribute("trans_id"), -1);
            if(id != -1 && building->GetTransition(id)) {
                res_parseDoor = parseDoorNode(xDoor, id, building);
                if(!res_parseDoor)
                    return false;
            }
        } // for xDoor
    } else
        LOG_INFO("No door info found in inifile.");
    // processing file node
    TiXmlNode* xFileNode = xRootNode->FirstChild("file");
    if(xFileNode) {
        fs::path p(_configuration->projectRootDir);
        std::string trafficFilename = xFileNode->FirstChild()->ValueStr();
        p /= trafficFilename;
        trafficFilename = p.string();
        LOG_INFO("Traffic file found <{}>", trafficFilename);
        TiXmlDocument docTraffic(trafficFilename);
        if(!docTraffic.LoadFile()) {
            LOG_ERROR("{}", docTraffic.ErrorDesc());
            LOG_ERROR("Could not parse the traffic file.");
            return false;
        }
        TiXmlElement* xRootNodeTraffic = docTraffic.RootElement();
        if(!xRootNodeTraffic) {
            LOG_ERROR("Root element does not exist.");
            return false;
        }

        if(xRootNodeTraffic->ValueStr() != "JPScore") {
            LOG_ERROR("Root element value is not 'JPScore'.");
            return false;
        }
        TiXmlNode* xTraffic = xRootNodeTraffic->FirstChild("traffic_constraints");
        if(!xTraffic) {
            LOG_ERROR("No traffic constraints in file found.");
            return false;
        }
        TiXmlNode* xDoorsNodeF = xTraffic->FirstChild("doors");
        if(xDoorsNodeF) {
            bool res_parseDoor;
            for(TiXmlElement* xDoor = xDoorsNodeF->FirstChildElement("door"); xDoor;
                xDoor = xDoor->NextSiblingElement("door")) {
                int id = xmltoi(xDoor->Attribute("trans_id"), -1);
                if(id != -1 && building->GetTransition(id)) {
                    res_parseDoor = parseDoorNode(xDoor, id, building);
                    if(!res_parseDoor)
                        return false;
                }
            } // for xDoor
        } else
            LOG_INFO("No door info found in traffic file");
    } else
        LOG_INFO("No traffic file found.");

    LOG_INFO("Done with loading traffic info file");
    return true;
}

Transition* GeoFileParser::parseTransitionNode(TiXmlElement* xTrans, Building* building)
{
    int id = xmltoi(xTrans->Attribute("id"), -1);
    // string caption = "door " + id;
    std::string caption = "door ";
    caption += std::to_string(id);
    caption = xmltoa(xTrans->Attribute("caption"), caption.c_str());
    int room1_id = xmltoi(xTrans->Attribute("room1_id"), -1);
    int room2_id = xmltoi(xTrans->Attribute("room2_id"), -1);
    int subroom1_id = xmltoi(xTrans->Attribute("subroom1_id"), -1);
    int subroom2_id = xmltoi(xTrans->Attribute("subroom2_id"), -1);
    std::string type = xmltoa(xTrans->Attribute("type"), "normal");

    double x1 = xmltof(xTrans->FirstChildElement("vertex")->Attribute("px"));
    double y1 = xmltof(xTrans->FirstChildElement("vertex")->Attribute("py"));

    double x2 = xmltof(xTrans->LastChild("vertex")->ToElement()->Attribute("px"));
    double y2 = xmltof(xTrans->LastChild("vertex")->ToElement()->Attribute("py"));

    Transition* t = new Transition();
    t->SetID(id);
    t->SetCaption(caption);
    t->SetPoint1(Point(x1, y1));
    t->SetPoint2(Point(x2, y2));
    t->SetType(type);
    //--- danger area
    if(room1_id != -1 && subroom1_id != -1) {
        Room* room = building->GetRoom(room1_id);
        SubRoom* subroom = room->GetSubRoom(subroom1_id);

        room->AddTransitionID(t->GetUniqueID());
        t->SetRoom1(room);
        t->SetSubRoom1(subroom);

        // new implementation
        subroom->AddTransition(t);
    }
    if(room2_id != -1 && subroom2_id != -1) {
        Room* room = building->GetRoom(room2_id);
        SubRoom* subroom = room->GetSubRoom(subroom2_id);
        room->AddTransitionID(t->GetUniqueID());
        t->SetRoom2(room);
        t->SetSubRoom2(subroom);

        // new implementation
        subroom->AddTransition(t);
    }
    return t;
}

Goal* GeoFileParser::parseGoalNode(TiXmlElement* e)
{
    LOG_INFO("Loading goal");
    int id = xmltoi(e->Attribute("id"), -1);
    int isFinal = std::string(e->Attribute("final")) == "true" ? true : false;
    std::string caption = xmltoa(e->Attribute("caption"), "-1");
    int room_id = xmltoi(e->Attribute("room_id"), -1);
    int subroom_id = xmltoi(e->Attribute("subroom_id"), -1);
    LOG_INFO("Goal id: {}", id);
    LOG_INFO("Goal caption: {}", caption);
    LOG_INFO("Goal room_id: {}", room_id);
    LOG_INFO("Goal subroom_id: {}", subroom_id);
    Goal* goal = new Goal();
    goal->SetId(id);
    goal->SetCaption(caption);
    goal->SetIsFinalGoal(isFinal);
    goal->SetRoomID(room_id);
    goal->SetSubRoomID(subroom_id);

    // looking for polygons (walls)
    for(TiXmlElement* xPolyVertices = e->FirstChildElement("polygon"); xPolyVertices;
        xPolyVertices = xPolyVertices->NextSiblingElement("polygon")) {
        for(TiXmlElement* xVertex = xPolyVertices->FirstChildElement("vertex");
            xVertex && xVertex != xPolyVertices->LastChild("vertex");
            xVertex = xVertex->NextSiblingElement("vertex")) {
            double x1 = xmltof(xVertex->Attribute("px"));
            double y1 = xmltof(xVertex->Attribute("py"));
            double x2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("px"));
            double y2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("py"));
            goal->AddWall(Wall(Point(x1, y1), Point(x2, y2)));
        }
    }

    if(!goal->ConvertLineToPoly()) {
        LOG_ERROR("Parsing polygon of goal {}", id);
        return nullptr;
    }
    LOG_INFO("Finished parsing goal {}", id);
    return goal;
}

Goal* GeoFileParser::parseWaitingAreaNode(TiXmlElement* e)
{
    LOG_INFO("Loading Waiting Area");

    WaitingArea* wa = new WaitingArea();

    // Read mandatory values and check for valid values, on fail write error
    // Read id and check for correct value
    if(const char* attribute = e->Attribute("id"); attribute) {
        if(int value = xmltoi(attribute, -1); value > -1 && attribute == std::to_string(value)) {
            wa->SetId(value);
        } else {
            LOG_ERROR(" waiting area id set but not an integer");
            delete wa;
            return nullptr;
        }
    } else {
        LOG_ERROR(" waiting area id required");
        delete wa;
        return nullptr;
    }

    // Read room_id and check for correct value
    if(const char* attribute = e->Attribute("room_id"); attribute) {
        if(int value = xmltoi(attribute, -1); value > -1 && attribute == std::to_string(value)) {
            wa->SetRoomID(value);
        } else {
            LOG_ERROR("waiting area {:d}: room_id set but not an integer", wa->GetId());
            delete wa;
            return nullptr;
        }
    } else {
        LOG_ERROR("waiting area {:d}: room_id required", wa->GetId());
        delete wa;
        return nullptr;
    }

    // Read subroom_id and check for correct value
    if(const char* attribute = e->Attribute("subroom_id"); attribute) {
        if(int value = xmltoi(attribute, -1); value > -1 && attribute == std::to_string(value)) {
            wa->SetSubRoomID(value);
        } else {
            LOG_ERROR("waiting area {:d}: subroom_id set but not an integer", wa->GetId());
            delete wa;
            return nullptr;
        }
    } else {
        LOG_ERROR("waiting area {:d}: subroom_id required", wa->GetId());
        delete wa;
        return nullptr;
    }

    // Read caption and check if correct value
    if(const char* attribute = e->Attribute("caption"); attribute) {
        if(std::string value = xmltoa(attribute, ""); !value.empty()) {
            wa->SetCaption(value);
        } else {
            wa->SetCaption("WA " + std::to_string(wa->GetId()));
        }
    }

    LOG_INFO("Goal id: {:d}", wa->GetId());
    LOG_INFO("Goal caption: {:s}", wa->GetCaption().c_str());
    LOG_INFO("Goal room_id: {:d}", wa->GetRoomID());
    LOG_INFO("Goal subroom_id: {:d}", wa->GetSubRoomID());

    // Read optional values and check for valid values, on fail write error
    // Read min_peds and check if correct value
    if(const char* attribute = e->Attribute("min_peds"); attribute) {
        if(int value = xmltoi(attribute, -1); value > 0 && attribute == std::to_string(value)) {
            wa->SetMinNumPed(value);
        } else {
            LOG_WARNING(
                "waiting area {:d}: input for min_peds should be positive integer.", wa->GetId());
        }
    }

    // Read max_peds and check if correct value
    if(const char* attribute = e->Attribute("max_peds"); attribute) {
        if(int value = xmltoi(attribute, -1); value > 0 && attribute == std::to_string(value)) {
            wa->SetMaxNumPed(value);
        } else {
            LOG_WARNING(
                "waiting area {:d}: input for max_peds should be positive integer.", wa->GetId());
        }
    }

    // Read waiting_time and check if correct value
    if(const char* attribute = e->Attribute("waiting_time"); attribute) {
        if(int value = xmltoi(attribute, -1); value >= 0 && attribute == std::to_string(value)) {
            wa->SetWaitingTime(value);
        } else {
            LOG_WARNING(
                "waiting area {:d}: input for waiting_time should be positive integer.",
                wa->GetId());
        }
    }

    // Read transition_id and check if correct value
    if(const char* attribute = e->Attribute("transition_id"); attribute) {
        if(int value = xmltoi(attribute, -1); value > -1 && attribute == std::to_string(value)) {
            wa->SetTransitionID(value);
        } else {
            LOG_WARNING(
                "waiting area {:d}: input for transition_id should be positive integer.",
                wa->GetId());
        }
    }

    // Read is_open and check if correct value
    if(const char* attribute = e->Attribute("is_open"); attribute) {
        std::string in = xmltoa(attribute, "false");
        std::transform(in.begin(), in.end(), in.begin(), ::tolower);

        if(in == "false") {
            wa->SetOpen(false);
        } else if(in == "true") {
            wa->SetOpen(true);
        } else {
            wa->SetOpen(true);
            LOG_WARNING(
                "waiting area {:d}: input for is_open neither <true> nor <false>. "
                "Default <true> is used.",
                wa->GetId());
        }
    }

    // Read global_timer and check if correct value
    if(const char* attribute = e->Attribute("global_timer"); attribute) {
        std::string in = xmltoa(attribute, "false");
        std::transform(in.begin(), in.end(), in.begin(), ::tolower);

        if(in == "false") {
            wa->SetGlobalTimer(false);
        } else if(in == "true") {
            wa->SetGlobalTimer(true);
        } else {
            wa->SetGlobalTimer(false);
            LOG_WARNING(
                "waiting area {:d}: input for global_timer neither <true> nor <false>. "
                "Default <true> is used.",
                wa->GetId());
        }
    }

    // Additional checks:
    const bool usesMinPedWaitingTime =
        (wa->GetMinNumPed() > 0 && wa->GetMaxNumPed() > 0 && wa->GetWaitingTime() >= 0.);
    const bool usesTransition = (wa->GetTransitionID() > 0);
    const bool usesGlobalTimer = (wa->IsGlobalTimer() && wa->GetWaitingTime() >= 0.);

    if(!usesMinPedWaitingTime && !usesTransition && !usesGlobalTimer) {
        LOG_ERROR(
            "waiting area {:d}: min_peds, max_peds, waiting_time, transition_id not set "
            "properly. Set either (min_peds, max_peds, waiting_time) OR transition_id OR "
            "(waiting_time, global_timer).",
            wa->GetId());
        delete wa;
        return nullptr;
    }

    // If (minPed, maxPed, waitingTime) AND transitionID are set only
    // transitionID is considered
    if(usesMinPedWaitingTime && usesTransition) {
        LOG_WARNING(
            "waiting area {:d}: min_peds, max_peds and waiting_time not considered since "
            "transition_id set.",
            wa->GetId());
    }

    // Read the succeeding goals of waiting area
    std::map<int, double> nextGoals;

    // looking for next_wa
    for(TiXmlElement* nextWa = e->FirstChildElement("next_wa"); nextWa;
        nextWa = nextWa->NextSiblingElement("next_wa")) {
        int nextWaId = xmltoi(nextWa->Attribute("id"), std::numeric_limits<int>::min());
        double nextWaP = xmltof(nextWa->Attribute("p"), std::numeric_limits<double>::min());

        if(nextWaId == std::numeric_limits<int>::min() ||
           nextWaP == std::numeric_limits<double>::min()) {
            LOG_ERROR("waiting area {:d}: check next_wa id or p not set properly.", wa->GetId());
            delete wa;
            return nullptr;
        }

        if(nextWaId < -2) {
            LOG_ERROR(
                "waiting area {:d}: check next_wa id should be positive integer.", wa->GetId());
            delete wa;
            return nullptr;
        }

        if(nextWaP < 0. || nextWaP > 1. + 1e-5) {
            LOG_ERROR("waiting area {:d}: check next_wa p should be in [0, 1].", wa->GetId());
            delete wa;
            return nullptr;
        }
        nextGoals.insert(std::pair<int, double>(nextWaId, nextWaP));
    }

    if(!wa->SetNextGoals(nextGoals)) {
        LOG_ERROR(
            "waiting area {:d}: check probabilities sum of p over all next_wa ids != 1.",
            wa->GetId());
        delete wa;
        return nullptr;
    }

    // Read boundaries of waiting area
    for(TiXmlElement* xPolyVertices = e->FirstChildElement("polygon"); xPolyVertices;
        xPolyVertices = xPolyVertices->NextSiblingElement("polygon")) {
        for(TiXmlElement* xVertex = xPolyVertices->FirstChildElement("vertex");
            xVertex && xVertex != xPolyVertices->LastChild("vertex");
            xVertex = xVertex->NextSiblingElement("vertex")) {
            double x1 = xmltof(xVertex->Attribute("px"));
            double y1 = xmltof(xVertex->Attribute("py"));
            double x2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("px"));
            double y2 = xmltof(xVertex->NextSiblingElement("vertex")->Attribute("py"));
            wa->AddWall(Wall(Point(x1, y1), Point(x2, y2)));
        }
    }

    // Check if boundaries area a valid polygon
    if(!wa->ConvertLineToPoly()) {
        LOG_ERROR("waiting area {:d}: parsing polygon of waiting area.", wa->GetId());
        delete wa;
        return nullptr;
    }

    LOG_INFO("waiting area {:d}: finished parsing waiting area.", wa->GetId());
    return wa;
}

bool GeoFileParser::ParseExternalFiles(const TiXmlNode& mainNode)
{
    // read external transition file
    if(mainNode.FirstChild("transitions") &&
       mainNode.FirstChild("transitions")->FirstChild("file")) {
        fs::path transitionFile =
            _configuration->projectRootDir /
            mainNode.FirstChild("transitions")->FirstChild("file")->FirstChild()->Value();
        _configuration->transitionFile = fs::weakly_canonical(transitionFile);
    }

    return true;
}

GeoFileParser::~GeoFileParser()
{
}

std::unique_ptr<Geometry> ParseGeometryXml(const std::filesystem::path& geometry_file)
{
    GeometryBuilder builder{};
    TiXmlDocument doc(geometry_file.string());
    if(!doc.LoadFile()) {
        throw std::runtime_error(
            fmt::format("Cannot parse {}, error: {}", geometry_file, doc.ErrorDesc()));
    }
    const auto* root = doc.RootElement();
    if(root == nullptr || root->ValueStr() != "geometry") {
        throw std::runtime_error(fmt::format("No root element found in {}", geometry_file));
    }
    const auto* rooms = root->FirstChild("rooms");
    if(rooms == nullptr) {
        throw std::runtime_error(fmt::format("No rooms element found in {}", geometry_file));
    }

    auto parsePolygon = [](auto& e, auto& builder) {
        for(const auto* xPolyVertices = e->FirstChildElement("polygon"); xPolyVertices;
            xPolyVertices = xPolyVertices->NextSiblingElement("polygon")) {
            for(const auto* vertex1 = xPolyVertices->FirstChildElement("vertex");
                vertex1 != nullptr;
                vertex1 = vertex1->NextSiblingElement("vertex")) {
                const auto* vertex2 = vertex1->NextSiblingElement("vertex");
                if(vertex2 == nullptr) {
                    break;
                }
                const double x1 = xmltof(vertex1->Attribute("px"));
                const double y1 = xmltof(vertex1->Attribute("py"));
                const double x2 = xmltof(vertex2->Attribute("px"));
                const double y2 = xmltof(vertex2->Attribute("py"));
                builder.AddLineSegment(x1, y1, x2, y2);
            }
        }
    };

    for(const auto* room = rooms->FirstChild("room"); room != nullptr;
        room = room->NextSiblingElement("room")) {
        for(const auto* subroom = room->FirstChild("subroom"); subroom != nullptr;
            subroom = subroom->NextSiblingElement("subroom")) {
            parsePolygon(subroom, builder);
            for(const auto* obstacle = subroom->FirstChild("obstacle"); obstacle != nullptr;
                obstacle = obstacle->NextSiblingElement("obstacle")) {
                parsePolygon(obstacle, builder);
            }
        }
    }

    auto parseDoor = [](auto& e, auto& builder) {
        const auto* id_attribute = e.Attribute("id");
        if(id_attribute == nullptr) {
            throw std::runtime_error("transition id attribute missing");
        }
        const auto id = xmltoi(id_attribute, -1);
        if(id == -1) {
            throw std::runtime_error("transition id attribute not an integer");
        }
        const auto* vertex1 = e.FirstChildElement("vertex");
        if(vertex1 == nullptr) {
            return;
        }
        const auto* vertex2 = vertex1->NextSiblingElement("vertex");
        if(vertex2 == nullptr) {
            return;
        }
        const double x1 = xmltof(vertex1->Attribute("px"));
        const double y1 = xmltof(vertex1->Attribute("py"));
        const double x2 = xmltof(vertex2->Attribute("px"));
        const double y2 = xmltof(vertex2->Attribute("py"));

        builder.AddDoor(x1, y1, x2, y2, id);
    };

    if(const auto* transitions = root->FirstChild("transitions"); transitions != nullptr) {
        for(const auto* transition = transitions->FirstChildElement("transition");
            transition != nullptr;
            transition = transition->NextSiblingElement("transition")) {
            parseDoor(*transition, builder);
        }
    }

    return std::make_unique<Geometry>(builder.Build());
}
