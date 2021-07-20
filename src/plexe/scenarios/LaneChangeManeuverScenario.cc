//
// Copyright (C) 2012-2021 Michele Segata <segata@ccs-labs.org>
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

#include "plexe/scenarios/LaneChangeManeuverScenario.h"

namespace plexe {

Define_Module(LaneChangeManeuverScenario);

void LaneChangeManeuverScenario::initialize(int stage)
{

    BaseScenario::initialize(stage);

    if (stage == 2) {
        app = FindModule<LaneChangePlatooningApp*>::findSubModule(getParentModule());
        prepareManeuverCars(0);
    }
}

void LaneChangeManeuverScenario::prepareManeuverCars(int platoonLane)
{
    const char* strController = par("otherCarController").stringValue();
    const int numberOfCars = par("numberOfCarsPerPlatoon").intValue();

    const int meneuverTime = par("meneuverTime").intValue();

    const int desiredPlatoonSpeed = par("desiredPlatoonSpeed").intValue();
    const int desiredOtherCarSpeed = par("desiredOtherCarSpeed").intValue();

    ACTIVE_CONTROLLER otherCar;
    if (strcmp(strController, "ACC") == 0) {
        otherCar = ACC;
    } else if (strcmp(strController, "CACC") == 0) {
        otherCar = CACC;
    } else if (strcmp(strController, "PLOEG") == 0) {
        otherCar = PLOEG;
    } else if (strcmp(strController, "CONSENSUS") == 0) {
        otherCar = CONSENSUS;
    } else if (strcmp(strController, "FLATBED") == 0) {
        otherCar = FLATBED;
    } else if (strcmp(strController, "DRIVER") == 0) {
        otherCar = DRIVER;
    } else {
        throw cRuntimeError("Invalid controller selected");
    }
    if (positionHelper->getId() == 0) {
        // this is the leader of the platoon ahead
        plexeTraciVehicle->setCruiseControlDesiredSpeed(desiredPlatoonSpeed / 3.6);
        plexeTraciVehicle->setActiveController(ACC);
        plexeTraciVehicle->setFixedLane(platoonLane);
        app->setPlatoonRole(PlatoonRole::LEADER);

        // after 3 seconds of simulation, start the maneuver
        startManeuver = new cMessage();
        scheduleAt(simTime() + SimTime(meneuverTime), startManeuver);
    }
    else if (!positionHelper->isLeader()) {
        if (positionHelper->getId() < numberOfCars)
        {
            // these are the followers which are already in the platoon
            plexeTraciVehicle->setCruiseControlDesiredSpeed(130.0 / 3.6);
            plexeTraciVehicle->setActiveController(CACC);
            plexeTraciVehicle->setFixedLane(platoonLane);
            app->setPlatoonRole(PlatoonRole::FOLLOWER);
        } else {
            plexeTraciVehicle->setActiveController(otherCar);
            plexeTraciVehicle->setCruiseControlDesiredSpeed(desiredOtherCarSpeed / 3.6);
            plexeTraciVehicle->setFixedLane(1);
        }
    }
}

LaneChangeManeuverScenario::~LaneChangeManeuverScenario()
{
    cancelAndDelete(startManeuver);
    startManeuver = nullptr;
}

void LaneChangeManeuverScenario::handleSelfMsg(cMessage* msg)
{

    // this takes car of feeding data into CACC and reschedule the self message
    BaseScenario::handleSelfMsg(msg);

    LOG << "Starting the change lane maneuver\n";
    if (msg == startManeuver) app->startChangeLaneManeuver(0, 0);
}

} // namespace plexe
