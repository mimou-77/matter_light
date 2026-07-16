
#pragma once
#ifndef __MATTER_DEFS_H__
#define __MATTER_DEFS_H__


#define FCTRY_RESET_PRESS_DURATION_US (10 * 1000000) // 10s
#define DEBOUNCE_MS 30 // min delay between 2 accepted edges on a btn (bounce filter) ; NOT a min press duration : any press >= 30ms toggles
#define LONG_PRESS_INF_MS 1000 // 1s

#define EXT_CMD_SETTLE_MS 100 // ext cmd edges closer than this belong to the same press (50Hz pulses / bounce) ; a later edge = a new press

/*-----------------------------------------------------------------------------------------------*/
/* types                                                                                         */
/*-----------------------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------------------------*/
/* headers                                                                                       */
/*-----------------------------------------------------------------------------------------------*/










#endif // __MATTER_DEFS_H__