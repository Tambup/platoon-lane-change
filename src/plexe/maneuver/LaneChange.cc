#include "LaneChange.h"
#include "plexe/maneuver/Maneuver.h"
#include "plexe/apps/LaneChangePlatooningApp.h"

#include "plexe/messages/WarnChangeLane_m.h"

namespace plexe {

LaneChange::LaneChange(GeneralPlatooningApp* app)
    :Maneuver(app)
    ,laneChangeManeuverState(LaneChangeManeuverState::IDLE)
{
}

bool LaneChange::initializeLaneChangeManeuver()
{
    if (laneChangeManeuverState == LaneChangeManeuverState::IDLE && app->getPlatoonRole() == PlatoonRole::LEADER) {
        if (app->isInManeuver()) {
            LOG << positionHelper->getId() << " cannot begin the maneuver because already involved in another one\n";
            return false;
        }

        app->setInManeuver(true, this);

        // after successful initialization we are going to send a request and wait for a reply
        laneChangeManeuverState = LaneChangeManeuverState::WAIT_REPLY;

        return true;
    }
    else {
        return false;
    }
}

void LaneChange::sendLaneChangeRequest(int leaderId, std::string externalId, int platoonId)
{
    for (int i: positionHelper->getPlatoonFormation()) {
        LOG << positionHelper->getId() << " sending laneChangeRequest to " << i << "\n";
        WarnChangeLane* msg = new WarnChangeLane("WarnChangeLane");
        app->fillManeuverMessage(msg, leaderId, externalId, platoonId, i);
        app->sendUnicast(msg, leaderId);
    }
}

void LaneChange::startManeuver(const void* parameters)
{
    if (initializeLaneChangeManeuver()) {
        // send laneChange request to all followers
        sendLaneChangeRequest(positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId());
    }
}

void LaneChange::abortManeuver()
{

}

void LaneChange::onFailedTransmissionAttempt(const ManeuverMessage* mm)
{

}

void LaneChange::onManeuverMessage(const ManeuverMessage* mm)
{

}

void LaneChange::onPlatoonBeacon(const PlatooningBeacon* pb)
{

}

} // namespace plexe
