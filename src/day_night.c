#include "global.h"
#include "day_night.h"
#include "constants/day_night.h"
#include "string_util.h"
#include "text.h"
#include "event_data.h"

static EWRAM_DATA u8 sTimeOfDayCheat = TIME_NOON;

u16 ScrCmd_CheatedTime(void)
{
    return sTimeOfDayCheat;
}

void CheatTimeOfDay(void)
{
    sTimeOfDayCheat = gSpecialVar_0x8003;
}
