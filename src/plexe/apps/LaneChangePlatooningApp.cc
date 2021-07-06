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

#include "plexe/apps/GeneralPlatooningApp.h"
#include "plexe/protocols/BaseProtocol.h"
#include "plexe/apps/LaneChangePlatooningApp.h"
#include "veins/modules/mac/ieee80211p/Mac1609_4.h"


using namespace veins;

namespace plexe {

Define_Module(LaneChangePlatooningApp);

void LaneChangePlatooningApp::initialize(int stage)
{
    GeneralPlatooningApp::initialize(stage);

    if (stage == 1) {
        std::string laneChangeManeuverName = par("laneChange").stdstringValue();
        if (laneChangeManeuverName == "LaneChange")
            laneChangeManeuver = new LaneChange(this);
        else
            throw new cRuntimeError("Invalid laneChange maneuver implementation chosen");

        scenario = FindModule<BaseScenario*>::findSubModule(getParentModule());
    }
}

void LaneChangePlatooningApp::handleSelfMsg(cMessage* msg)
{
    if (laneChangeManeuver && laneChangeManeuver->handleSelfMsg(msg)) return;
    BaseApp::handleSelfMsg(msg);
}

void LaneChangePlatooningApp::onPlatoonBeacon(const PlatooningBeacon* pb)
{
    laneChangeManeuver->onPlatoonBeacon(pb);
    // maintain platoon
    BaseApp::onPlatoonBeacon(pb);
}

void LaneChangePlatooningApp::onManeuverMessage(ManeuverMessage* mm)
{
    if (activeManeuver) {
        activeManeuver->onManeuverMessage(mm);
    }
    else {
        laneChangeManeuver->onManeuverMessage(mm);
    }
    delete mm;
}


void LaneChangePlatooningApp::startChangeLaneManeuver(int platoonId, int leaderId)
{
    ASSERT(getPlatoonRole() == PlatoonRole::NONE);
    ASSERT(!isInManeuver());

    laneChangeManeuver->startManeuver(nullptr);
}



void LaneChangePlatooningApp::receiveSignal(cComponent* src, simsignal_t id, cObject* value, cObject* details)
{
    if (id == Mac1609_4::sigRetriesExceeded) {
        BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(value);
        ManeuverMessage* mm = check_and_cast<ManeuverMessage*>(frame->getEncapsulatedPacket());
        if (frame) {
            laneChangeManeuver->onFailedTransmissionAttempt(mm);
        }
    }
}


LaneChangePlatooningApp::~LaneChangePlatooningApp()
{
    delete laneChangeManeuver;
}

} // namespace plexe
