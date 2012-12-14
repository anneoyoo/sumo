/****************************************************************************/
/// @file    TraCIServerAPI_Vehicle.cpp
/// @author  Daniel Krajzewicz
/// @author  Laura Bieker
/// @author  Christoph Sommer
/// @author  Michael Behrisch
/// @author  Bjoern Hendriks
/// @date    07.05.2009
/// @version $Id$
///
// APIs for getting/setting vehicle values via TraCI
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2012 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#ifndef NO_TRACI

#include <microsim/MSNet.h>
#include <microsim/MSInsertionControl.h>
#include <microsim/MSVehicle.h>
#include <microsim/MSLane.h>
#include <microsim/MSEdge.h>
#include <microsim/MSEdgeWeightsStorage.h>
#include <microsim/MSAbstractLaneChangeModel.h>
#include <utils/geom/PositionVector.h>
#include <utils/common/DijkstraRouterTT.h>
#include <utils/common/DijkstraRouterEffort.h>
#include <utils/common/HelpersHBEFA.h>
#include <utils/common/HelpersHarmonoise.h>
#include <utils/common/SUMOVehicleParameter.h>
#include "TraCIConstants.h"
#include "TraCIServerAPI_Simulation.h"
#include "TraCIServerAPI_Vehicle.h"
#include "TraCIServerAPI_VehicleType.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// used namespaces
// ===========================================================================
using namespace traci;


