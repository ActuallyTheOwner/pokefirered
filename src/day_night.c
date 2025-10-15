#include "global.h"
#include "day_night.h"
#include "constants/day_night.h"
#include "string_util.h"
#include "text.h"
#include "event_data.h"

static EWRAM_DATA u8 sTimeOfDay = TIME_NOON;

u16 GetCurrentTimeOfDay(void)
{
    return sTimeOfDay;
}

void SetTimeOfDay(void)
{
    sTimeOfDay = gSpecialVar_0x8003;
}

void LoadClock(void)
{
    sTimeOfDay = gSaveBlock2Ptr->localTimeOffset.hours;
}

void AddTimeOfDay(void)
{
    u8 offset;
    offset = 1;

    if (sTimeOfDay + offset <= TIME_MIDNIGHT)
        sTimeOfDay = sTimeOfDay + offset;
    else
        sTimeOfDay = TIME_PREDAWN;
}

void SubtractTimeOfDay(void)
{
    u8 offset;
    offset = 1;

    //0
    if (sTimeOfDay - offset > TIME_PREDAWN)
        sTimeOfDay = sTimeOfDay - offset;
    else
        sTimeOfDay = TIME_MIDNIGHT;
}
