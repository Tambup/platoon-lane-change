This project is an extension of [plexe](github.com/michele-segata/plexe) (3.0) which implements a protocol to make a platoon change lane quite safely.

A FSM of the maneuver, and a high level message description, are in the maneuverDoc folder.

There are two main test, settable changing the otherCarController property in the ini file:
* with "DRIVER", which securely fails the maneuver;
* with "CACC", which succesfully end the maneuver. 
