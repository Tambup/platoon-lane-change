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

#ifndef LANECHANGE_H_
#define LANECHANGE_H_

#include <algorithm>

#include "plexe/utilities/BasePositionHelper.h"
#include "plexe/maneuver/Maneuver.h"
#include "plexe/messages/ManeuverMessage_m.h"
#include "plexe/messages/MoveToPositionAck_m.h"
#include "plexe/messages/MoveToPosition_m.h"
#include "plexe/messages/MergePlatoonRequest_m.h"


#include "veins/modules/mobility/traci/TraCIConstants.h"
#include "veins/base/utils/Coord.h"

using namespace veins;

namespace plexe {

struct LaneChangeParameters {
    int platoonId;
    int leaderId;
};

class LaneChange : public Maneuver {

public:
    /**
     * Constructor
     *
     * @param app pointer to the generic application used to fetch parameters and inform it about a concluded maneuver
     */
    LaneChange(GeneralPlatooningApp* app);
    ~LaneChange(){};

    virtual void onManeuverMessage(const ManeuverMessage* mm) override;

    /**
     * This method is invoked by the generic application to start the maneuver
     *
     * @param parameters parameters passed to the maneuver
     */
    void startManeuver(const void* parameters) override;

    /**
     * Handles the abortion of the maneuver when required by the generic application.
     * This method does currently nothing and it is meant for future used and improved maneuvers.
     */
    void abortManeuver() override;

    /**
     * This method is invoked by the generic application when a beacon message is received
     * The maneuver must not free the memory of the message, as this might be needed by other maneuvers as well.
     */
    void onPlatoonBeacon(const PlatooningBeacon* pb) override;

    /**
     * This method is invoked by the generic application when a failed transmission occurred, indicating the packet for which transmission has failed
     * The manuever must not free the memory of the message, as this might be needed by other maneuvers as well.
     */
    void onFailedTransmissionAttempt(const ManeuverMessage* mm) override;




protected:
    /** Possible states a vehicle can be in during a join maneuver */
    enum class LaneChangeManeuverState {
        IDLE, ///< The maneuver did not start
        // Joiner
        J_WAIT_REPLY, ///< The joiner sent a JoinRequest waits for a reply from the Platoon leader
        J_WAIT_INFORMATION, ///< The joiner waits for information about the Platoon
        J_MOVE_IN_POSITION, ///< The joiner moves to its position
        J_WAIT_JOIN, ///< The joiner waits for the join permission
        // Leader
        L_WAIT_JOINER_IN_POSITION, ///< The leader waits for the joiner to be in position, the followers made space already
        L_WAIT_JOINER_TO_JOIN, ///< The leader waits for the joiner to join
    };

    /** data that a joiner stores about a Platoon it wants to join */
    struct TargetPlatoonData {
        int platoonId; ///< the id of the platoon to join
        int platoonLeader; ///< the if ot the leader of the platoon
        int platoonLane; ///< the lane the platoon is driving on
        double platoonSpeed; ///< the speed of the platoon
        int joinIndex; ///< position in the new platoon formation, 0 based !
        std::vector<int> newFormation; ///< the new formation of the platoon
        Coord lastFrontPos; ///< the last kwown position of the front vehicle

        /** c'tor for TargetPlatoonData */
        TargetPlatoonData()
        {
            platoonId = INVALID_PLATOON_ID;
            platoonLeader = TraCIConstants::INVALID_INT_VALUE;
            platoonLane = TraCIConstants::INVALID_INT_VALUE;
            platoonSpeed = TraCIConstants::INVALID_DOUBLE_VALUE;
            joinIndex = TraCIConstants::INVALID_INT_VALUE;
        }

        /**
         * Initializes the TargetPlatoonData object with the contents of a
         * MoveToPosition
         *
         * @param MoveToPosition msg to initialize from
         * @see MoveToPosition
         */
        void from(const MoveToPosition* msg)
        {
            platoonId = msg->getPlatoonId();
            platoonLeader = msg->getVehicleId();
            platoonLane = msg->getPlatoonLane();
            platoonSpeed = msg->getPlatoonSpeed();
            newFormation.resize(msg->getNewPlatoonFormationArraySize());
            for (unsigned int i = 0; i < msg->getNewPlatoonFormationArraySize(); i++) {
                newFormation[i] = msg->getNewPlatoonFormation(i);
            }
            const auto it = std::find(newFormation.begin(), newFormation.end(), msg->getDestinationId());
            if (it != newFormation.end()) {
                joinIndex = std::distance(newFormation.begin(), it);
                ASSERT(newFormation.at(joinIndex) == msg->getDestinationId());
            }
        }

        /**
         * Get the id of the front vehicle
         *
         * @return int the id of the front vehicle
         */
        int frontId() const
        {
            return newFormation.at(joinIndex - 1);
        }
    };

    /** data that a leader stores about a joining vehicle */
    struct JoinerData {
        int joinerId; ///< the id of the vehicle joining the Platoon
        int joinerLane; ///< the lane chosen for joining the Platoon
        std::vector<int> newFormation;

        /** c'tor for JoinerData */
        JoinerData()
        {
            joinerId = TraCIConstants::INVALID_INT_VALUE;
            joinerLane = TraCIConstants::INVALID_INT_VALUE;
        }

        /**
         * Initializes the JoinerData object with the contents of a
         * JoinPlatoonRequest
         *
         * @param JoinPlatoonRequest jr to initialize from
         * @see JoinPlatoonRequest
         */
        void from(const JoinPlatoonRequest* msg)
        {
            joinerId = msg->getVehicleId();
            joinerLane = msg->getCurrentLaneIndex();
        }
    };

    /** the current state in the join maneuver */
    LaneChangeManeuverState laneChangeManeuverState;

    /** the data about the target platoon */
    std::unique_ptr<TargetPlatoonData> targetPlatoonData;

    /** the data about the current joiner */
    std::unique_ptr<JoinerData> joinerData;

    /** initializes a join maneuver, setting up required data */
    bool initializeLaneChangeManeuver(const void* parameters);

    /** initializes the handling of a join request */
    bool processJoinRequest(const JoinPlatoonRequest* msg);
};

} // namespace plexe

#endif
