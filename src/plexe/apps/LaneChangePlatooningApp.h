
//
// Copyright (C) 2012-2021 Michele Segata <segata@ccs-labs.org>
// Copyright (C) 2018-2021 Julian Heinovski <julian.heinovski@ccs-labs.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef LANECHANGEPLATTONINGAPP_H_
#define LANECHANGEPLATTONINGAPP_H_

#include <algorithm>
#include <memory>

#include "plexe/apps/BaseApp.h"
#include "plexe/apps/GeneralPlatooningApp.h"
#include "plexe/maneuver/Maneuver.h"
#include "plexe/maneuver/LaneChange.h"


#include "plexe/messages/ManeuverMessage_m.h"
#include "plexe/messages/UpdatePlatoonData_m.h"

#include "plexe/scenarios/BaseScenario.h"

#include "veins/modules/mobility/traci/TraCIConstants.h"
#include "veins/modules/utility/SignalManager.h"

namespace plexe {

/**
 * General purpose application for Platoons
 *
 * This class handles maintenance of an existing Platoon by storing relevant
 * information about it and feeding the CACC.
 * It also provides an easily extendable base for supporting arbitrary
 * maneuvers.
 *
 * @see BaseApp
 * @see Maneuver
 * @see JoinManeuver
 * @see JoinAtBack
 * @see ManeuverMessage
 */
class LaneChangePlatooningApp : public GeneralPlatooningApp {

public:
    /** c'tor for LaneChangePlatooningApp */
    LaneChangePlatooningApp()
        : GeneralPlatooningApp()
        , laneChangeManeuver(nullptr)

    {
    }

    /** d'tor for LaneChangePlatooningApp */
    virtual ~LaneChangePlatooningApp();

    virtual void startChangeLaneManeuver(int platoonId, int leaderId);

    /** override from GeneralPlatooningApp */
    virtual void initialize(int stage) override;

    /** override from GeneralPlatooningApp */
    virtual void handleSelfMsg(cMessage* msg) override;



protected:
    /** used to receive the "retries exceeded" signal **/
    virtual void receiveSignal(cComponent* src, simsignal_t id, cObject* value, cObject* details) override;

    /**
     * Handles PlatoonBeacons
     *
     * @param PlatooningBeacon pb to handle
     */
    virtual void onPlatoonBeacon(const PlatooningBeacon* pb) override;

    /**
     * Handles ManeuverMessages
     *
     * @param ManeuverMessage mm to handle
     */
    virtual void onManeuverMessage(ManeuverMessage* mm);



private:
    /** platoons change lane implementation */
    Maneuver* laneChangeManeuver;
};

} // namespace plexe

#endif
