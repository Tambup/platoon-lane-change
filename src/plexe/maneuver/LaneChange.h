#ifndef LANECHANGE_H_
#define LANECHANGE_H_

#include <algorithm>

#include "plexe/maneuver/Maneuver.h"

#include "plexe/messages/WarnLaneChange_m.h"
#include "plexe/messages/WarnLaneChangeAck_m.h"
#include "plexe/messages/StartSignal_m.h"
#include "plexe/messages/LaneChanged_m.h"
#include "plexe/messages/LaneChangeClose_m.h"
#include "plexe/messages/Again_m.h"

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
     * Handles a WarnLaneChange in the context of this application
     *
     * @param WarnLaneChange msg to handle
     */
    void handleWarnLaneChange(const WarnLaneChange* mm);

    /**
     * Handles a handleWarnLaneChangeAck in the context of this application
     *
     * @param WarnLaneChangeAck msg to handle
     */
    void handleWarnLaneChangeAck(const WarnLaneChangeAck* mm);
    //TODO eminare test
    void handleAgain(const Again* msg);

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

    /**
     * Handles an Abort message in the context of this application
     *
     */
    void handleAbort();

    virtual void abortManeuver() override;
    virtual void onFailedTransmissionAttempt(const ManeuverMessage* mm) override;
    virtual void onManeuverMessage(const ManeuverMessage* mm) override;
    virtual void onPlatoonBeacon(const PlatooningBeacon* pb) override;
protected:
    /** Possible states a vehicle can be in during a laneChange maneuver */
    enum class LaneChangeManeuverState {
        IDLE, ///< The maneuver did not start
        LANE_CHANGE, ///< The maneuver is running
        // LEADER
        WAIT_REPLY, ///< The Leader is waiting all platoon members response
        WAIT_ALL_CHANGED, ///< The Leader is waiting all platoon members finished the maneuver
        // FOLLOWER
        PREPARE_LANE_CHANGE, ///< The follower evalueate if is possible to change and if possible send an ack
        COMPLETE_LANE_CHANGE ///< The follower advert the successfully end of the maneuver
    };

    /** the current state in the laneChange maneuver */
    LaneChangeManeuverState laneChangeManeuverState;

    /** initializes a laneChange maneuver, setting up required data */
    bool initializeLaneChangeManeuver();

    /** initializes the handling of a WarnLaneChange */
    bool processWarnLaneChange(const WarnLaneChange* msg);

    /** initializes the handling of a WarnLaneChange */
    bool processWarnLaneChangeAck(const WarnLaneChangeAck* msg);

    /** initializes the handling of a StartSignal */
    bool processStartSignal(const StartSignal* msg);

    /** initializes the handling of a LaneChanged message */
    bool processLaneChanged(const LaneChanged* msg);

    /** initializes the handling of a LaneChangeClose message */
    bool processLaneChangeClose(const LaneChangeClose* msg);

private:
    void sendLaneChangeRequest(int leaderId, std::string externalId, int platoonId);
    void resetReceivedAck();

    int destination();

    bool isLaneFree(int destination);

    virtual bool handleSelfMsg(cMessage* msg) override;

    std::map<int, bool> receivedAck;
    // -1 no destination value
    int nextDestination = -1;
};

} // namespace plexe

#endif
