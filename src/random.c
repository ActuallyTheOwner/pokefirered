#include "global.h"
#include "random.h"

// The number 1103515245 comes from the example implementation
// of rand and srand in the ISO C standard.

COMMON_DATA u32 gRngValue = 0;

u16 Random(void)
{
    gRngValue = ISO_RANDOMIZE1(gRngValue);
    return gRngValue >> 16;
}
//Pokeemerald is 0 and 5a0 is for Ruby or Sapphire on a dead battery
void SeedRng(int seed)
{
    gRngValue = seed;
}
