/***************************************************************************
                          MSRightOfWayJunction.cpp  -  Usual right-of-way
                          junction.
                             -------------------
    begin                : Wed, 12 Dez 2001
    copyright            : (C) 2001 by Christian Roessel
    email                : roessel@zpr.uni-koeln.de
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
// Revision 1.19  2006/10/06 07:13:40  dkrajzew
// debugging internal lanes
//
// Revision 1.18  2006/10/04 13:18:17  dkrajzew
// debugging internal lanes, multiple vehicle emission and net building
//
// Revision 1.17  2006/09/18 10:07:43  dkrajzew
// patching junction-internal state simulation
//
// Revision 1.16  2006/08/02 11:58:23  dkrajzew
// first try to make junctions tls-aware
//
// Revision 1.15  2006/02/23 11:27:56  dkrajzew
// tls may have now several programs
//
// Revision 1.14  2005/10/07 11:37:45  dkrajzew
// THIRD LARGE CODE RECHECK: patched problems on Linux/Windows configs
//
// Revision 1.13  2005/09/22 13:45:51  dkrajzew
// SECOND LARGE CODE RECHECK: converted doubles and floats to SUMOReal
//
// Revision 1.12  2005/09/15 11:10:46  dkrajzew
// LARGE CODE RECHECK
//
// Revision 1.11  2005/05/04 08:32:05  dkrajzew
// level 3 warnings removed; a certain SUMOTime time description added
//
// Revision 1.10  2004/08/02 12:09:39  dkrajzew
// using Position2D instead of two SUMOReals
//
// Revision 1.9  2003/12/05 14:59:33  dkrajzew
// removed some unused lines
//
// Revision 1.8  2003/12/04 13:30:41  dkrajzew
// work on internal lanes
//
// Revision 1.7  2003/10/31 08:03:38  dkrajzew
// hope to have patched false usage of RAND_MAX when using gcc
//
// Revision 1.6  2003/10/15 11:41:43  dkrajzew
// false usage of rand() patched
//
// Revision 1.5  2003/06/18 11:30:26  dkrajzew
// debug outputs now use a DEBUG_OUT macro instead of cout;
//  this shall ease the search for further couts which must be redirected to
//  the messaaging subsystem
//
// Revision 1.4  2003/05/20 09:31:46  dkrajzew
// emission debugged; movement model reimplemented (seems ok);
//  detector output debugged; setting and retrieval of some parameter added
//
// Revision 1.3  2003/02/07 10:41:50  dkrajzew
// updated
//
// Revision 1.2  2002/10/16 16:42:29  dkrajzew
// complete deletion within destructors implemented; clear-operator added
//  for container; global file include; junction extended by position
//  information (should be revalidated later)
//
// Revision 1.1  2002/10/16 14:48:26  dkrajzew
// ROOT/sumo moved to ROOT/src
//
// Revision 1.3  2002/04/18 10:53:20  croessel
// In findCompetitor we now ignore lanes, that don't have a vehicle that
// is able to leave the lane.
//
// Revision 1.2  2002/04/11 15:25:56  croessel
// Changed SUMOReal to SUMOReal.
//
// Revision 1.1.1.1  2002/04/08 07:21:23  traffic
// new project name
//
// Revision 2.3  2002/03/06 10:56:36  croessel
// Bugfix: myRespond will have always the correct size before being passed
//  to myLogic.
//
// Revision 2.2  2002/02/27 13:47:57  croessel
// Additional assert's because of parameter-passing-problems.
//
// Revision 2.1  2002/02/21 18:49:45  croessel
// Deadlock-killer implemented.
//
// Revision 2.0  2002/02/14 14:43:19  croessel
// Bringing all files to revision 2.0. This is just cosmetics.
//
// Revision 1.5  2002/02/01 15:48:26  croessel
// Changed condition in moveFirstVehicles() again.
//
// Revision 1.4  2002/02/01 14:14:33  croessel
// Changed condition in moveFirstVehicles(). Now vehicles with a
// BrakeRequest only will also be moved.
//
// Revision 1.3  2002/02/01 11:52:28  croessel
// Removed function-adaptor findCompetitor from inside the class to the
// outside to please MSVC++.
//
// Revision 1.2  2002/02/01 11:40:34  croessel
// Changed return-type of some void methods used in for_each-loops to
// bool in order to please MSVC++.
//
// Revision 1.1  2001/12/13 15:54:49  croessel
// Initial commit. Has been MSJunction.cpp before.
//
/* =========================================================================
 * compiler pragmas
 * ======================================================================= */
#pragma warning(disable: 4786)


/* =========================================================================
 * included modules
 * ======================================================================= */
