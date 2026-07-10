
#pragma once
#ifndef __MATTER_DEFS_H__
#define __MATTER_DEFS_H__


#define FCTRY_RESET_PRESS_DURATION_US (10 * 1000000) // 10s
#define DEBOUNCE_MS 30 // min delay between 2 accepted edges on a btn (bounce filter) ; NOT a min press duration : any press >= 30ms toggles
#define LONG_PRESS_INF_MS 1000 // 1s

#define EXT_CMD_CONFIRM_MS 30 // a single isolated edge on an armed ext line fires only if the line still reads non-idle after this delay (or if a 2nd edge follows sooner) ; lone emi glitches are discarded
#define EXT_CMD_SETTLE_MS 100 // after an actuation fired, the line must be quiet AND back at its idle level for this long before it re-arms (filters bounce, release edges and relay-switching transients)
#define EXT_CMD_POLL_MS 25    // period of the confirm / re-arm checks while an ext line is not at rest
#define EXT_CMD_IDLE_RESYNC_MS 10000 // a line stuck quiet at a NON-idle level for this long after its actuation adopts that level as the new idle (recovers e.g. a boot that happened while the ext btn was held)

/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/










#endif // __MATTER_DEFS_H__