// ===========================================================================
// method definitions
// ===========================================================================
bool
TraCIServerAPI_Vehicle::processGet(TraCIServer& server, tcpip::Storage& inputStorage,
                                   tcpip::Storage& outputStorage) {
    // variable & id
    int variable = inputStorage.readUnsignedByte();
    std::string id = inputStorage.readString();
    // check variable
    if (variable != ID_LIST && variable != VAR_SPEED && variable != VAR_SPEED_WITHOUT_TRACI && variable != VAR_POSITION && variable != VAR_ANGLE
            && variable != VAR_ROAD_ID && variable != VAR_LANE_ID && variable != VAR_LANE_INDEX
            && variable != VAR_TYPE && variable != VAR_ROUTE_ID && variable != VAR_COLOR
            && variable != VAR_LANEPOSITION
            && variable != VAR_CO2EMISSION && variable != VAR_COEMISSION && variable != VAR_HCEMISSION && variable != VAR_PMXEMISSION
            && variable != VAR_NOXEMISSION && variable != VAR_FUELCONSUMPTION && variable != VAR_NOISEEMISSION
            && variable != VAR_PERSON_NUMBER
            && variable != VAR_EDGE_TRAVELTIME && variable != VAR_EDGE_EFFORT
            && variable != VAR_ROUTE_VALID && variable != VAR_EDGES
            && variable != VAR_SIGNALS
            && variable != VAR_LENGTH && variable != VAR_MAXSPEED && variable != VAR_VEHICLECLASS
            && variable != VAR_SPEED_FACTOR && variable != VAR_SPEED_DEVIATION && variable != VAR_EMISSIONCLASS
            && variable != VAR_WIDTH && variable != VAR_MINGAP && variable != VAR_SHAPECLASS
            && variable != VAR_ACCEL && variable != VAR_DECEL && variable != VAR_IMPERFECTION
            && variable != VAR_TAU && variable != VAR_BEST_LANES && variable != DISTANCE_REQUEST
            && variable != ID_COUNT
       ) {
        server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Get Vehicle Variable: unsupported variable specified", outputStorage);
        return false;
    }
    // begin response building
    tcpip::Storage tempMsg;
    //  response-code, variableID, objectID
    tempMsg.writeUnsignedByte(RESPONSE_GET_VEHICLE_VARIABLE);
    tempMsg.writeUnsignedByte(variable);
    tempMsg.writeString(id);
    // process request
    if (variable == ID_LIST || variable == ID_COUNT) {
        std::vector<std::string> ids;
        MSVehicleControl& c = MSNet::getInstance()->getVehicleControl();
        for (MSVehicleControl::constVehIt i = c.loadedVehBegin(); i != c.loadedVehEnd(); ++i) {
            if ((*i).second->isOnRoad()) {
                ids.push_back((*i).first);
            }
        }
        if (variable == ID_LIST) {
            tempMsg.writeUnsignedByte(TYPE_STRINGLIST);
            tempMsg.writeStringList(ids);
        } else {
            tempMsg.writeUnsignedByte(TYPE_INTEGER);
            tempMsg.writeInt((int) ids.size());
        }
    } else {
        SUMOVehicle* sumoVehicle = MSNet::getInstance()->getVehicleControl().getVehicle(id);
        if (sumoVehicle == 0) {
            server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Vehicle '" + id + "' is not known", outputStorage);
            return false;
        }
        MSVehicle* v = dynamic_cast<MSVehicle*>(sumoVehicle);
        if (v == 0) {
            server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE,
                                  RTYPE_ERR, "Vehicle '" + id + "' is not a micro-simulation vehicle", outputStorage);
            return false;
        }
        const bool onRoad = v->isOnRoad();
        switch (variable) {
            case VAR_SPEED:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getSpeed() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_SPEED_WITHOUT_TRACI:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getSpeedWithoutTraciInfluence() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_POSITION:
                tempMsg.writeUnsignedByte(POSITION_2D);
                tempMsg.writeDouble(onRoad ? v->getPosition().x() : INVALID_DOUBLE_VALUE);
                tempMsg.writeDouble(onRoad ? v->getPosition().y() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_ANGLE:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getAngle() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_ROAD_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(onRoad ? v->getLane()->getEdge().getID() : "");
                break;
            case VAR_LANE_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(onRoad ? v->getLane()->getID() : "");
                break;
            case VAR_LANE_INDEX:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                if (onRoad) {
                    const std::vector<MSLane*>& lanes = v->getLane()->getEdge().getLanes();
                    tempMsg.writeInt((int)std::distance(lanes.begin(), std::find(lanes.begin(), lanes.end(), v->getLane())));
                } else {
                    tempMsg.writeInt(INVALID_INT_VALUE);
                }
                break;
            case VAR_TYPE:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getVehicleType().getID());
                break;
            case VAR_ROUTE_ID:
                tempMsg.writeUnsignedByte(TYPE_STRING);
                tempMsg.writeString(v->getRoute().getID());
                break;
            case VAR_COLOR:
                tempMsg.writeUnsignedByte(TYPE_COLOR);
                tempMsg.writeUnsignedByte(static_cast<int>(v->getParameter().color.red() * 255. + 0.5));
                tempMsg.writeUnsignedByte(static_cast<int>(v->getParameter().color.green() * 255. + 0.5));
                tempMsg.writeUnsignedByte(static_cast<int>(v->getParameter().color.blue() * 255. + 0.5));
                tempMsg.writeUnsignedByte(255);
                break;
            case VAR_LANEPOSITION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? v->getPositionOnLane() : INVALID_DOUBLE_VALUE);
                break;
            case VAR_CO2EMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeCO2(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_COEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeCO(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_HCEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeHC(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_PMXEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computePMx(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_NOXEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeNOx(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_FUELCONSUMPTION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHBEFA::computeFuel(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_NOISEEMISSION:
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                tempMsg.writeDouble(onRoad ? HelpersHarmonoise::computeNoise(v->getVehicleType().getEmissionClass(), v->getSpeed(), v->getAcceleration()) : INVALID_DOUBLE_VALUE);
                break;
            case VAR_PERSON_NUMBER:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                tempMsg.writeInt(v->getPersonNumber());
                break;
            case VAR_EDGE_TRAVELTIME: {
                if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Retrieval of travel time requires a compound object.", outputStorage);
                    return false;
                }
                if (inputStorage.readInt() != 2) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Retrieval of travel time requires time, and edge as parameter.", outputStorage);
                    return false;
                }
                // time
                SUMOTime time = 0;
                if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires the referenced time as first parameter.", time)) {
                    return false;
                }
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_GET_VEHICLE_VARIABLE, "Retrieval of travel time requires the referenced edge as second parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // retrieve
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal value;
                if (!v->getWeightsStorage().retrieveExistingTravelTime(edge, 0, time, value)) {
                    tempMsg.writeDouble(INVALID_DOUBLE_VALUE);
                } else {
                    tempMsg.writeDouble(value);
                }

            }
            break;
            case VAR_EDGE_EFFORT: {
                if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Retrieval of travel time requires a compound object.", outputStorage);
                    return false;
                }
                if (inputStorage.readInt() != 2) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Retrieval of travel time requires time, and edge as parameter.", outputStorage);
                    return false;
                }
                // time
                SUMOTime time = 0;
                if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_GET_VEHICLE_VARIABLE, "Retrieval of effort requires the referenced time as first parameter.", time)) {
                    return false;
                }
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_GET_VEHICLE_VARIABLE, "Retrieval of effort requires the referenced edge as second parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // retrieve
                tempMsg.writeUnsignedByte(TYPE_DOUBLE);
                SUMOReal value;
                if (!v->getWeightsStorage().retrieveExistingEffort(edge, 0, time, value)) {
                    tempMsg.writeDouble(INVALID_DOUBLE_VALUE);
                } else {
                    tempMsg.writeDouble(value);
                }

            }
            break;
            case VAR_ROUTE_VALID: {
                std::string msg;
                tempMsg.writeUnsignedByte(TYPE_UBYTE);
                tempMsg.writeUnsignedByte(v->hasValidRoute(msg));
            }
            break;
            case VAR_EDGES: {
                const MSRoute& r = v->getRoute();
                tempMsg.writeUnsignedByte(TYPE_STRINGLIST);
                tempMsg.writeInt(r.size());
                for (MSRouteIterator i = r.begin(); i != r.end(); ++i) {
                    tempMsg.writeString((*i)->getID());
                }
            }
            break;
            case VAR_SIGNALS:
                tempMsg.writeUnsignedByte(TYPE_INTEGER);
                tempMsg.writeInt(v->getSignals());
                break;
            case VAR_BEST_LANES: {
                tempMsg.writeUnsignedByte(TYPE_COMPOUND);
                tcpip::Storage tempContent;
                unsigned int cnt = 0;
                tempContent.writeUnsignedByte(TYPE_INTEGER);
                const std::vector<MSVehicle::LaneQ>& bestLanes = onRoad ? v->getBestLanes() : std::vector<MSVehicle::LaneQ>();
                tempContent.writeInt((int) bestLanes.size());
                ++cnt;
                for (std::vector<MSVehicle::LaneQ>::const_iterator i = bestLanes.begin(); i != bestLanes.end(); ++i) {
                    const MSVehicle::LaneQ& lq = *i;
                    tempContent.writeUnsignedByte(TYPE_STRING);
                    tempContent.writeString(lq.lane->getID());
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_DOUBLE);
                    tempContent.writeDouble(lq.length);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_DOUBLE);
                    tempContent.writeDouble(lq.nextOccupation);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_BYTE);
                    tempContent.writeByte(lq.bestLaneOffset);
                    ++cnt;
                    tempContent.writeUnsignedByte(TYPE_UBYTE);
                    lq.allowsContinuation ? tempContent.writeUnsignedByte(1) : tempContent.writeUnsignedByte(0);
                    ++cnt;
                    std::vector<std::string> bestContIDs;
                    for (std::vector<MSLane*>::const_iterator j = lq.bestContinuations.begin(); j != lq.bestContinuations.end(); ++j) {
                        bestContIDs.push_back((*j)->getID());
                    }
                    tempContent.writeUnsignedByte(TYPE_STRINGLIST);
                    tempContent.writeStringList(bestContIDs);
                    ++cnt;
                }
                tempMsg.writeInt((int) cnt);
                tempMsg.writeStorage(tempContent);
            }
            break;
            case DISTANCE_REQUEST:
                if (!commandDistanceRequest(server, inputStorage, tempMsg, v)) {
                    return false;
                }
                break;
            default:
                TraCIServerAPI_VehicleType::getVariable(variable, v->getVehicleType(), tempMsg);
                break;
        }
    }
    server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_OK, "", outputStorage);
    server.writeResponseWithLength(outputStorage, tempMsg);
    return true;
}