#ifdef HAVE_CONFIG_H
#ifdef WIN32
#include <windows_config.h>
#else
#include <config.h>
#endif
#endif // HAVE_CONFIG_H

#include "MSRightOfWayJunction.h"
#include "MSLane.h"
#include "MSJunctionLogic.h"
#include "MSBitSetLogic.h"
#include "MSGlobals.h"
#include "MSInternalLane.h"
#include <algorithm>
#include <cassert>
#include <cmath>

#ifdef ABS_DEBUG
#include "MSDebugHelper.h"
#endif

#ifdef _DEBUG
#include <utils/dev/debug_new.h>
#endif // _DEBUG


/* =========================================================================
 * used namespaces
 * ======================================================================= */
using namespace std;


/* =========================================================================
 * some definitions (debugging only)
 * ======================================================================= */
#define DEBUG_OUT cout


/* =========================================================================
 * method definitions
 * ======================================================================= */
MSRightOfWayJunction::MSRightOfWayJunction( string id,
                                            const Position2D &position,
                                            LaneCont incoming,
#ifdef HAVE_INTERNAL_LANES
                                            LaneCont internal,
#endif
                                            MSJunctionLogic* logic)
    : MSLogicJunction( id, position, incoming
#ifdef HAVE_INTERNAL_LANES
    , internal ),
#else
    ),
#endif
    myLogic( logic )
{
}


bool
MSRightOfWayJunction::clearRequests()
{
    myRequest.reset();
    myInnerState.reset();
    return true;
}


MSRightOfWayJunction::~MSRightOfWayJunction()
{
}


void
MSRightOfWayJunction::postloadInit()
{
    // inform links where they have to report approaching vehicles to
    size_t requestPos = 0;
    LaneCont::iterator i;
    // going through the incoming lanes...
    for(i=myIncomingLanes.begin(); i!=myIncomingLanes.end(); i++) {
        const MSLinkCont &links = (*i)->getLinkCont();
        // ... set information for every link
        for(MSLinkCont::const_iterator j=links.begin(); j!=links.end(); j++) {
            (*j)->setRequestInformation(&myRequest, requestPos,
                &myRespond, requestPos/*, clearInfo*/);
            requestPos++;
        }
    }
#ifdef HAVE_INTERNAL_LANES
    // set information for the internal lanes
    requestPos = 0;
    for(i=myInternalLanes.begin(); i!=myInternalLanes.end(); i++) {
        // ... set information about participation
        static_cast<MSInternalLane*>(*i)->setParentJunctionInformation(
            &myInnerState, requestPos++);
    }
#endif
}


bool
MSRightOfWayJunction::setAllowed()
{
#ifdef ABS_DEBUG
    if(debug_globaltime>debug_searchedtime&&myID==debug_searchedJunction) {
        DEBUG_OUT << "Request: " << myRequest << endl;
        DEBUG_OUT << "InnerSt: " << myInnerState<< endl;
    }
#endif
    // Get myRespond from logic and check for deadlocks.
    myLogic->respond( myRequest, myInnerState, myRespond );
    deadlockKiller();

#ifdef HAVE_INTERNAL_LANES
    // lets reset the yield information on internal, split
    //  left-moving links
    if(MSGlobals::gUsingInternalLanes) {
        for(LaneCont::iterator i=myInternalLanes.begin(); i!=myInternalLanes.end(); ++i) {
            const MSLinkCont &lc = (*i)->getLinkCont();
            if(lc.size()==1) {
                MSLink *link = lc[0];
                if(link->getViaLane()!=0) {
                    // thisis a split left-mover
                    link->resetInternalPriority();
                }
            }
        }
    }
#endif

#ifdef ABS_DEBUG
    if(debug_globaltime>debug_searchedtime&&myID==debug_searchedJunction) {
        DEBUG_OUT << "Respond: " << myRespond << endl;
    }
#endif
    return true;
}


