/****************************************************************************/
/// @file    GNECalibrator.h
/// @author  Pablo Alvarez Lopez
/// @date    Nov 2015
/// @version $Id: GNECalibrator.h 19790 2016-01-25 11:59:12Z palcraft $
///
///
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo-sim.org/
// Copyright (C) 2001-2013 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef GNECalibrator_h
#define GNECalibrator_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include "GNEAdditional.h"

// ===========================================================================
// class declarations
// ===========================================================================
class GNERouteProbe;

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class GNECalibrator
 * ------------
 */
class GNECalibrator : public GNEAdditional {
public:
    /**@brief Constructor
     * @param[in] id The storage of gl-ids to get the one for this lane representation from
     * @param[in] lane Lane of this StoppingPlace belongs
     * @param[in] viewNet pointer to GNEViewNet of this additional element belongs
     * @param[in] pos position of the detector on the lane
     * @param[in] frequency the aggregation interval in which to calibrate the flows
     * @param[in] routeProbe The id of the routeProbe element from which to determine the route distribution for generated vehicles
     * @param[in] output The output file for writing calibrator information
     * @param[in] blocked set initial blocking state of item
     */
    GNECalibrator(const std::string& id, GNELane* lane, GNEViewNet* viewNet, SUMOReal pos, SUMOReal frequency, GNERouteProbe *routeProbe, const std::string& output, bool blocked);

    /// @brief Destructor
    ~GNECalibrator();

    /// @brief update pre-computed geometry information
    /// @note: must be called when geometry changes (i.e. lane moved)
    void updateGeometry();

    /**@brief writte additional element into a xml file
     * @param[in] device device in which write parameters of additional element
     */
    void writeAdditional(OutputDevice& device);

    /// @name inherited from GUIGlObject
    /// @{
    /**@brief Returns an own parameter window
     *
     * @param[in] app The application needed to build the parameter window
     * @param[in] parent The parent window needed to build the parameter window
     * @return The built parameter window
     * @see GUIGlObject::getParameterWindow
     */
    GUIParameterTableWindow* getParameterWindow(GUIMainWindow& app, GUISUMOAbstractView& parent);

    /**@brief Draws the object
     * @param[in] s The settings for the current view (may influence drawing)
     * @see GUIGlObject::drawGL
     */
    void drawGL(const GUIVisualizationSettings& s) const;
    /// @}

    /// @name inherited from GNEAttributeCarrier
    /// @{
    /* @brief method for getting the Attribute of an XML key
     * @param[in] key The attribute key
     * @return string with the value associated to key
     */
    std::string getAttribute(SumoXMLAttr key) const;

    /* @brief method for setting the attribute and letting the object perform additional changes
     * @param[in] key The attribute key
     * @param[in] value The new value
     * @param[in] undoList The undoList on which to register changes
     */
    void setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList);

    /* @brief method for checking if the key and their correspond attribute are valids
     * @param[in] key The attribute key
     * @param[in] value The value asociated to key key
     * @return true if the value is valid, false in other case
     */
    bool isValid(SumoXMLAttr key, const std::string& value);
    /// @}

protected:
    /// @brief lane in which this calibrator is placed
    GNELane *myLane;

    /// @brief Frequency of calibrator
    int myFrequency;

    /// @brief Routeprobe vinculated with this Calibrator
    GNERouteProbe *myRouteProbe;

    /// @brief output of calibrator
    std::string myOutput;

private:
    /// @brief set attribute after validation
    void setAttribute(SumoXMLAttr key, const std::string& value);

    /// @brief Invalidated copy constructor.
    GNECalibrator(const GNECalibrator&);

    /// @brief Invalidated assignment operator.
    GNECalibrator& operator=(const GNECalibrator&);
};

#endif
/****************************************************************************/