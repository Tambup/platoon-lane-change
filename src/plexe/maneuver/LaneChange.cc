#include "LaneChange.h"
#include "plexe/maneuver/Maneuver.h"
#include "plexe/apps/LaneChangePlatooningApp.h"

#include "plexe/messages/WarnChangeLane_m.h"
#include "plexe/messages/WarnChangeLaneAck_m.h"
#include "plexe/messages/StartSignal_m.h"
#include "plexe/messages/LaneChanged_m.h"
#include "plexe/messages/LaneChangeClose_m.h"
#include "plexe/messages/Again_m.h"


namespace plexe {

LaneChange::LaneChange(GeneralPlatooningApp* app)
    : Maneuver(app)
    , laneChangeManeuverState(LaneChangeManeuverState::IDLE)
{
}

bool LaneChange::handleSelfMsg(cMessage* msg)
{
    for (int i: positionHelper->getPlatoonFormation())
    {
        if (i != positionHelper->getLeaderId())
        {
            LOG << "Leader " << positionHelper->getId() << " sending abort to the follower with id " << i << " because timeout\n";
            //TODO abort
//            StartSignal* response = new StartSignal("StartSignal");
//            app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), i);
//            app->sendUnicast(response, i);
        }
    }
    app->setInManeuver(false, this);
    laneChangeManeuverState = LaneChangeManeuverState::IDLE;

    return true;
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

        static_cast<LaneChangePlatooningApp*>(app)->sendTimeoutMsg();

        return true;
    }
    else {
        return false;
    }
}

void LaneChange::sendLaneChangeRequest(int leaderId, std::string externalId, int platoonId)
{
    for (int i: positionHelper->getPlatoonFormation()) {
        if (i != leaderId){
            LOG << positionHelper->getId() << " sending laneChangeRequest to " << i << "\n";
            WarnChangeLane* msg = new WarnChangeLane("WarnChangeLane");
            app->fillManeuverMessage(msg, leaderId, externalId, platoonId, i);
            app->sendUnicast(msg, i);
        }
    }
}

void LaneChange::resetReceivedAck() {
    for (int i : positionHelper->getPlatoonFormation()) {
        if (i != positionHelper->getLeaderId()) {
            receivedAck[i] = false;
        }
    }
}

void LaneChange::startManeuver(const void* parameters)
{
    if (initializeLaneChangeManeuver())
    {
        resetReceivedAck();
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

void LaneChange::onPlatoonBeacon(const PlatooningBeacon* pb)
{

}

void LaneChange::onManeuverMessage(const ManeuverMessage* mm)
{
    if (const WarnChangeLane* msg = dynamic_cast<const WarnChangeLane*>(mm)) {
        handleWarnChangeLane(msg);
    } else if (const WarnChangeLaneAck* msg = dynamic_cast<const WarnChangeLaneAck*>(mm)) {
        handleWarnChangeLaneAck(msg);
    } else if (const StartSignal* msg = dynamic_cast<const StartSignal*>(mm)) {
        handleStartSignal(msg);
    } else if (const LaneChanged* msg = dynamic_cast<const LaneChanged*>(mm)) {
        handleLaneChanged(msg);
    } else if (const LaneChangeClose* msg = dynamic_cast<const LaneChangeClose*>(mm)) {
        handleLaneChangeClose(msg);
    } else if (const Again* msg = dynamic_cast<const Again*>(mm)) {
        handleAgain(msg);
    }
}

bool LaneChange::processLaneChangeClose(const LaneChangeClose* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::FOLLOWER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::COMPLETE_LANE_CHANGE && !app->isInManeuver()) return false;

    return true;
}

void LaneChange::handleAgain(const Again* msg)
{
    LOG<<"Restart laneChange";
    startManeuver(nullptr);
}


void LaneChange::handleLaneChangeClose(const LaneChangeClose* msg)
{
    if (processLaneChangeClose(msg)) {
        app->setInManeuver(false, this);
        laneChangeManeuverState = LaneChangeManeuverState::IDLE;
    }
//TODO test, is only for restart infinite time the maneuver for testing.
//    if (positionHelper->getId() == 3){
//        Again* response = new Again("Again");
//        app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), positionHelper->getLeaderId());
//        app->sendUnicast(response, positionHelper->getLeaderId());
//    }
}

bool LaneChange::processLaneChanged(const LaneChanged* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::LEADER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::WAIT_ALL_CHANGED && !app->isInManeuver()) return false;

    receivedAck[msg->getVehicleId()] = true;

    for (auto const& x : receivedAck)
    {
        if(!x.second) return false;
    }

    static_cast<LaneChangePlatooningApp*>(app)->resetTimeoutMsg();
    resetReceivedAck();
    return true;
}

void LaneChange::handleLaneChanged(const LaneChanged* msg)
{
    if (processLaneChanged(msg)) {
        // send response to followers
        for (int i: positionHelper->getPlatoonFormation())
        {
            if (i != positionHelper->getLeaderId())
            {
                LOG << "Leader " << positionHelper->getId() << " sending laneChangeClose to the follower with id " << i << "\n";
                LaneChangeClose* response = new LaneChangeClose("LaneChangeClose");
                app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), i);
                app->sendUnicast(response, i);
            }
        }
        app->setInManeuver(false, this);
        laneChangeManeuverState = LaneChangeManeuverState::IDLE;
    }
}

bool LaneChange::processStartSignal(const StartSignal* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::FOLLOWER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::PREPARE_LANE_CHANGE && !app->isInManeuver()) return false;


    laneChangeManeuverState = LaneChangeManeuverState::CHANGE_LANE;
    return true;
}

void LaneChange::handleStartSignal(const StartSignal* msg)
{
    if (processStartSignal(msg)) {
        plexeTraciVehicle->setFixedLane(msg->getDestinationLane(), false);
        positionHelper->setPlatoonLane(msg->getDestinationLane());

        // send response to the leader
        LOG << positionHelper->getId() << " sending laneChanged to the leader (" << msg->getVehicleId() << ")\n";
        LaneChanged* response = new LaneChanged("LaneChanged");
        app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), msg->getVehicleId());
        app->sendUnicast(response, msg->getVehicleId());
    }
}

bool LaneChange::processWarnChangeLaneAck(const WarnChangeLaneAck* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::LEADER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::WAIT_REPLY && !app->isInManeuver()) return false;

    receivedAck[msg->getVehicleId()] = true;

    for (auto const& x : receivedAck)
    {
        if(!x.second) return false;
    }

    static_cast<LaneChangePlatooningApp*>(app)->resetTimeoutMsg();
    laneChangeManeuverState = LaneChangeManeuverState::CHANGE_LANE;
    resetReceivedAck();
    return true;
}

void LaneChange::handleWarnChangeLaneAck(const WarnChangeLaneAck* msg)
{
    double duration = 0;
    //TODO determinare la destinazione in maniera sensata

    std::set<int> possibleDestination;

    if ((positionHelper->getPlatoonLane() +1) < plexeTraciVehicle->getLanesCount()){
        possibleDestination.insert(positionHelper->getPlatoonLane() + 1);
    }
    if((positionHelper->getPlatoonLane() -1) != -1){
        possibleDestination.insert(positionHelper->getPlatoonLane() - 1);
    }

    int destination = *next(possibleDestination.begin(), rand()%possibleDestination.size());

    if (processWarnChangeLaneAck(msg)) {
        plexeTraciVehicle->setFixedLane(destination, true);
        positionHelper->setPlatoonLane(destination);

        // send response to followers
        for (int i: positionHelper->getPlatoonFormation())
        {
            if (i != positionHelper->getLeaderId())
            {
                LOG << "Leader " << positionHelper->getId() << " sending startSignal to the follower with id " << i << "\n";
                StartSignal* response = new StartSignal("StartSignal");
                response->setLaneChangeDutration(duration);
                response->setDestinationLane(destination);
                app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), i);
                app->sendUnicast(response, i);
            }
        }
        laneChangeManeuverState = LaneChangeManeuverState::WAIT_ALL_CHANGED;

        static_cast<LaneChangePlatooningApp*>(app)->sendTimeoutMsg();
    }
}

bool LaneChange::processWarnChangeLane(const WarnChangeLane* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::FOLLOWER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::IDLE || app->isInManeuver()) return false;

    app->setInManeuver(true, this);

    laneChangeManeuverState = LaneChangeManeuverState::PREPARE_LANE_CHANGE;
    return true;
}

void LaneChange::handleWarnChangeLane(const WarnChangeLane* msg)
{
    if (processWarnChangeLane(msg)) {
        // send response to the leader
        LOG << positionHelper->getId() << " sending WarnChangeLaneAck to the leader (" << msg->getVehicleId() << ")\n";
        WarnChangeLaneAck* response = new WarnChangeLaneAck("WarnChangeLaneAck");
        response->setResponse(true);
        app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), msg->getVehicleId());
        app->sendUnicast(response, msg->getVehicleId());
    } else {
        //TODO call abort
    }
}

//TODO VERIFICARE PRIMA CHE LA LANE SIA LIBERA

} // namespace plexe