void
MSRightOfWayJunction::deadlockKiller()
{
    if ( myRequest.none() ) {
        return;
    }

    // let's assume temporary, that deadlocks only occure on right-before-left
    //  junctions
    if ( myRespond.none() && myInnerState.none() ) {
#ifdef ABS_DEBUG
    if(debug_globaltime>debug_searchedtime&&myID==debug_searchedJunction) {
        DEBUG_OUT << "Killing deadlock" << endl;
    }
#endif

        // Handle deadlock: Create randomly a deadlock-free request out of
        // myRequest, i.e. a "single bit" request. Then again, send it
        // through myLogic (this is neccessary because we don't have a
        // mapping between requests and lanes.) !!! (we do now!!)
        vector< unsigned > trueRequests;
        trueRequests.reserve( myRespond.size() );
        for ( unsigned i = 0; i < myRequest.size(); ++i ) {

            if ( myRequest.test(i) ) {

                trueRequests.push_back( i );
                assert( trueRequests.size() <= myRespond.size() );
            }
        }
        // Choose randomly an index out of [0,trueRequests.size()];
        // !!! random choosing may choose one of less priorised lanes
        unsigned noLockIndex = static_cast< unsigned > ( floor (
           static_cast< SUMOReal >( rand() ) /
           (static_cast< SUMOReal >( RAND_MAX ) + 1.0) *
           static_cast< SUMOReal >( trueRequests.size() ) ) );

        // Create deadlock-free request.
        std::bitset<64> noLockRequest(false);
        assert(trueRequests.size()>noLockIndex);
        noLockRequest.set( trueRequests[ noLockIndex ] );
        // Calculate respond with deadlock-free request.
        myLogic->respond( noLockRequest, myInnerState,  myRespond );
    }
    return;
}


bool areRealFoes(MSLink *l1, MSLink *l2)
{
    if(l1->getLane()->getEdge()!=l2->getLane()->getEdge()) {
        return true;
    }
    return false;//l1->getLane()==l2->getLane();
}

void
MSRightOfWayJunction::rebuildPriorities()
{
    MSBitSetLogic<64>::Logic *logic2 = new MSBitSetLogic<64>::Logic();
    logic2->resize(myLogic->nLinks());
    const MSBitSetLogic<64>::Foes &foes = static_cast<MSBitSetLogic<64>*>(myLogic)->getInternalFoes();
    // go through each link
    size_t running = 0;
    for(size_t i=0; i<myIncomingLanes.size(); ++i) {
        const MSLinkCont &links = myIncomingLanes[i]->getLinkCont();
        for(size_t j=0; j<links.size(); ++j) {
            MSLink *l = links[j];

            // check possible foe links
            size_t running2 = 0;
            for(size_t i2=0; i2<myIncomingLanes.size(); ++i2) {
                const MSLinkCont &links2 = myIncomingLanes[i2]->getLinkCont();
                for(size_t j2=0; j2<links2.size(); ++j2) {
                    MSLink *l2 = links2[j2];
/*
                    if(l2->getLane()->getEdge()->getID()=="-905002550") {
                        int bla = 0;
                    }
                    if(l->getLane()->getEdge()->getID()=="-905002550") {
                        int bla = 0;
                    }
                    if(l2->getLane()->getEdge()->getID()=="-905002550") {
                        if(l->getLane()->getEdge()->getID()=="-905002550") {
                            cout << myIncomingLanes[i]->getEdge()->getID()
                                << endl
                                << myIncomingLanes[i2]->getEdge()->getID()
                                << endl << endl;
                            if(myIncomingLanes[i]->getEdge()->getID()=="-53091655_200_1565") {
                                if(myIncomingLanes[i2]->getEdge()->getID()=="-905002551") {
                                    int bla = 0;
                                }
                            }
                            if(myIncomingLanes[i2]->getEdge()->getID()=="-53091655_200_1565") {
                                if(myIncomingLanes[i]->getEdge()->getID()=="-905002551") {
                                    int bla = 0;
                                }
                            }
                        }
                    }
*/

                    if(foes[running].test(running2)) {//[running2]) {
                        // ok, both do cross
                        if(l->getDirection()!=MSLink::LINKDIR_STRAIGHT) {
                            // orig is turning
                            //  -> keep waiting
                            (*logic2)[running][running2] = areRealFoes(l, l2) ? 1 : 0;
                        } else {
                            if(l2->getDirection()!=MSLink::LINKDIR_STRAIGHT) {
                                // this is straight, the other not
                                //  -> may pass
                                (*logic2)[running][running2] = 0;
                            } else {
                                // both are straight
                                //  -> wait
                                (*logic2)[running][running2] = areRealFoes(l, l2) ? 1 : 0;
                            }
                        }
                    } else {
                        // no crossing -> vehicles may drive
                        (*logic2)[running][running2] = 0;
                    }
                    running2++;
                }
            }
            running++;
        }
    }
    MSBitSetLogic<64> *nlogic = new MSBitSetLogic<64>(myLogic->nLinks(), myLogic->nInLanes(),
        logic2,
        new MSBitSetLogic<64>::Foes(static_cast<MSBitSetLogic<64>*>(myLogic)->getInternalFoes()),
        static_cast<MSBitSetLogic<64>*>(myLogic)->getInternalConts());
    delete myLogic;
    myLogic = nlogic;
    MSJunctionLogic::replace(getID(), myLogic);
}


/**************** DO NOT DEFINE ANYTHING AFTER THE INCLUDE *****************/

// Local Variables:
// mode:C++
// End:
