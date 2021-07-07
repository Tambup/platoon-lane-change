#ifndef LANECHANGE_H_
#define LANECHANGE_H_

#include <algorithm>

#include "plexe/maneuver/Maneuver.h"

#include "plexe/messages/WarnChangeLane_m.h"
#include "plexe/messages/WarnChangeLaneAck_m.h"
#include "plexe/messages/StartSignal_m.h"
#include "plexe/messages/LaneChanged_m.h"
#include "plexe/messages/LaneChangeClose_m.h"

using namespace veins;

namespace plexe {

class LaneChange : public Maneuver {

public:
    /**
     * Constructor
     *
     * @param app pointer to the generic application used to fetch parameters and inform it about a concluded maneuver
     */
    LaneChange(GeneralPlatooningApp* app);
    ~LaneChange(){};

    /**
     * This method is invoked by the generic application to start the maneuver
     *
     * @param parameters parameters passed to the maneuver
     */
    virtual void startManeuver(const void* parameters) override;

    /**
     * Handles a WarnChangeLane in the context of this application
     *
     * @param WarnChangeLane msg to handle
     */
    void handleWarnChangeLane(const WarnChangeLane* mm);

    /**
     * Handles a handleWarnChangeLaneAck in the context of this application
     *
     * @param WarnChangeLaneAck msg to handle
     */
    void handleWarnChangeLaneAck(const WarnChangeLaneAck* mm);

    /**
     * Handles a StartSignal in the context of this application
     *
     * @param StartSignal msg to handle
     */
    void handleStartSignal(const StartSignal* msg);

    /**
     * Handles a LaneChanged in the context of this application
     *
     * @param LaneChanged msg to handle
     */
    void handleLaneChanged(const LaneChanged* msg);

    /**
     * Handles a LaneChangeClose in the context of this application
     *
     * @param LaneChangeClose msg to handle
     */
    void handleLaneChangeClose(const LaneChangeClose* msg);

    virtual void abortManeuver() override;
    virtual void onFailedTransmissionAttempt(const ManeuverMessage* mm) override;
    virtual void onManeuverMessage(const ManeuverMessage* mm) override;
    virtual void onPlatoonBeacon(const PlatooningBeacon* pb) override;
protected:
    /** Possible states a vehicle can be in during a laneChange maneuver */
    enum class LaneChangeManeuverState {
        IDLE, ///< The maneuver did not start
        CHANGE_LANE, ///< The maneuver is running
        // LEADER
        WAIT_REPLY, ///< The Leader is waiting all platoon members response
        WAIT_ALL_CHANGED, ///< The Leader is waiting all platoon members finished the maneuver
        // FOLLOWER
        PREPARE_LANE_CHANGE, ///< The follower evalueate if is possible to change and if possible send an ack
        WAIT_START_SIGNAL, ///< The follower waits for the Leader start message
        COMPLETE_LANE_CHANGE ///< The follower advert the successfully end of the maneuver
    };

    /** the current state in the laneChange maneuver */
    LaneChangeManeuverState laneChangeManeuverState;

    /** initializes a laneChange maneuver, setting up required data */
    bool initializeLaneChangeManeuver();

    /** initializes the handling of a WarnChangeLane */
    bool processWarnChangeLane(const WarnChangeLane* msg);

    /** initializes the handling of a WarnChangeLane */
    bool processWarnChangeLaneAck(const WarnChangeLaneAck* msg);

    /** initializes the handling of a StartSignal */
    bool processStartSignal(const StartSignal* msg);

    /** initializes the handling of a LaneChanged message */
    bool processLaneChanged(const LaneChanged* msg);

    /** initializes the handling of a LaneChangeClose message */
    bool processLaneChangeClose(const LaneChangeClose* msg);

private:
    void sendLaneChangeRequest(int leaderId, std::string externalId, int platoonId);
    void resetReceivedAck();

    virtual bool handleSelfMsg(cMessage* msg) override;

    std::map<int, bool> receivedAck;
};

} // namespace plexe

#endif
