#include "LaneChange.h"
#include "plexe/maneuver/Maneuver.h"
#include "plexe/apps/LaneChangePlatooningApp.h"

#include "plexe/messages/WarnLaneChange_m.h"
#include "plexe/messages/WarnLaneChangeAck_m.h"
#include "plexe/messages/StartSignal_m.h"
#include "plexe/messages/LaneChanged_m.h"
#include "plexe/messages/LaneChangeClose_m.h"
#include "plexe/messages/Again_m.h"
#include "plexe/messages/Abort_m.h"


namespace plexe {

LaneChange::LaneChange(GeneralPlatooningApp* app, int securityDistance)
    : Maneuver(app)
    , laneChangeManeuverState(LaneChangeManeuverState::IDLE)
    , securityDistance(securityDistance)
{
}

bool LaneChange::handleSelfMsg(cMessage* msg)
{
    std::string title = msg->getName();
    if (title.compare("TimeoutMsg") == 0) {
        abortManeuver();
    }

    return true;
}

bool LaneChange::isLaneFree(int destination)
{
    int state, state2;
    plexeTraciVehicle->getLaneChangeState(destination-positionHelper->getPlatoonLane(), state, state2);
    int direction = destination-positionHelper->getPlatoonLane() > 0 ? 0 : 1;
    double minBack = plexeTraciVehicle->getMinNeighDistance(direction, 0);
    double minFront = plexeTraciVehicle->getMinNeighDistance(direction, 1);

    if ((state & (1 << 13)) != 0 || minBack < securityDistance || minFront < securityDistance)
    {
        LOG << positionHelper->getId() << " cannot begin the maneuver because lane " << destination << " is occupied\n";
        return false;
    }
    return true;
}

bool LaneChange::initializeLaneChangeManeuver()
{
    if (laneChangeManeuverState == LaneChangeManeuverState::IDLE && app->getPlatoonRole() == PlatoonRole::LEADER) {
        if (app->isInManeuver())
        {
            LOG << positionHelper->getId() << " cannot begin the maneuver because already involved in another one\n";
            return false;
        }
        if (!isLaneFree(nextDestination)) return false;

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
            WarnLaneChange* msg = new WarnLaneChange("WarnLaneChange");
            msg->setPlatoonLaneDestination(nextDestination);
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
    nextDestination = destination();
    if (initializeLaneChangeManeuver())
    {
        resetReceivedAck();
        // send laneChange request to all followers
        sendLaneChangeRequest(positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId());
    }
}

void LaneChange::handleAbort()
{
    nextDestination = -1;
    app->setInManeuver(false, this);
    laneChangeManeuverState = LaneChangeManeuverState::IDLE;
}

void LaneChange::abortManeuver()
{
    for (int i: positionHelper->getPlatoonFormation())
    {
        if (i != positionHelper->getId())
        {
            LOG << "Platoon member " << positionHelper->getId() << " sending Abort to member " << i << "\n";
            Abort* response = new Abort("Abort");
            app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), i);
            app->sendUnicast(response, i);
        }
    }
    nextDestination = -1;
    app->setInManeuver(false, this);
    laneChangeManeuverState = LaneChangeManeuverState::IDLE;
}

void LaneChange::onFailedTransmissionAttempt(const ManeuverMessage* mm)
{
    throw cRuntimeError("Impossible to send this packet: %s. Maximum number of unicast retries reached", mm->getName());
}

void LaneChange::onPlatoonBeacon(const PlatooningBeacon* pb)
{

}

void LaneChange::onManeuverMessage(const ManeuverMessage* mm)
{
    if (const WarnLaneChange* msg = dynamic_cast<const WarnLaneChange*>(mm)) {
        handleWarnLaneChange(msg);
    } else if (const WarnLaneChangeAck* msg = dynamic_cast<const WarnLaneChangeAck*>(mm)) {
        handleWarnLaneChangeAck(msg);
    } else if (const StartSignal* msg = dynamic_cast<const StartSignal*>(mm)) {
        handleStartSignal(msg);
    } else if (const LaneChanged* msg = dynamic_cast<const LaneChanged*>(mm)) {
        handleLaneChanged(msg);
    } else if (const LaneChangeClose* msg = dynamic_cast<const LaneChangeClose*>(mm)) {
        handleLaneChangeClose(msg);
    } else if (const Again* msg = dynamic_cast<const Again*>(mm)) {
        handleAgain(msg);
    } else if (const Abort* msg = dynamic_cast<const Abort*>(mm)) {
        handleAbort();
    }
}

