/***************************************************************************
                          NIElmar2NodesHandler.cpp
             A LineHandler-derivate to load nodes form a elmar-nodes-file
                             -------------------
    project              : SUMO
    begin                : Sun, 16 May 2004
    copyright            : (C) 2004 by DLR/IVF http://ivf.dlr.de/
    author               : Daniel Krajzewicz
    email                : Daniel.Krajzewicz@dlr.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
namespace
{
    const char rcsid[] =
    "$Id$";
}
// $Log$
// Revision 1.1  2005/07/14 11:05:28  dkrajzew
// elmar unsplitted import added
//
//
/* =========================================================================
 * compiler pragmas
 * ======================================================================= */
#pragma warning(disable: 4786)


/* =========================================================================
 * included modules
 * ======================================================================= */
#include <string>
#include <utils/importio/LineHandler.h>
#include <utils/common/StringTokenizer.h>
#include <utils/common/MsgHandler.h>
#include <utils/common/UtilExceptions.h>
#include <utils/convert/TplConvert.h>
#include <utils/geom/GeomHelper.h>
#include <netbuild/nodes/NBNode.h>
#include <netbuild/nodes/NBNodeCont.h>
#include "NIElmar2NodesHandler.h"


/* =========================================================================
 * used namespaces
 * ======================================================================= */
using namespace std;

/* =========================================================================
 * method definitions
 * ======================================================================= */
NIElmar2NodesHandler::NIElmar2NodesHandler(NBNodeCont &nc,
                                           const std::string &file,
                                           double centerX, double centerY,
                                           std::map<std::string, Position2DVector> &geoms)
    : FileErrorReporter("elmar-nodes", file),
    myCurrentLine(0), myInitX(centerX), myInitY(centerY),
    myNodeCont(nc), myGeoms(geoms)
{
    myInitX /= 100000.0;
    myInitY /= 100000.0;
}


NIElmar2NodesHandler::~NIElmar2NodesHandler()
{
}


bool
NIElmar2NodesHandler::report(const std::string &result)
{
    // skip previous information
    while(++myCurrentLine<7) {
        return true;
    }
    string id;
    double x, y;
    int no_geoms, intermediate;
    StringTokenizer st(result, StringTokenizer::WHITECHARS);
    // check
    if(st.size()<5) {
        MsgHandler::getErrorInstance()->inform(
            "Something is wrong with the following data line");
        MsgHandler::getErrorInstance()->inform(
            result);
        throw ProcessError();
    }
    // parse
        // id
    id = st.next();
        // intermediate?
    try {
        intermediate = (double) TplConvert<char>::_2int(st.next().c_str());
    } catch (NumberFormatException &) {
        MsgHandler::getErrorInstance()->inform(
            "Non-numerical value for internmediate y/n occured.");
        throw ProcessError();
    }
        // number of geometrical information
    try {
        no_geoms = (double) TplConvert<char>::_2int(st.next().c_str());
    } catch (NumberFormatException &) {
        MsgHandler::getErrorInstance()->inform(
            "Non-numerical value for number of nodes occured.");
        throw ProcessError();
    }
        // geometrical information
    Position2DVector geoms;
    for(int i=0; i<no_geoms; i++) {
        try {
            x = (double) TplConvert<char>::_2float(st.next().c_str());
        } catch (NumberFormatException &) {
            MsgHandler::getErrorInstance()->inform(
                "Non-numerical value for node-x-position occured.");
            throw ProcessError();
        }
        try {
            y = (double) TplConvert<char>::_2float(st.next().c_str());
        } catch (NumberFormatException &) {
            MsgHandler::getErrorInstance()->inform(
                "Non-numerical value for node-y-position occured.");
            throw ProcessError();
        }
        x = x / 100000.0;
        y = y / 100000.0;
        double ys = y;
        x = (x-myInitX);
        y = (y-myInitY);
        double x1 = x * 111.320*1000;
        double y1 = y * 111.136*1000;
        x1 *= cos(ys*PI/180.0);
        geoms.push_back(Position2D(x1, y1));
    }
    // geo->metric
//    x = -1.0 * (x-myInitX)
//        * (double) 111.320 * /*(double) 1000.0 * */cos(y / (double) 10000.0*PI/180.0)
//        / (double) 10.0; // 10000.0
//    y = (y-myInitY)
//        * (double) 111.136 /* * (double) 1000.0*/
//        / (double) 10.0; // 10000.0

//    y1 *= 4.0/2.0;

    if(intermediate==0) {
        NBNode *n = new NBNode(id, geoms.at(0));
        if(!myNodeCont.insert(n)) {
            delete n;
            MsgHandler::getErrorInstance()->inform(
                string("Could not add node '") + id + string("'."));
            throw ProcessError();
        }
    } else {
        myGeoms[id] = geoms;
    }
    return true;
}


/**************** DO NOT DEFINE ANYTHING AFTER THE INCLUDE *****************/

// Local Variables:
// mode:C++
// End:


