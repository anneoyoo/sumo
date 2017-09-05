/****************************************************************************/
/// @file    HelpersPHEMlight.cpp
/// @author  Daniel Krajzewicz
/// @author  Michael Behrisch
/// @author  Nikolaus Furian
/// @date    Sat, 20.04.2013
/// @version $Id$
///
// Helper methods for PHEMlight-based emission computation
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2013-2017 DLR (http://www.dlr.de/) and contributors
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

#include <limits>
#include <cmath>
#include <foreign/PHEMlight/cpp/Constants.h>
#include <utils/options/OptionsCont.h>
#include "HelpersPHEMlight.h"


// ===========================================================================
// method definitions
// ===========================================================================
HelpersPHEMlight::HelpersPHEMlight() : PollutantsInterface::Helper("PHEMlight"), myIndex(PHEMLIGHT_BASE) {
    myEmissionClassStrings.insert("zero", myIndex++);
}


SUMOEmissionClass
HelpersPHEMlight::getClassByName(const std::string& eClass, const SUMOVehicleClass vc) {
    if (eClass == "unknown" && !myEmissionClassStrings.hasString("unknown")) {
        myEmissionClassStrings.addAlias("unknown", getClassByName("PKW_G_EU4", vc));
    }
    if (myEmissionClassStrings.hasString(eClass)) {
        return myEmissionClassStrings.get(eClass);
    }
    if (eClass.size() < 6) {
        throw InvalidArgument("Unknown emission class '" + eClass + "'.");
    }
    int index = myIndex++;
    const std::string type = eClass.substr(0, 3);
    if (type == "HDV" || type == "LB_" || type == "RB_" || type == "LSZ" || eClass.find("LKW") != std::string::npos) {
        index |= PollutantsInterface::HEAVY_BIT;
    }
    myEmissionClassStrings.insert(eClass, index);
    std::vector<std::string> phemPath;
    phemPath.push_back(OptionsCont::getOptions().getString("phemlight-path") + "/");
    if (getenv("PHEMLIGHT_PATH") != 0) {
        phemPath.push_back(std::string(getenv("PHEMLIGHT_PATH")) + "/");
    }
    if (getenv("SUMO_HOME") != 0) {
        phemPath.push_back(std::string(getenv("SUMO_HOME")) + "/data/emissions/PHEMlight/");
    }
    myHelper.setCommentPrefix("c");
    myHelper.setPHEMDataV("V4");
    myHelper.setclass(eClass);
    if (!myCEPHandler.GetCEP(phemPath, &myHelper)) {
        myEmissionClassStrings.remove(eClass, index);
        myIndex--;
        throw InvalidArgument("File for PHEM emission class " + eClass + " not found.\n" + myHelper.getErrMsg());
    }
    myCEPs[index] = myCEPHandler.getCEPS().find(myHelper.getgClass())->second;
    std::string eclower = eClass;
    std::transform(eclower.begin(), eclower.end(), eclower.begin(), tolower);
    myEmissionClassStrings.addAlias(eclower, index);
    return index;
}


SUMOEmissionClass
HelpersPHEMlight::getClass(const SUMOEmissionClass base, const std::string& vClass, const std::string& fuel, const std::string& eClass, const double weight) const {
    std::string eClassOffset = "0";
    if (eClass.length() == 5 && eClass.substr(0, 4) == "Euro") {
        if (eClass[4] >= '0' && eClass[4] <= '6') {
            eClassOffset = eClass.substr(4, 1);
        }
    }
    std::string desc;
    if (vClass == "Passenger") {
        desc = "PKW_";
        if (fuel == "Gasoline") {
            desc += "G_";
        } else if (fuel == "Diesel") {
            desc += "D_";
        } else if (fuel == "HybridGasoline") {
            desc = "H_" + desc + "G_";
        } else if (fuel == "HybridDiesel") {
            desc = "H_" + desc + "G_";
        }
        desc += "EU" + eClassOffset;
    } else if (vClass == "Moped") {
        desc = "KKR_G_EU" + eClassOffset;
    } else if (vClass == "Motorcycle") {
        desc = "MR_G_EU" + eClassOffset;
        if (fuel == "Gasoline2S") {
            desc += "_2T";
        } else {
            desc += "_4T";
        }
    } else if (vClass == "Delivery") {
        desc = "LNF_";
        if (fuel == "Gasoline") {
            desc += "G_";
        } else if (fuel == "Diesel") {
            desc += "D_";
        }
        desc += "EU" + eClassOffset + "_I";
        if (weight > 1305.) {
            desc += "I";
            if (weight > 1760.) {
                desc += "I";
            }
        }
    } else if (vClass == "UrbanBus") {
        desc = "LB_D_EU" + eClassOffset;
    } else if (vClass == "Coach") {
        desc = "RB_D_EU" + eClassOffset;
    } else if (vClass == "Truck") {
        desc = "Solo_LKW_D_EU" + eClassOffset + "_I";
        if (weight > 1305.) {
            desc += "I";
        }
    } else if (vClass == "Trailer") {
        desc = "LSZ_D_EU" + eClassOffset;
    }
    if (myEmissionClassStrings.hasString(desc)) {
        return myEmissionClassStrings.get(desc);
    }
    return base;
}


std::string
HelpersPHEMlight::getAmitranVehicleClass(const SUMOEmissionClass c) const {
    const std::string name = myEmissionClassStrings.getString(c);
    if (name.find("KKR_") != std::string::npos) {
        return "Moped";
    } else if (name.find("RB_") != std::string::npos) {
        return "Coach";
    } else if (name.find("LB_") != std::string::npos) {
        return "UrbanBus";
    } else if (name.find("LNF_") != std::string::npos) {
        return "Delivery";
    } else if (name.find("LSZ_") != std::string::npos) {
        return "Trailer";
    } else if (name.find("MR_") != std::string::npos) {
        return "Motorcycle";
    } else if (name.find("LKW_") != std::string::npos) {
        return "Truck";
    }
    return "Passenger";
}


std::string
HelpersPHEMlight::getFuel(const SUMOEmissionClass c) const {
    const std::string name = myEmissionClassStrings.getString(c);
    std::string fuel = "Gasoline";
    if (name.find("_D_") != std::string::npos) {
        fuel = "Diesel";
    }
    if (name.find("H_") != std::string::npos) {
        fuel = "Hybrid" + fuel;
    }
    return fuel;
}


int
HelpersPHEMlight::getEuroClass(const SUMOEmissionClass c) const {
    const std::string name = myEmissionClassStrings.getString(c);
    if (name.find("_EU1") != std::string::npos) {
        return 1;
    } else if (name.find("_EU2") != std::string::npos) {
        return 2;
    } else if (name.find("_EU3") != std::string::npos) {
        return 3;
    } else if (name.find("_EU4") != std::string::npos) {
        return 4;
    } else if (name.find("_EU5") != std::string::npos) {
        return 5;
    } else if (name.find("_EU6") != std::string::npos) {
        return 6;
    }
    return 0;
}


double
HelpersPHEMlight::getWeight(const SUMOEmissionClass c) const {
    const std::string name = myEmissionClassStrings.getString(c);
    if (name.find("LNF_") != std::string::npos) {
        if (name.find("_III") != std::string::npos) {
            return 2630.;
        } else if (name.find("_II") != std::string::npos) {
            return 1532.;
        } else if (name.find("_I") != std::string::npos) {
            return 652.;
        }
    }
    if (name.find("Solo_LKW_") != std::string::npos) {
        if (name.find("_II") != std::string::npos) {
            return 8398.;
        } else if (name.find("_I") != std::string::npos) {
            return 18702.;
        }
    }
    return -1.;
}


double
HelpersPHEMlight::getEmission(PHEMlightdll::CEP* currCep, const std::string& e, const double p, const double v) const {
    return currCep->GetEmission(e, p, v, &myHelper);
}


double
HelpersPHEMlight::compute(const SUMOEmissionClass c, const PollutantsInterface::EmissionType e, const double v, const double a, const double slope, const std::map<int, double>* /* param */) const {
    if (c == PHEMLIGHT_BASE) { // zero emission class
        return 0.;
    }
    PHEMlightdll::CEP* const currCep = myCEPs.count(c) == 0 ? 0 : myCEPs.find(c)->second;
    const double corrSpeed = MAX2((double) 0.0, v);
    const double corrAcc = v == 0.0 ? 0.0 : MIN2(a, currCep->GetMaxAccel(corrSpeed, a, slope));
    const std::string& fuelType = currCep->getFuelType();
    if (fuelType != PHEMlightdll::Constants::strBEV && a < currCep->GetDecelCoast(corrSpeed, corrAcc, slope) && corrSpeed > PHEMlightdll::Constants::ZERO_SPEED_ACCURACY) {
        return 0;
    }
    const double power = currCep->CalcPower(corrSpeed, corrAcc, slope);
    switch (e) {
        case PollutantsInterface::CO:
            return getEmission(currCep, "CO", power, corrSpeed) / 3600. * 1000.;
        case PollutantsInterface::CO2:
            return currCep->GetCO2Emission(getEmission(currCep, "FC", power, corrSpeed),
                                           getEmission(currCep, "CO", power, corrSpeed),
                                           getEmission(currCep, "HC", power, corrSpeed), &myHelper) / 3600. * 1000.;
        case PollutantsInterface::HC:
            return getEmission(currCep, "HC", power, corrSpeed) / 3600. * 1000.;
        case PollutantsInterface::NO_X:
            return getEmission(currCep, "NOx", power, corrSpeed) / 3600. * 1000.;
        case PollutantsInterface::PM_X:
            return getEmission(currCep, "PM", power, corrSpeed) / 3600. * 1000.;
        case PollutantsInterface::FUEL: {
            if (fuelType == PHEMlightdll::Constants::strDiesel) { // divide by average diesel density of 836 g/l
                return getEmission(currCep, "FC", power, corrSpeed) / 836. / 3600. * 1000.;
            } else if (fuelType == PHEMlightdll::Constants::strGasoline) { // divide by average gasoline density of 742 g/l
                return getEmission(currCep, "FC", power, corrSpeed) / 742. / 3600. * 1000.;
            } else if (fuelType == PHEMlightdll::Constants::strBEV) {
                return 0;
            } else {
                return getEmission(currCep, "FC", power, corrSpeed) / 3600. * 1000.; // surely false, but at least not additionally modified
            }
        }
        case PollutantsInterface::ELEC:
            if (fuelType == PHEMlightdll::Constants::strBEV) {
                return getEmission(currCep, "FC", power, corrSpeed) / 3600. * 1000.;
            }
            return 0;
    }
    // should never get here
    return 0.;
}


/****************************************************************************/