bool LaneChange::processLaneChangeClose(const LaneChangeClose* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::FOLLOWER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::COMPLETE_LANE_CHANGE || !app->isInManeuver()) return false;

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
        nextDestination = -1;
        app->setInManeuver(false, this);
        laneChangeManeuverState = LaneChangeManeuverState::IDLE;
    } else {
        abortManeuver();
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

    if (laneChangeManeuverState != LaneChangeManeuverState::WAIT_ALL_CHANGED || !app->isInManeuver()) return false;

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
        nextDestination = -1;
        app->setInManeuver(false, this);
        laneChangeManeuverState = LaneChangeManeuverState::IDLE;
    }
}

bool LaneChange::processStartSignal(const StartSignal* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::FOLLOWER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::PREPARE_LANE_CHANGE || !app->isInManeuver()) return false;

    if (nextDestination < 0) return false;

    laneChangeManeuverState = LaneChangeManeuverState::LANE_CHANGE;
    return true;
}

void LaneChange::handleStartSignal(const StartSignal* msg)
{
    if (processStartSignal(msg)) {
        plexeTraciVehicle->setFixedLane(nextDestination, false);
        positionHelper->setPlatoonLane(nextDestination);

        laneChangeManeuverState = LaneChangeManeuverState::COMPLETE_LANE_CHANGE;

        // send response to the leader
        LOG << positionHelper->getId() << " sending laneChanged to the leader (" << msg->getVehicleId() << ")\n";
        LaneChanged* response = new LaneChanged("LaneChanged");
        app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), msg->getVehicleId());
        app->sendUnicast(response, msg->getVehicleId());
    } else {
        abortManeuver();
    }
}

bool LaneChange::processWarnLaneChangeAck(const WarnLaneChangeAck* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::LEADER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::WAIT_REPLY || !app->isInManeuver()) return false;

    receivedAck[msg->getVehicleId()] = true;

    for (auto const& x : receivedAck)
    {
        if(!x.second) return false;
    }

    static_cast<LaneChangePlatooningApp*>(app)->resetTimeoutMsg();
    laneChangeManeuverState = LaneChangeManeuverState::LANE_CHANGE;
    resetReceivedAck();
    return true;
}

int LaneChange::destination()
{
    std::set<int> possibleDestination;

    if ((positionHelper->getPlatoonLane() + 1) < plexeTraciVehicle->getLanesCount())
    {
        possibleDestination.insert(positionHelper->getPlatoonLane() + 1);
    }
    if((positionHelper->getPlatoonLane() - 1) != -1)
    {
        possibleDestination.insert(positionHelper->getPlatoonLane() - 1);
    }

    return *next(possibleDestination.begin(), rand()%possibleDestination.size());
}

void LaneChange::handleWarnLaneChangeAck(const WarnLaneChangeAck* msg)
{
    if (processWarnLaneChangeAck(msg)) {
        if (nextDestination >= 0)
        {
            plexeTraciVehicle->setFixedLane(nextDestination, false);
            positionHelper->setPlatoonLane(nextDestination);

            // send response to followers
            for (int i: positionHelper->getPlatoonFormation())
            {
                if (i != positionHelper->getLeaderId())
                {
                    LOG << "Leader " << positionHelper->getId() << " sending startSignal to the follower with id " << i << "\n";
                    StartSignal* response = new StartSignal("StartSignal");
                    app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), i);
                    app->sendUnicast(response, i);
                }
            }
            laneChangeManeuverState = LaneChangeManeuverState::WAIT_ALL_CHANGED;

            static_cast<LaneChangePlatooningApp*>(app)->sendTimeoutMsg();
        } else
            abortManeuver();
    }
}

bool LaneChange::processWarnLaneChange(const WarnLaneChange* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId()) return false;

    if (app->getPlatoonRole() != PlatoonRole::FOLLOWER) return false;

    if (laneChangeManeuverState != LaneChangeManeuverState::IDLE || app->isInManeuver()) return false;

    if (!isLaneFree(msg->getPlatoonLaneDestination())) return false;

    app->setInManeuver(true, this);

    laneChangeManeuverState = LaneChangeManeuverState::PREPARE_LANE_CHANGE;
    return true;
}

void LaneChange::handleWarnLaneChange(const WarnLaneChange* msg)
{
    if (processWarnLaneChange(msg)) {
        nextDestination = msg->getPlatoonLaneDestination();
        // send response to the leader
        LOG << positionHelper->getId() << " sending WarnLaneChangeAck to the leader (" << msg->getVehicleId() << ")\n";
        WarnLaneChangeAck* response = new WarnLaneChangeAck("WarnLaneChangeAck");
        response->setResponse(true);
        app->fillManeuverMessage(response, positionHelper->getId(), positionHelper->getExternalId(), positionHelper->getPlatoonId(), msg->getVehicleId());
        app->sendUnicast(response, msg->getVehicleId());
    } else {
        abortManeuver();
    }
}

} // namespace plexe
