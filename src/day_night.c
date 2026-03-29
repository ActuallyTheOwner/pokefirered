#include "global.h"
#include "day_night.h"
#include "constants/day_night.h"
#include "string_util.h"
#include "text.h"
#include "event_data.h"
#include "rtc.h"
#include "field_screen.h"

static EWRAM_DATA u8 sTimeOfDay = TIME_INVALID;

static void UpdatePerDay(struct Time *localTime)
{
    u16 *days = GetVarPointer(VAR_DAYS);
    u16 daysSince;

    if (*days != localTime->days && *days <= localTime->days)
    {
        daysSince = localTime->days - *days;
        //ClearDailyFlags();
        UpdateWeatherPerDay(daysSince);
        //UpdatePartyPokerusTime(daysSince);
        *days = localTime->days;
    }
}

static void UpdatePerMinute(struct Time *localTime)
{
    struct Time difference;
    int minutes;

    CalcTimeDifference(&difference, &gSaveBlock2Ptr->lastBerryTreeUpdate, localTime);
    minutes = 24 * 60 * difference.days + 60 * difference.hours + difference.minutes;
    if (minutes != 0)
    {
        if (minutes >= 0)
        {
            //BerryTreeTimeUpdate(minutes);
            gSaveBlock2Ptr->lastBerryTreeUpdate = *localTime;
        }
    }
}


void DoTimeBasedEvents(void)
{
    // Ruby and Sapphire do not update
    // when inside of Pokemon Centers
    if (FlagGet(FLAG_PLAYER_ADVENTURE_STARTED)) 
    {
        RtcCalcLocalTime();
        UpdatePerDay(&gLocalTime);
        UpdatePerMinute(&gLocalTime); // no berry trees
    }
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

    //1
    if (sTimeOfDay - offset > TIME_PREDAWN)
        sTimeOfDay = sTimeOfDay - offset;
    else
        sTimeOfDay = TIME_MIDNIGHT;
}