bool
TraCIServerAPI_Vehicle::processSet(TraCIServer& server, tcpip::Storage& inputStorage,
                                   tcpip::Storage& outputStorage) {
    std::string warning = ""; // additional description for response
    // variable
    int variable = inputStorage.readUnsignedByte();
    if (variable != CMD_STOP && variable != CMD_CHANGELANE
            && variable != CMD_SLOWDOWN && variable != CMD_CHANGETARGET
            && variable != VAR_ROUTE_ID && variable != VAR_ROUTE
            && variable != VAR_EDGE_TRAVELTIME && variable != VAR_EDGE_EFFORT
            && variable != CMD_REROUTE_TRAVELTIME && variable != CMD_REROUTE_EFFORT
            && variable != VAR_SIGNALS && variable != VAR_MOVE_TO
            && variable != VAR_LENGTH && variable != VAR_MAXSPEED && variable != VAR_VEHICLECLASS
            && variable != VAR_SPEED_FACTOR && variable != VAR_SPEED_DEVIATION && variable != VAR_EMISSIONCLASS
            && variable != VAR_WIDTH && variable != VAR_MINGAP && variable != VAR_SHAPECLASS
            && variable != VAR_ACCEL && variable != VAR_DECEL && variable != VAR_IMPERFECTION
            && variable != VAR_TAU
            && variable != VAR_SPEED && variable != VAR_SPEEDSETMODE && variable != VAR_COLOR
            && variable != ADD && variable != REMOVE
            && variable != VAR_MOVE_TO_VTD
       ) {
        server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Change Vehicle State: unsupported variable specified", outputStorage);
        return false;
    }
    // id
    std::string id = inputStorage.readString();
    const bool shouldExist = variable != ADD;
    SUMOVehicle* sumoVehicle = MSNet::getInstance()->getVehicleControl().getVehicle(id);
    if (sumoVehicle == 0) {
        if (shouldExist) {
            server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Vehicle '" + id + "' is not known", outputStorage);
            return false;
        }
    }
    MSVehicle* v = dynamic_cast<MSVehicle*>(sumoVehicle);
    if (v == 0 && shouldExist) {
        server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE,
                              RTYPE_ERR, "Vehicle '" + id + "' is not a micro-simulation vehicle", outputStorage);
        return false;
    }
    switch (variable) {
        case CMD_STOP: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Stop needs a compound object description.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 4) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Stop needs a compound object description of four items.", outputStorage);
                return false;
            }
            // read road map position
            std::string roadId;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The first stop parameter must be the edge id given as a string.", roadId)) {
                return false;
            }
            SUMOReal pos = 0;
            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The second stop parameter must be the position along the edge given as a double.", pos)) {
                return false;
            }
            int laneIndex = 0;
            if(!server.readTypeCheckingByte(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The third stop parameter must be the lane index given as a byte.", laneIndex)) {
                return false;
            }
            // waitTime
            SUMOTime waitTime = 0;
            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_GET_VEHICLE_VARIABLE, "The fourth stop parameter must be the waiting time given as an integer.", waitTime)) {
                return false;
            }
            // check
            if (pos < 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Position on lane must not be negative", outputStorage);
                return false;
            }
            // get the actual lane that is referenced by laneIndex
            MSEdge* road = MSEdge::dictionary(roadId);
            if (road == 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Unable to retrieve road with given id", outputStorage);
                return false;
            }
            const std::vector<MSLane*>& allLanes = road->getLanes();
            if ((laneIndex < 0) || laneIndex >= (int)(allLanes.size())) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "No lane existing with such id on the given road", outputStorage);
                return false;
            }
            // Forward command to vehicle
            if (!v->addTraciStop(allLanes[laneIndex], pos, 0, waitTime)) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Vehicle is too close or behind the stop on " + allLanes[laneIndex]->getID(), outputStorage);
                return false;
            }
        }
        break;
        case CMD_CHANGELANE: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Lane change needs a compound object description.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 2) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Lane change needs a compound object description of two items.", outputStorage);
                return false;
            }
            // Lane ID
            int laneIndex = 0;
            if(!server.readTypeCheckingByte(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The first lane change parameter must be the lane index given as a byte.", laneIndex)) {
                return false;
            }
            // stickyTime
            SUMOTime stickyTime = 0;
            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The second lane change parameter must be the duration given as an integer.", stickyTime)) {
                return false;
            }
            if ((laneIndex < 0) || (laneIndex >= (int)(v->getEdge()->getLanes().size()))) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "No lane existing with given id on the current road", outputStorage);
                return false;
            }
            // Forward command to vehicle
            std::vector<std::pair<SUMOTime, unsigned int> > laneTimeLine;
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), laneIndex));
            laneTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + stickyTime, laneIndex));
            v->getInfluencer().setLaneTimeLine(laneTimeLine);
            MSVehicle::ChangeRequest req = v->getInfluencer().checkForLaneChanges(MSNet::getInstance()->getCurrentTimeStep(),
                                           *v->getEdge(), v->getLaneIndex());
            v->getLaneChangeModel().requestLaneChange(req);
        }
        break;
        case CMD_SLOWDOWN: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Slow down needs a compound object description.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 2) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Slow down needs a compound object description of two items.", outputStorage);
                return false;
            }
            SUMOReal newSpeed = 0;
            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The first slow down parameter must be the speed given as a double.", newSpeed)) {
                return false;
            }
            if (newSpeed < 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Speed must not be negative", outputStorage);
                return false;
            }
            SUMOTime duration = 0;
            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The second slow down parameter must be the duration given as an integer.", duration)) {
                return false;
            }
            if (duration < 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid time interval", outputStorage);
                return false;
            }
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), v->getSpeed()));
            speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep() + duration, newSpeed));
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
        }
        break;
        case CMD_CHANGETARGET: {
            std::string edgeID;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Change target requires a string containing the id of the new destination edge as parameter.", edgeID)) {
                return false;
            }
            const MSEdge* destEdge = MSEdge::dictionary(edgeID);
            if (destEdge == 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Can not retrieve road with ID " + edgeID, outputStorage);
                return false;
            }
            // build a new route between the vehicle's current edge and destination edge
            MSEdgeVector newRoute;
            const MSEdge* currentEdge = v->getEdge();
            MSNet::getInstance()->getRouterTT().compute(
                currentEdge, destEdge, (const MSVehicle * const) v, MSNet::getInstance()->getCurrentTimeStep(), newRoute);
            // replace the vehicle's route by the new one
            if (!v->replaceRouteEdges(newRoute)) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Route replacement failed for " + v->getID(), outputStorage);
                return false;
            }
        }
        break;
        case VAR_ROUTE_ID: {
            std::string rid;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The route id must be given as a string.", rid)) {
                return false;
            }
            const MSRoute* r = MSRoute::dictionary(rid);
            if (r == 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "The route '" + rid + "' is not known.", outputStorage);
                return false;
            }
            if (!v->replaceRoute(r)) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Route replacement failed for " + v->getID(), outputStorage);
                return false;
            }
        }
        break;
        case VAR_ROUTE: {
            std::vector<std::string> edgeIDs;
            if(!server.readTypeCheckingStringList(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "A route must be defined as a list of edge ids.", edgeIDs)) {
                return false;
            }
            std::vector<const MSEdge*> edges;
            MSEdge::parseEdgesList(edgeIDs, edges, "<unknown>");
            if (!v->replaceRouteEdges(edges)) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Route replacement failed for " + v->getID(), outputStorage);
                return false;
            }
        }
        break;
        case VAR_EDGE_TRAVELTIME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting travel time requires a compound object.", outputStorage);
                return false;
            }
            int parameterCount = inputStorage.readInt();
            if (parameterCount == 4) {
                // begin time
                SUMOTime begTime = 0, endTime = 0;
                if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the begin time as first parameter.", begTime)) {
                    return false;
                }
                // begin time
                if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the end time as second parameter.", endTime)) {
                    return false;
                }
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the referenced edge as third parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // value
                SUMOReal value = 0;
                if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 4 parameters requires the travel time as double as fourth parameter.", value)) {
                    return false;
                }
                // retrieve
                v->getWeightsStorage().addTravelTime(edge, begTime, endTime, value);
            } else if (parameterCount == 2) {
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 2 parameters requires the referenced edge as first parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // value
                SUMOReal value = 0;
                if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 2 parameters requires the travel time as second parameter.", value)) {
                    return false;
                }
                // retrieve
                while (v->getWeightsStorage().knowsTravelTime(edge)) {
                    v->getWeightsStorage().removeTravelTime(edge);
                }
                v->getWeightsStorage().addTravelTime(edge, 0, SUMOTime_MAX, value);
            } else if (parameterCount == 1) {
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting travel time using 1 parameter requires the referenced edge as first parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // retrieve
                while (v->getWeightsStorage().knowsTravelTime(edge)) {
                    v->getWeightsStorage().removeTravelTime(edge);
                }
            } else {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting travel time requires 1, 2, or 4 parameters.", outputStorage);
                return false;
            }
        }
        break;
        case VAR_EDGE_EFFORT: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting effort requires a compound object.", outputStorage);
                return false;
            }
            int parameterCount = inputStorage.readInt();
            if (parameterCount == 4) {
                // begin time
                SUMOTime begTime = 0, endTime = 0;
                if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the begin time as first parameter.", begTime)) {
                    return false;
                }
                // begin time
                if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the end time as second parameter.", endTime)) {
                    return false;
                }
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the referenced edge as third parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // value
                SUMOReal value = 0;
                if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 4 parameters requires the travel time as fourth parameter.", value)) {
                    return false;
                }
                // retrieve
                v->getWeightsStorage().addEffort(edge, begTime, endTime, value);
            } else if (parameterCount == 2) {
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 2 parameters requires the referenced edge as first parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // value
                SUMOReal value = 0;
                if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 2 parameters requires the travel time as second parameter.", value)) {
                    return false;
                }
                // retrieve
                while (v->getWeightsStorage().knowsEffort(edge)) {
                    v->getWeightsStorage().removeEffort(edge);
                }
                v->getWeightsStorage().addEffort(edge, 0, SUMOTime_MAX, value);
            } else if (parameterCount == 1) {
                // edge
                std::string edgeID;
                if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting effort using 1 parameter requires the referenced edge as first parameter.", edgeID)) {
                    return false;
                }
                MSEdge* edge = MSEdge::dictionary(edgeID);
                if (edge == 0) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Referenced edge '" + edgeID + "' is not known.", outputStorage);
                    return false;
                }
                // retrieve
                while (v->getWeightsStorage().knowsEffort(edge)) {
                    v->getWeightsStorage().removeEffort(edge);
                }
            } else {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting effort requires 1, 2, or 4 parameters.", outputStorage);
                return false;
            }
        }
        break;
        case CMD_REROUTE_TRAVELTIME: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Rerouting requires a compound object.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Rerouting should obtain an empty compound object.", outputStorage);
                return false;
            }
            v->reroute(MSNet::getInstance()->getCurrentTimeStep(), MSNet::getInstance()->getRouterTT());
        }
        break;
        case CMD_REROUTE_EFFORT: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Rerouting requires a compound object.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Rerouting should obtain an empty compound object.", outputStorage);
                return false;
            }
            v->reroute(MSNet::getInstance()->getCurrentTimeStep(), MSNet::getInstance()->getRouterEffort());
        }
        break;
        case VAR_SIGNALS: {
            int signals = 0;
            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting signals requires an integer.", signals)) {
                return false;
            }
            v->switchOffSignal(0x0fffffff);
            v->switchOnSignal(signals);
                          }
            break;
        case VAR_MOVE_TO: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting position requires a compound object.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 2) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting position should obtain the lane id and the position.", outputStorage);
                return false;
            }
            // lane ID
            std::string laneID;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The first parameter for setting a position must be the lane ID given as a string.", laneID)) {
                return false;
            }
            // position on lane
            SUMOReal position = 0;
            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The second parameter for setting a position must be the position given as a double.", position)) {
                return false;
            }
            // process
            MSLane* l = MSLane::dictionary(laneID);
            if (l == 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Unknown lane '" + laneID + "'.", outputStorage);
                return false;
            }
            MSEdge& destinationEdge = l->getEdge();
            if (!v->willPass(&destinationEdge)) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Vehicle '" + laneID + "' may be set onto an edge to pass only.", outputStorage);
                return false;
            }
            v->onRemovalFromNet(MSMoveReminder::NOTIFICATION_TELEPORT);
            v->getLane()->removeVehicle(v);
            while (v->getEdge() != &destinationEdge) {
                const MSEdge* nextEdge = v->succEdge(1);
                // let the vehicle move to the next edge
                if (v->enterLaneAtMove(nextEdge->getLanes()[0], true)) {
                    MSNet::getInstance()->getVehicleControl().scheduleVehicleRemoval(v);
                    continue;
                }
            }
            l->forceVehicleInsertion(v, position);
        }
        break;
        case VAR_SPEED: {
            SUMOReal speed = 0;
            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting speed requires a double.", speed)) {
                return false;
            }
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            if (speed >= 0) {
                speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), speed));
                speedTimeLine.push_back(std::make_pair(SUMOTime_MAX, speed));
            }
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
        }
        break;
        case VAR_SPEEDSETMODE: {
            int speedMode = 0;
            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Setting speed mode requires an integer.", speedMode)) {
                return false;
            }
            v->getInfluencer().setConsiderSafeVelocity((speedMode & 1) != 0);
            v->getInfluencer().setConsiderMaxAcceleration((speedMode & 2) != 0);
            v->getInfluencer().setConsiderMaxDeceleration((speedMode & 4) != 0);
        }
        break;
        case VAR_COLOR: {
            RGBColor col;
            if(!server.readTypeCheckingColor(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The color must be given using the according type.", col)) {
                return false;
            }
            v->getParameter().color.set(col.red(), col.green(), col.blue());
            v->getParameter().setParameter |= VEHPARS_COLOR_SET;
        }
        break;
        case ADD: {
            if (v != 0) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "The vehicle " + id + " to add already exists.", outputStorage);
                return false;
            }
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Adding a vehicle requires a compound object.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 6) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Adding a vehicle needs six parameters.", outputStorage);
                return false;
            }
            SUMOVehicleParameter vehicleParams;
            vehicleParams.id = id;

            std::string vTypeID;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "First parameter (type) requires a string.", vTypeID)) {
                return false;
            }
            MSVehicleType* vehicleType = MSNet::getInstance()->getVehicleControl().getVType(vTypeID);
            if (!vehicleType) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid type '" + vTypeID + "' for vehicle '" + id + "'");
                return false;
            }

            std::string routeID;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Second parameter (route) requires a string.", routeID)) {
                return false;
            }
            const MSRoute* route = MSRoute::dictionary(routeID);
            if (!route) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid route '" + routeID + "' for vehicle: '" + id + "'");
                return false;
            }

            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Third parameter (depart) requires an integer.", vehicleParams.depart)) {
                return false;
            }
            if (vehicleParams.depart < 0) {
                const int proc = static_cast<int>(-vehicleParams.depart);
                if (proc >= static_cast<int>(DEPART_DEF_MAX)) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid departure time.", outputStorage);
                    return false;
                }
                vehicleParams.departProcedure = (DepartDefinition)proc;
            }

            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Fourth parameter (position) requires a double.", vehicleParams.departPos)) {
                return false;
            }
            if (vehicleParams.departPos < 0) {
                const int proc = static_cast<int>(-vehicleParams.departPos);
                if (proc >= static_cast<int>(DEPART_POS_DEF_MAX)) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid departure position.", outputStorage);
                    return false;
                }
                vehicleParams.departPosProcedure = (DepartPosDefinition)proc;
            } else {
                vehicleParams.departPosProcedure = DEPART_POS_GIVEN;
            }

            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Fifth parameter (speed) requires a double.", vehicleParams.departSpeed)) {
                return false;
            }
            if (vehicleParams.departSpeed < 0) {
                const int proc = static_cast<int>(-vehicleParams.departSpeed);
                if (proc >= static_cast<int>(DEPART_SPEED_DEF_MAX)) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid departure speed.", outputStorage);
                    return false;
                }
                vehicleParams.departSpeedProcedure = (DepartSpeedDefinition)proc;
            } else {
                vehicleParams.departSpeedProcedure = DEPART_SPEED_GIVEN;
            }

            if (inputStorage.readUnsignedByte() != TYPE_BYTE) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Sixth parameter (lane) requires a byte.", outputStorage);
                return false;
            }
            vehicleParams.departLane = inputStorage.readByte();
            if (vehicleParams.departLane < 0) {
                const int proc = static_cast<int>(-vehicleParams.departLane);
                if (proc >= static_cast<int>(DEPART_LANE_DEF_MAX)) {
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Invalid departure lane.", outputStorage);
                    return false;
                }
                vehicleParams.departLaneProcedure = (DepartLaneDefinition)proc;
            } else {
                vehicleParams.departLaneProcedure = DEPART_LANE_GIVEN;
            }

            SUMOVehicleParameter* params = new SUMOVehicleParameter();
            *params = vehicleParams;
            try {
                SUMOVehicle* vehicle = MSNet::getInstance()->getVehicleControl().buildVehicle(params, route, vehicleType);
                MSNet::getInstance()->getVehicleControl().addVehicle(vehicleParams.id, vehicle);
                MSNet::getInstance()->getInsertionControl().add(vehicle);
            } catch (ProcessError& e) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, e.what(), outputStorage);
                return false;
            }
        }
        break;
        case REMOVE: {
            int why = 0;
            if(!server.readTypeCheckingByte(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "Removing a vehicle requires a byte.", why)) {
                return false;
            }
            MSMoveReminder::Notification n = MSMoveReminder::NOTIFICATION_ARRIVED;
            switch (why) {
                case REMOVE_TELEPORT:
                    n = MSMoveReminder::NOTIFICATION_TELEPORT;
                    break;
                case REMOVE_PARKING:
                    n = MSMoveReminder::NOTIFICATION_PARKING;
                    break;
                case REMOVE_ARRIVED:
                    n = MSMoveReminder::NOTIFICATION_ARRIVED;
                    break;
                case REMOVE_VAPORIZED:
                    n = MSMoveReminder::NOTIFICATION_VAPORIZED;
                    break;
                case REMOVE_TELEPORT_ARRIVED:
                    n = MSMoveReminder::NOTIFICATION_TELEPORT_ARRIVED;
                    break;
                default:
                    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Unknown removal status.", outputStorage);
                    return false;
            }
            v->onRemovalFromNet(n);
            v->getLane()->removeVehicle(v);
            MSNet::getInstance()->getVehicleControl().scheduleVehicleRemoval(v);
        }
        break;
        case VAR_MOVE_TO_VTD: {
            if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting VTD vehicle requires a compound object.", outputStorage);
                return false;
            }
            if (inputStorage.readInt() != 4) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "Setting VTD vehicle should obtain: edgeID, lane, x, y.", outputStorage);
                return false;
            }
            // edge ID
            std::string edgeID;
            if(!server.readTypeCheckingString(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The first parameter for setting a VTD vehicle must be the edge ID given as a string.", edgeID)) {
                return false;
            }
            // lane index
            int laneNum = 0;
            if(!server.readTypeCheckingInt(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The second parameter for setting a VTD vehicle must be lane given as an int.", laneNum)) {
                return false;
            }
            // x
            SUMOReal x = 0, y = 0;
            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The third parameter for setting a VTD vehicle must be the x-position given as a double.", x)) {
                return false;
            }
            // y
            if(!server.readTypeCheckingDouble(inputStorage, outputStorage, CMD_SET_VEHICLE_VARIABLE, "The fourth parameter for setting a VTD vehicle must be the y-position given as a double.", y)) {
                return false;
            }
            //if (inputStorage.readUnsignedByte() != TYPE_DOUBLE) {
            //    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, "The fifth parameter for setting a VTD vehicle must be the speed given as a double.", outputStorage);
            //    return false;
            //}
            //SUMOReal speed = inputStorage.readDouble();
            // process
            if (!v->isOnRoad()) {
                break;
            }
            std::string origID = edgeID + " " + toString(laneNum);
            if (laneNum < 0) {
                edgeID = '-' + edgeID;
                laneNum = -laneNum;
            }
            //
            Position pos(x, y);
            unsigned int r = 0;
            SUMOReal minDist = 1 << (11);
            MSLane* minDistLane = 0;
            MSLane* nameMatchingLane = 0;
            SUMOReal minDistNameMatchingLane = 1 << (11);
            for (; minDistLane == 0 && r < 10 && nameMatchingLane == 0; ++r) {
                std::set<std::string> into;
                PositionVector shape;
                shape.push_back(pos);
                server.collectObjectsInRange(CMD_GET_EDGE_VARIABLE, shape, 1 << r, into);
                for (std::set<std::string>::const_iterator j = into.begin(); j != into.end(); ++j) {
                    MSEdge* e = MSEdge::dictionary(*j);
                    const std::vector<MSLane*>& lanes = e->getLanes();
                    for (std::vector<MSLane*>::const_iterator k = lanes.begin(); k != lanes.end(); ++k) {
                        MSLane* lane = *k;
                        bool nameMatches = false;
                        SUMOReal dist = lane->getShape().distance(pos);
                        if (lane->knowsParameter("origId")) {
                            if (lane->getParameter("origId", "") == origID) {
                                nameMatches = true;
                                if (dist < minDistNameMatchingLane) {
                                    minDistNameMatchingLane = dist;
                                    nameMatchingLane = lane;
                                }
                            }
                        }
                        if (dist < minDist) {
                            minDist = dist;
                            minDistLane = lane;
                        }
                    }
                }
            }
            MSLane* lane = nameMatchingLane != 0 ? nameMatchingLane : minDistLane;
            if (lane != v->getLane()) {
                MSEdge& destinationEdge = lane->getEdge();
                MSEdge* routePos = &destinationEdge;
                while (routePos->getPurpose() == MSEdge::EDGEFUNCTION_INTERNAL) {
                    routePos = &routePos->getLanes()[0]->getLogicalPredecessorLane()->getEdge();
                }
                r = 0;
                const MSRoute& route = v->getRoute();
                unsigned int c = v->getRoutePosition();
                unsigned int l = (int)route.getEdges().size();
                unsigned int rindex = 0;
                bool found = false;
                while (!found && ((int)(c - r) >= 0 || c + r < l)) {
                    if ((int)(c - r) >= 0 && route[c - r] == routePos) {
                        rindex = c - r;
                        found = true;
                    }
                    if (c + r < l && route[c + r] == routePos) {
                        rindex = c + r;
                        found = true;
                    }
                    ++r;
                }
                if (v->isOnRoad()) {
                    v->getLane()->removeVehicle(v);
                    v->onRemovalFromNet(MSMoveReminder::NOTIFICATION_TELEPORT);
                }
                if (!found) {
                    MSEdgeVector edges;
                    MSLane* firstLane = lane;
                    if (destinationEdge.getPurpose() != MSEdge::EDGEFUNCTION_INTERNAL) {
                        edges.push_back(&destinationEdge);
                    } else {
                        firstLane = lane->getLogicalPredecessorLane();
                        edges.push_back(&firstLane->getEdge());
                    }
                    const MSLinkCont& lc = firstLane->getLinkCont();
                    if (lc.size() != 0 && lc[0]->getLane() != 0) {
                        edges.push_back(&lc[0]->getLane()->getEdge());
                    }
                    v->replaceRouteEdges(edges, true);
                } else {
                    v->resetRoutePosition(rindex);
                }
            } else {
                v->onRemovalFromNet(MSMoveReminder::NOTIFICATION_TELEPORT);
                v->getLane()->removeVehicle(v);
            }
            /*
            std::vector<std::pair<SUMOTime, SUMOReal> > speedTimeLine;
            if (speed >= 0) {
                speedTimeLine.push_back(std::make_pair(MSNet::getInstance()->getCurrentTimeStep(), speed));
                speedTimeLine.push_back(std::make_pair(SUMOTime_MAX, speed));
            }
            v->getInfluencer().setSpeedTimeLine(speedTimeLine);
            */

            const SUMOReal position = lane->interpolateGeometryPosToLanePos(
                                          lane->getShape().nearest_position_on_line_to_point2D(pos, false));
            lane->forceVehicleInsertion(v, position);
            //v->getInfluencer().setPosition(position);
            v->getBestLanes(true, lane);
        }
        break;
        default:
            try {
                if (!TraCIServerAPI_VehicleType::setVariable(CMD_SET_VEHICLE_VARIABLE, variable, getSingularType(v), server, inputStorage, outputStorage)) {
                    return false;
                }
            } catch (ProcessError& e) {
                server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_ERR, e.what(), outputStorage);
                return false;
            }
            break;
    }
    server.writeStatusCmd(CMD_SET_VEHICLE_VARIABLE, RTYPE_OK, warning, outputStorage);
    return true;
}


