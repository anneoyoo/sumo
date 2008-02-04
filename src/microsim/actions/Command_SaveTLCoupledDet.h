/****************************************************************************/
/// @file    Command_SaveTLCoupledDet.h
/// @author  Daniel Krajzewicz
/// @date    15 Feb 2004
/// @version $Id$
///
// Writes e2-state on each tls switch
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// copyright : (C) 2001-2007
//  by DLR (http://www.dlr.de/) and ZAIK (http://www.zaik.uni-koeln.de/AFS)
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef Command_SaveTLCoupledDet_h
#define Command_SaveTLCoupledDet_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <string>
#include <microsim/MSNet.h>
#include <utils/common/DiscreteCommand.h>
#include <microsim/traffic_lights/MSTLLogicControl.h>


// ===========================================================================
// class declarations
// ===========================================================================
class MSNet;
class MSTrafficLightLogic;
class MSDetectorFileOutput;
class OutputDevice;


// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class Command_SaveTLCoupledDet
 * @brief Writes e2-state on each tls switch
 *
 * @todo Check whether there may be a better solution than calling "maskRedLinks" directly
 * @todo Problem: The detector may not save the last state (on simulation end)
 * @todo Basically, this does not have to be a DiscreteCommand; its called by the tls (is THIS ok?)
 */
class Command_SaveTLCoupledDet : public DiscreteCommand
{
public:
    /** @brief Constructor
     *
     * @param[in] tlls The logic to observe
     * @param[in] dtf The detector used to generate the values
     * @param[in] begin The begin simulation time
     * @param[in] device The output device to write the detector values into
     */
    Command_SaveTLCoupledDet(MSTLLogicControl::TLSLogicVariants &tlls,
        MSDetectorFileOutput *dtf, unsigned int begin, OutputDevice& device) throw();


    /// @brief Destructor
    virtual ~Command_SaveTLCoupledDet() throw();


    /** @brief Executes the command
     *
     * Called when an active tls program switches, this method calls 
     *  "writeXMLOutput" of its detector (e2; the values are resetted there).
     *
     * Returns always true
     *
     * @return Always true (do not remove)
     * @see MSDetectorFileOutput::writeXMLOutput
     * @see MSE2Collector::writeXMLOutput
     */
    virtual bool execute() throw();


protected:
    /// @brief The file to write the output to
    OutputDevice& myDevice;

    /// @brief The logic to use
    const MSTLLogicControl::TLSLogicVariants &myLogics;

    /// @brief The detector to use
    MSDetectorFileOutput *myDetector;

    /// @brief The last time the values were written
    unsigned int myStartTime;


private:
    /// @brief Invalidated copy constructor.
    Command_SaveTLCoupledDet(const Command_SaveTLCoupledDet&);

    /// @brief Invalidated assignment operator.
    Command_SaveTLCoupledDet& operator=(const Command_SaveTLCoupledDet&);

};


#endif

/****************************************************************************/

