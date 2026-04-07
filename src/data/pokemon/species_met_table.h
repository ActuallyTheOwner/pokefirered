#include "constants/region_map_sections.h"

//https://bulbapedia.bulbagarden.net/wiki/List_of_locations_by_index_number_in_Generation_III
#define PALLET_TOWN 0x58
#define ROUTE_101 0x10

static const bool8 SpeciesHasNativeEncounter[NUM_SPECIES] =
{
    [SPECIES_TREECKO] = TRUE,
    [SPECIES_TORCHIC] = TRUE,
    [SPECIES_MUDKIP] = TRUE,
};

static const u32 sSpeciesIdToMetTable[] =
{
    [SPECIES_TREECKO] = ROUTE_101,
    [SPECIES_TORCHIC] = ROUTE_101,
    [SPECIES_MUDKIP] = ROUTE_101,
};