bool
TraCIServerAPI_Vehicle::commandDistanceRequest(traci::TraCIServer& server, tcpip::Storage& inputStorage,
        tcpip::Storage& outputStorage, const MSVehicle* v) {
    if (inputStorage.readUnsignedByte() != TYPE_COMPOUND) {
        server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Retrieval of distance requires a compound object.", outputStorage);
        return false;
    }
    if (inputStorage.readInt() != 2) {
        server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Retrieval of distance requires position and distance type as parameter.", outputStorage);
        return false;
    }

    Position pos;
    std::pair<const MSLane*, SUMOReal> roadPos;

    // read position
    int posType = inputStorage.readUnsignedByte();
    switch (posType) {
        case POSITION_ROADMAP:
            try {
                std::string roadID = inputStorage.readString();
                roadPos.second = inputStorage.readDouble();
                roadPos.first = TraCIServerAPI_Simulation::getLaneChecking(roadID, inputStorage.readUnsignedByte(), roadPos.second);
                pos = roadPos.first->getShape().positionAtLengthPosition(roadPos.second);
            } catch (TraCIException& e) {
                server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, e.what());
                return false;
            }
            break;
        case POSITION_2D:
        case POSITION_3D: {
            const double p1x = inputStorage.readDouble();
            const double p1y = inputStorage.readDouble();
            pos.set(p1x, p1y);
        }
        if (posType == POSITION_3D) {
            inputStorage.readDouble();		// z value is ignored
        }
        roadPos = TraCIServerAPI_Simulation::convertCartesianToRoadMap(pos);
        break;
        default:
            server.writeStatusCmd(CMD_GET_VEHICLE_VARIABLE, RTYPE_ERR, "Unknown position format used for distance request");
            return false;
    }

    // read distance type
    int distType = inputStorage.readUnsignedByte();

    SUMOReal distance = INVALID_DOUBLE_VALUE;
    if (v->isOnRoad()) {
        if (distType == REQUEST_DRIVINGDIST) {
            distance = v->getRoute().getDistanceBetween(v->getPositionOnLane(), roadPos.second,
                       v->getEdge(), &roadPos.first->getEdge());
            if (distance == std::numeric_limits<SUMOReal>::max()) {
                distance = INVALID_DOUBLE_VALUE;
            }
        } else {
            // compute air distance (default)
            distance = v->getPosition().distanceTo(pos);
        }
    }
    // write response command
    outputStorage.writeUnsignedByte(TYPE_DOUBLE);
    outputStorage.writeDouble(distance);
    return true;
}


// ------ helper functions ------
bool
TraCIServerAPI_Vehicle::getPosition(const std::string& id, Position& p) {
    MSVehicle* v = dynamic_cast<MSVehicle*>(MSNet::getInstance()->getVehicleControl().getVehicle(id));
    if (v == 0) {
        return false;
    }
    p = v->getPosition();
    return true;
}


MSVehicleType&
TraCIServerAPI_Vehicle::getSingularType(SUMOVehicle* const veh) {
    const MSVehicleType& oType = veh->getVehicleType();
    std::string newID = oType.getID().find('@') == std::string::npos ? oType.getID() + "@" + veh->getID() : oType.getID();
    MSVehicleType* type = MSVehicleType::build(newID, &oType);
    static_cast<MSVehicle*>(veh)->replaceVehicleType(type);
    return *type;
}


#endif


/****************************************************************************/

