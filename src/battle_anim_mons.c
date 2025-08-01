#include "global.h"
#include "gflib.h"
#include "battle_anim.h"
#include "data.h"
#include "decompress.h"
#include "pokemon_icon.h"
#include "task.h"
#include "trig.h"
#include "util.h"
#include "constants/battle_anim.h"

#define IS_DOUBLE_BATTLE() (gBattleTypeFlags & BATTLE_TYPE_DOUBLE)

static u8 GetBattlerSpriteFinal_Y(u8 battlerId, u16 species, bool8 a3);
static void PlayerThrowBall_AnimTranslateLinear_WithFollowup(struct Sprite *sprite);
static void AnimFastTranslateLinearWaitEnd(struct Sprite *sprite);
static bool8 ShouldRotScaleSpeciesBeFlipped(void);
static void AnimThrowProjectile_Step(struct Sprite *sprite);
static void AnimTask_AlphaFadeIn_Step(u8 taskId);
static void AnimTask_BlendMonInAndOutSetup(struct Task *task);
static void AnimTask_BlendMonInAndOut_Step(u8 taskId);
static u16 GetBattlerYDeltaFromSpriteId(u8 spriteId);
static void AnimTask_AttackerPunchWithTrace_Step(u8 taskId);
static void CreateBattlerTrace(struct Task *task, u8 taskId);
static void AnimBattlerTrace(struct Sprite *sprite);
static void AnimWeatherBallUp_Step(struct Sprite *sprite);

static EWRAM_DATA union AffineAnimCmd *sAnimTaskAffineAnim = NULL;

static const struct UCoords8 sBattlerCoords[][MAX_BATTLERS_COUNT] =
{
    { // Single battle
        { 72, 80 },
        { 176, 40 },
        { 48, 40 },
        { 112, 80 },
    },
    { // Double battle
        { 32, 80 },
        { 200, 40 },
        { 90, 88 },
        { 152, 32 },
    },
};

// One entry for each of the four Castform forms.
const struct MonCoords gCastformFrontSpriteCoords[NUM_CASTFORM_FORMS] =
{
    [CASTFORM_NORMAL] = { .size = MON_COORDS_SIZE(32, 32), .y_offset = 17 },
    [CASTFORM_FIRE]   = { .size = MON_COORDS_SIZE(48, 48), .y_offset =  9 },
    [CASTFORM_WATER]  = { .size = MON_COORDS_SIZE(32, 48), .y_offset =  9 },
    [CASTFORM_ICE]    = { .size = MON_COORDS_SIZE(64, 48), .y_offset =  8 },
};

static const u8 sCastformElevations[NUM_CASTFORM_FORMS] =
{
    [CASTFORM_NORMAL] = 13,
    [CASTFORM_FIRE]   = 14,
    [CASTFORM_WATER]  = 13,
    [CASTFORM_ICE]    = 13,
};

// Y position of the backsprite for each of the four Castform forms.
static const u8 sCastformBackSpriteYCoords[NUM_CASTFORM_FORMS] =
{
    [CASTFORM_NORMAL] = 0,
    [CASTFORM_FIRE]   = 0,
    [CASTFORM_WATER]  = 0,
    [CASTFORM_ICE]    = 0,
};

// Placeholders for pokemon sprites to be created for a move animation effect (e.g. Role Play / Snatch)
#define TAG_MOVE_EFFECT_MON_1 55125
#define TAG_MOVE_EFFECT_MON_2 55126

static const struct SpriteTemplate sSpriteTemplates_MoveEffectMons[] =
{
    {
        .tileTag = TAG_MOVE_EFFECT_MON_1,
        .paletteTag = TAG_MOVE_EFFECT_MON_1,
        .oam = &gOamData_AffineNormal_ObjNormal_64x64,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    {
        .tileTag = TAG_MOVE_EFFECT_MON_2,
        .paletteTag = TAG_MOVE_EFFECT_MON_2,
        .oam = &gOamData_AffineNormal_ObjNormal_64x64,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    }
};

static const struct SpriteSheet sSpriteSheets_MoveEffectMons[] =
{
    { gMiscBlank_Gfx, MON_PIC_SIZE, TAG_MOVE_EFFECT_MON_1 },
    { gMiscBlank_Gfx, MON_PIC_SIZE, TAG_MOVE_EFFECT_MON_2 },
};

u8 GetBattlerSpriteCoord(u8 battlerId, u8 coordType)
{
    u8 retVal;
    u16 species;
    struct BattleSpriteInfo *spriteInfo;

    switch (coordType)
    {
    case BATTLER_COORD_X:
    case BATTLER_COORD_X_2:
        retVal = sBattlerCoords[IS_DOUBLE_BATTLE()][GetBattlerPosition(battlerId)].x;
        break;
    case BATTLER_COORD_Y:
        retVal = sBattlerCoords[IS_DOUBLE_BATTLE()][GetBattlerPosition(battlerId)].y;
        break;
    case BATTLER_COORD_Y_PIC_OFFSET:
    case BATTLER_COORD_Y_PIC_OFFSET_DEFAULT:
    default:
        if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
            else
                species = spriteInfo[battlerId].transformSpecies;
        }
        else
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
            else
                species = spriteInfo[battlerId].transformSpecies;
        }
        if (coordType == BATTLER_COORD_Y_PIC_OFFSET)
            retVal = GetBattlerSpriteFinal_Y(battlerId, species, TRUE);
        else
            retVal = GetBattlerSpriteFinal_Y(battlerId, species, FALSE);
        break;
    }
    return retVal;
}

static u8 GetBattlerYDelta(u8 battlerId, u16 species)
{
    u16 letter;
    u32 personality;
    struct BattleSpriteInfo *spriteInfo;
    u8 ret;
    u16 coordSpecies;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
    {
        if (species == SPECIES_UNOWN)
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                personality = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
            else
                personality = gTransformedPersonalities[battlerId];
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                coordSpecies = species;
            else
                coordSpecies = letter + SPECIES_UNOWN_B - 1;
            ret = gMonBackPicCoords[coordSpecies].y_offset;
        }
        else if (species == SPECIES_CASTFORM)
        {
            ret = sCastformBackSpriteYCoords[gBattleMonForms[battlerId]];
        }
        else if (species > NUM_SPECIES)
        {
            ret = gMonBackPicCoords[0].y_offset;
        }
        else
        {
            ret = gMonBackPicCoords[species].y_offset;
        }
    }
    else
    {
        if (species == SPECIES_UNOWN)
        {
            spriteInfo = gBattleSpritesDataPtr->battlerData;
            if (!spriteInfo[battlerId].transformSpecies)
                personality = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
            else
                personality = gTransformedPersonalities[battlerId];
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                coordSpecies = species;
            else
                coordSpecies = letter + SPECIES_UNOWN_B - 1;
            ret = gMonFrontPicCoords[coordSpecies].y_offset;
        }
        else if (species == SPECIES_CASTFORM)
        {
            ret = gCastformFrontSpriteCoords[gBattleMonForms[battlerId]].y_offset;
        }
        else if (species > NUM_SPECIES)
        {
            ret = gMonFrontPicCoords[0].y_offset;
        }
        else
        {
            ret = gMonFrontPicCoords[species].y_offset;
        }
    }
    return ret;
}

static u8 GetBattlerElevation(u8 battlerId, u16 species)
{
    u8 ret = 0;

    if (GetBattlerSide(battlerId) == B_SIDE_OPPONENT)
    {
        if (species == SPECIES_CASTFORM)
            ret = sCastformElevations[gBattleMonForms[battlerId]];
        else if (species > NUM_SPECIES)
            ret = gEnemyMonElevation[0];
        else
            ret = gEnemyMonElevation[species];
    }
    return ret;
}

static u8 GetBattlerSpriteFinal_Y(u8 battlerId, u16 species, bool8 a3)
{
    u16 offset;
    u8 y;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
    {
        offset = GetBattlerYDelta(battlerId, species);
    }
    else
    {
        offset = GetBattlerYDelta(battlerId, species);
        offset -= GetBattlerElevation(battlerId, species);
    }
    y = offset + sBattlerCoords[IS_DOUBLE_BATTLE()][GetBattlerPosition(battlerId)].y;
    if (a3)
    {
        if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
            y += 8;
        if (y > DISPLAY_HEIGHT - MON_PIC_HEIGHT + 8)
            y = DISPLAY_HEIGHT - MON_PIC_HEIGHT + 8;
    }
    return y;
}

u8 GetBattlerSpriteCoord2(u8 battlerId, u8 coordType)
{
    u16 species;
    struct BattleSpriteInfo *spriteInfo;

    if (coordType == BATTLER_COORD_Y_PIC_OFFSET || coordType == BATTLER_COORD_Y_PIC_OFFSET_DEFAULT)
    {
        spriteInfo = gBattleSpritesDataPtr->battlerData;
        if (!spriteInfo[battlerId].transformSpecies)
            species = gAnimBattlerSpecies[battlerId];
        else
            species = spriteInfo[battlerId].transformSpecies;
        if (coordType == BATTLER_COORD_Y_PIC_OFFSET)
            return GetBattlerSpriteFinal_Y(battlerId, species, TRUE);
        else
            return GetBattlerSpriteFinal_Y(battlerId, species, FALSE);
    }
    else
    {
        return GetBattlerSpriteCoord(battlerId, coordType);
    }
}

u8 GetBattlerSpriteDefault_Y(u8 battlerId)
{
    return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y_PIC_OFFSET_DEFAULT);
}

u8 GetSubstituteSpriteDefault_Y(u8 battlerId)
{
    u16 y;

    if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        y = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y) + 16;
    else
        y = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y) + 17;
    return y;
}

u8 GetGhostSpriteDefault_Y(u8 battlerId)
{
    if (GetBattlerSide(battlerId) != B_SIDE_OPPONENT)
        return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y_PIC_OFFSET_DEFAULT);
    else
        return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y);
}

u8 GetBattlerYCoordWithElevation(u8 battlerId)
{
    u16 species;
    u8 y;
    struct BattleSpriteInfo *spriteInfo;

    y = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y);
    if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
    {
        spriteInfo = gBattleSpritesDataPtr->battlerData;
        if (!spriteInfo[battlerId].transformSpecies)
            species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
        else
            species = spriteInfo[battlerId].transformSpecies;
    }
    else
    {
        spriteInfo = gBattleSpritesDataPtr->battlerData;
        if (!spriteInfo[battlerId].transformSpecies)
            species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
        else
            species = spriteInfo[battlerId].transformSpecies;
    }
    if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
        y -= GetBattlerElevation(battlerId, species);
    return y;
}

u8 GetAnimBattlerSpriteId(u8 animBattler)
{
    u8 *sprites;

    if (animBattler == ANIM_ATTACKER)
    {
        if (IsBattlerSpritePresent(gBattleAnimAttacker))
        {
            sprites = gBattlerSpriteIds;
            return sprites[gBattleAnimAttacker];
        }
        else
        {
            return SPRITE_NONE;
        }
    }
    else if (animBattler == ANIM_TARGET)
    {
        if (IsBattlerSpritePresent(gBattleAnimTarget))
        {
            sprites = gBattlerSpriteIds;
            return sprites[gBattleAnimTarget];
        }
        else
        {
            return SPRITE_NONE;
        }
    }
    else if (animBattler == ANIM_ATK_PARTNER)
    {
        if (!IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
            return SPRITE_NONE;
        else
            return gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)];
    }
    else
    {
        if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimTarget)))
            return gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimTarget)];
        else
            return SPRITE_NONE;
    }
}

void StoreSpriteCallbackInData6(struct Sprite *sprite, SpriteCallback callback)
{
    sprite->data[6] = (u32)(callback) & 0xFFFF;
    sprite->data[7] = (u32)(callback) >> 16;
}

static void SetCallbackToStoredInData6(struct Sprite *sprite)
{
    u32 callback = (u16)sprite->data[6] | (sprite->data[7] << 16);
    
    sprite->callback = (SpriteCallback)callback;
}

// Sprite data for TranslateSpriteInCircle/Ellipse and related
#define sCirclePos    data[0]
#define sAmplitude    data[1]
#define sCircleSpeed  data[2]
#define sDuration     data[3]

// TranslateSpriteInGrowingCircle
#define sAmplitudeSpeed  data[4]
#define sAmplitudeChange data[5]

// TranslateSpriteInEllipse
#define sAmplitudeX sAmplitude
#define sAmplitudeY data[4]

// TranslateSpriteInLissajousCurve
#define sCirclePosX   sCirclePos
#define sCircleSpeedX sCircleSpeed
#define sCirclePosY   data[4]
#define sCircleSpeedY data[5]

// x = a * sin(theta0 + dtheta * t)
// y = a * cos(theta0 + dtheta * t)
void TranslateSpriteInCircle(struct Sprite *sprite)
{
    if (sprite->sDuration)
    {
        sprite->x2 = Sin(sprite->sCirclePos, sprite->sAmplitude);
        sprite->y2 = Cos(sprite->sCirclePos, sprite->sAmplitude);
        sprite->sCirclePos += sprite->sCircleSpeed;
        if (sprite->sCirclePos >= 0x100)
            sprite->sCirclePos -= 0x100;
        else if (sprite->sCirclePos < 0)
            sprite->sCirclePos += 0x100;
        sprite->sDuration--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

// x = (a0 + da * t) * sin(theta0 + dtheta * t)
// y = (a0 + da * t) * cos(theta0 + dtheta * t)
void TranslateSpriteInGrowingCircle(struct Sprite *sprite)
{
    if (sprite->sDuration)
    {
        sprite->x2 = Sin(sprite->sCirclePos, (sprite->sAmplitudeChange >> 8) + sprite->sAmplitude);
        sprite->y2 = Cos(sprite->sCirclePos, (sprite->sAmplitudeChange >> 8) + sprite->sAmplitude);
        sprite->sCirclePos += sprite->sCircleSpeed;
        sprite->sAmplitudeChange += sprite->sAmplitudeSpeed;
        if (sprite->sCirclePos >= 0x100)
            sprite->sCirclePos -= 0x100;
        else if (sprite->sCirclePos < 0)
            sprite->sCirclePos += 0x100;
        sprite->sDuration--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

// Unused
// Exact shape depends on arguments. Can move in a figure-8-like pattern, or circular, etc.
// x = alpl * sin(alpha0 + dalpha * t)
// y = ampl * cos(beta0 + dbeta * t)
static void TranslateSpriteInLissajousCurve(struct Sprite *sprite)
{
    if (sprite->sDuration)
    {
        sprite->x2 = Sin(sprite->sCirclePosX, sprite->sAmplitude);
        sprite->y2 = Cos(sprite->sCirclePosY, sprite->sAmplitude);
        sprite->sCirclePosX += sprite->sCircleSpeedX;
        sprite->sCirclePosY += sprite->sCircleSpeedY;
        
        if (sprite->sCirclePosX >= 0x100)
            sprite->sCirclePosX -= 0x100;
        else if (sprite->sCirclePosX < 0)
            sprite->sCirclePosX += 0x100;

        if (sprite->sCirclePosY >= 0x100)
            sprite->sCirclePosY -= 0x100;
        else if (sprite->sCirclePosY < 0)
            sprite->sCirclePosY += 0x100;

        sprite->sDuration--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

// x = a * sin(theta0 + dtheta * t)
// y = b * cos(theta0 + dtheta * t)
void TranslateSpriteInEllipse(struct Sprite *sprite)
{
    if (sprite->sDuration)
    {
        sprite->x2 = Sin(sprite->sCirclePos, sprite->sAmplitudeX);
        sprite->y2 = Cos(sprite->sCirclePos, sprite->sAmplitudeY);
        sprite->sCirclePos += sprite->sCircleSpeed;
        if (sprite->sCirclePos >= 0x100)
            sprite->sCirclePos -= 0x100;
        else if (sprite->sCirclePos < 0)
            sprite->sCirclePos += 0x100;
        sprite->sDuration--;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

#undef sCirclePos
#undef sAmplitude
#undef sCircleSpeed
#undef sDuration
#undef sAmplitudeSpeed
#undef sAmplitudeChange
#undef sAmplitudeX
#undef sAmplitudeY
#undef sCirclePosX
#undef sCircleSpeedX
#undef sCirclePosY
#undef sCircleSpeedY

// Simply waits until the sprite's data[0] hits zero.
// This is used to let sprite anims or affine anims to run for a designated
// duration.
void WaitAnimForDuration(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
        --sprite->data[0];
    else
        SetCallbackToStoredInData6(sprite);
}

// Sprite data for ConvertPosDataToTranslateLinearData
#define sStepsX  data[0]
#define sStartX  data[1]
#define sTargetX data[2]
#define sStartY  data[3]
#define sTargetY data[4]

// Sprite data for TranslateSpriteLinear
#define sMoveSteps data[0]
#define sSpeedX    data[1]
#define sSpeedY    data[2]

static void AnimPosToTranslateLinear(struct Sprite *sprite)
{
    ConvertPosDataToTranslateLinearData(sprite);
    sprite->callback = TranslateSpriteLinear;
    sprite->callback(sprite);
}

void ConvertPosDataToTranslateLinearData(struct Sprite *sprite)
{
    s16 old;
    int xDiff;

    if (sprite->sStartX > sprite->sTargetX)
        sprite->sStepsX = -sprite->sStepsX;
    xDiff = sprite->sTargetX - sprite->sStartX;
    old = sprite->sStepsX;
    sprite->sMoveSteps = abs(xDiff / sprite->sStepsX);
    sprite->sSpeedY = (sprite->sTargetY - sprite->sStartY) / sprite->sMoveSteps;
    sprite->sSpeedX = old;
}

void TranslateSpriteLinear(struct Sprite *sprite)
{
    if (sprite->sMoveSteps > 0)
    {
        sprite->sMoveSteps--;
        sprite->x2 += sprite->sSpeedX;
        sprite->y2 += sprite->sSpeedY;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void TranslateSpriteLinearFixedPoint(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        --sprite->data[0];
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        sprite->x2 = sprite->data[3] >> 8;
        sprite->y2 = sprite->data[4] >> 8;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

static void TranslateSpriteLinearFixedPointIconFrame(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        --sprite->data[0];
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        sprite->x2 = sprite->data[3] >> 8;
        sprite->y2 = sprite->data[4] >> 8;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }

    UpdateMonIconFrame(sprite);
}

// Unused
static void TranslateSpriteToBattleTargetPos(struct Sprite *sprite)
{
    sprite->data[1] = sprite->x + sprite->x2;
    sprite->data[3] = sprite->y + sprite->y2;
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2);
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET);
    sprite->callback = AnimPosToTranslateLinear;
}

// Same as TranslateSpriteLinear but takes an id to specify which sprite to move
void TranslateSpriteLinearById(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        --sprite->data[0];
        gSprites[sprite->data[3]].x2 += sprite->data[1];
        gSprites[sprite->data[3]].y2 += sprite->data[2];
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void TranslateSpriteLinearByIdFixedPoint(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        --sprite->data[0];
        sprite->data[3] += sprite->data[1];
        sprite->data[4] += sprite->data[2];
        gSprites[sprite->data[5]].x2 = sprite->data[3] >> 8;
        gSprites[sprite->data[5]].y2 = sprite->data[4] >> 8;
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void TranslateSpriteLinearAndFlicker(struct Sprite *sprite)
{
    if (sprite->data[0] > 0)
    {
        --sprite->data[0];
        sprite->x2 = sprite->data[2] >> 8;
        sprite->data[2] += sprite->data[1];
        sprite->y2 = sprite->data[4] >> 8;
        sprite->data[4] += sprite->data[3];
        if (sprite->data[0] % sprite->data[5] == 0)
        {
            if (sprite->data[5])
                sprite->invisible ^= 1;
        }
    }
    else
    {
        SetCallbackToStoredInData6(sprite);
    }
}

void DestroySpriteAndMatrix(struct Sprite *sprite)
{
    FreeSpriteOamMatrix(sprite);
    DestroyAnimSprite(sprite);
}

// Unused
static void SetupAndStartSpriteLinearTranslationToAttacker(struct Sprite *sprite)
{
    sprite->sStartX = sprite->x + sprite->x2;
    sprite->sStartY = sprite->y + sprite->y2;
    sprite->sTargetX = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->sTargetY = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
    sprite->callback = AnimPosToTranslateLinear;
}

#undef sStepsX
#undef sStartX
#undef sTargetX
#undef sStartY
#undef sTargetY

// Unused
static void EndUnkPaletteAnim(struct Sprite *sprite)
{
    PaletteStruct_ResetById(sprite->data[5]);
    DestroySpriteAndMatrix(sprite);
}

void RunStoredCallbackWhenAffineAnimEnds(struct Sprite *sprite)
{
    if (sprite->affineAnimEnded)
        SetCallbackToStoredInData6(sprite);
}

void RunStoredCallbackWhenAnimEnds(struct Sprite *sprite)
{
    if (sprite->animEnded)
        SetCallbackToStoredInData6(sprite);
}

void DestroyAnimSpriteAndDisableBlend(struct Sprite *sprite)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DestroyAnimSprite(sprite);
}

void DestroyAnimVisualTaskAndDisableBlend(u8 taskId)
{
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DestroyAnimVisualTask(taskId);
}

void SetSpriteCoordsToAnimAttackerCoords(struct Sprite *sprite)
{
    sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
}

// Sets the initial x offset of the anim sprite depending on the horizontal orientation
// of the two involved mons.
void SetAnimSpriteInitialXOffset(struct Sprite *sprite, s16 xOffset)
{
    u16 attackerX = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X);
    u16 targetX = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X);

    if (attackerX > targetX)
    {
        sprite->x -= xOffset;
    }
    else if (attackerX < targetX)
    {
        sprite->x += xOffset;
    }
    else
    {
        if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
            sprite->x -= xOffset;
        else
            sprite->x += xOffset;
    }
}

void InitAnimArcTranslation(struct Sprite *sprite)
{
    sprite->sTransl_InitX = sprite->x;
    sprite->sTransl_InitY = sprite->y;
    InitAnimLinearTranslation(sprite);
    sprite->data[6] = 0x8000 / sprite->sTransl_Speed;
    sprite->data[7] = 0;
}

bool8 TranslateAnimHorizontalArc(struct Sprite *sprite)
{
    if (AnimTranslateLinear(sprite))
        return TRUE;
    sprite->data[7] += sprite->data[6];
    sprite->y2 += Sin((u8)(sprite->data[7] >> 8), sprite->sTransl_ArcAmpl);
    return FALSE;
}

bool8 TranslateAnimVerticalArc(struct Sprite *sprite)
{
    if (AnimTranslateLinear(sprite))
        return TRUE;
    sprite->data[7] += sprite->data[6];
    sprite->x2 += Sin((u8)(sprite->data[7] >> 8), sprite->sTransl_ArcAmpl);
    return FALSE;
}

void SetSpritePrimaryCoordsFromSecondaryCoords(struct Sprite *sprite)
{
    sprite->x += sprite->x2;
    sprite->y += sprite->y2;
    sprite->x2 = 0;
    sprite->y2 = 0;
}

void InitSpritePosToAnimTarget(struct Sprite *sprite, bool8 respectMonPicOffsets)
{
    // Battle anim sprites are automatically created at the anim target's center, which
    // is why there is no else clause for the "respectMonPicOffsets" check.
    if (!respectMonPicOffsets)
    {
        sprite->x = GetBattlerSpriteCoord2(gBattleAnimTarget, BATTLER_COORD_X);
        sprite->y = GetBattlerSpriteCoord2(gBattleAnimTarget, BATTLER_COORD_Y);
    }
    SetAnimSpriteInitialXOffset(sprite, gBattleAnimArgs[0]);
    sprite->y += gBattleAnimArgs[1];
}

void InitSpritePosToAnimAttacker(struct Sprite *sprite, bool8 respectMonPicOffsets)
{
    if (!respectMonPicOffsets)
    {
        sprite->x = GetBattlerSpriteCoord2(gBattleAnimAttacker, BATTLER_COORD_X);
        sprite->y = GetBattlerSpriteCoord2(gBattleAnimAttacker, BATTLER_COORD_Y);
    }
    else
    {
        sprite->x = GetBattlerSpriteCoord2(gBattleAnimAttacker, BATTLER_COORD_X_2);
        sprite->y = GetBattlerSpriteCoord2(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
    }
    SetAnimSpriteInitialXOffset(sprite, gBattleAnimArgs[0]);
    sprite->y += gBattleAnimArgs[1];
}

u8 GetBattlerSide(u8 battlerId)
{
    return GET_BATTLER_SIDE2(battlerId);
}

u8 GetBattlerPosition(u8 battlerId)
{
    return GET_BATTLER_POSITION(battlerId);
}

u8 GetBattlerAtPosition(u8 position)
{
    u8 i;

    for (i = 0; i < gBattlersCount; ++i)
        if (gBattlerPositions[i] == position)
            break;
    return i;
}

bool8 IsBattlerSpritePresent(u8 battlerId)
{
    if (gBattlerPositions[battlerId] == 0xFF)
    {
        return FALSE;
    }
    else if (GetBattlerSide(battlerId) != B_SIDE_PLAYER)
    {
        if (GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_HP) != 0)
            return TRUE;
    }
    else
    {
        if (GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_HP) != 0)
            return TRUE;
    }
    return FALSE;
}

bool8 IsDoubleBattle(void)
{
    return IS_DOUBLE_BATTLE();
}

#define BG_ANIM_PAL_1        8
#define BG_ANIM_PAL_2        9

void GetBattleAnimBg1Data(struct BattleAnimBgData *animBgData)
{
    animBgData->bgTiles = gBattleAnimBgTileBuffer;
    animBgData->bgTilemap = (u16 *)gBattleAnimBgTilemapBuffer;
    animBgData->paletteId = BG_ANIM_PAL_1;
    animBgData->bgId = 1;
    animBgData->tilesOffset = 0x200;
    animBgData->unused = 0;
}

void GetBattleAnimBgData(struct BattleAnimBgData *animBgData, u32 bgId)
{
    if (bgId == 1)
    {
        GetBattleAnimBg1Data(animBgData);
    }
    else
    {
        animBgData->bgTiles = gBattleAnimBgTileBuffer;
        animBgData->bgTilemap = (u16 *)gBattleAnimBgTilemapBuffer;
        animBgData->paletteId = BG_ANIM_PAL_2;
        animBgData->bgId = 2;
        animBgData->tilesOffset = 0x300;
        animBgData->unused = 0;
    }
}

void GetBattleAnimBgDataByPriorityRank(struct BattleAnimBgData *animBgData, u8 unused)
{
    animBgData->bgTiles = gBattleAnimBgTileBuffer;
    animBgData->bgTilemap = (u16 *)gBattleAnimBgTilemapBuffer;
    if (GetBattlerSpriteBGPriorityRank(gBattleAnimAttacker) == 1)
    {
        animBgData->paletteId = BG_ANIM_PAL_1;
        animBgData->bgId = 1;
        animBgData->tilesOffset = 0x200;
        animBgData->unused = 0;
    }
    else
    {
        animBgData->paletteId = BG_ANIM_PAL_2;
        animBgData->bgId = 2;
        animBgData->tilesOffset = 0x300;
        animBgData->unused = 0;
    }
}

void InitBattleAnimBg(u32 bgId)
{
    struct BattleAnimBgData animBgData;

    GetBattleAnimBgData(&animBgData, bgId);
    CpuFill32(0, animBgData.bgTiles, 0x2000);
    LoadBgTiles(bgId, animBgData.bgTiles, 0x2000, animBgData.tilesOffset);
    FillBgTilemapBufferRect(bgId, 0, 0, 0, 32, 64, 17);
    CopyBgTilemapBufferToVram(bgId);
}

void AnimLoadCompressedBgGfx(u32 bgId, const u32 *src, u32 tilesOffset)
{
    CpuFill32(0, gBattleAnimBgTileBuffer, 0x2000);
    LZDecompressWram(src, gBattleAnimBgTileBuffer);
    LoadBgTiles(bgId, gBattleAnimBgTileBuffer, 0x2000, tilesOffset);
}

void InitAnimBgTilemapBuffer(u32 bgId, const void *src)
{
    FillBgTilemapBufferRect(bgId, 0, 0, 0, 32, 64, 17);
    CopyToBgTilemapBuffer(bgId, src, 0, 0);
}

void AnimLoadCompressedBgTilemap(u32 bgId, const u32 *src)
{
    InitAnimBgTilemapBuffer(bgId, src);
    CopyBgTilemapBufferToVram(bgId);
}

u8 GetBattleBgPaletteNum(void)
{
    /*
    if (IsContest())
        return 1;
    */
    return 2;
}

void ToggleBg3Mode(bool8 largeScreenSize)
{
    if (!largeScreenSize)
    {
        SetAnimBgAttribute(3, BG_ANIM_SCREEN_SIZE, 0);
        SetAnimBgAttribute(3, BG_ANIM_AREA_OVERFLOW_MODE, 1);
    }
    else
    {
        SetAnimBgAttribute(3, BG_ANIM_SCREEN_SIZE, 1);
        SetAnimBgAttribute(3, BG_ANIM_AREA_OVERFLOW_MODE, 0);
    }
}

void Trade_MoveSelectedMonToTarget(struct Sprite *sprite)
{
    sprite->data[1] = sprite->x;
    sprite->data[3] = sprite->y;
    InitSpriteDataForLinearTranslation(sprite);
    sprite->callback = TranslateSpriteLinearFixedPointIconFrame;
    sprite->callback(sprite);
}

void InitSpriteDataForLinearTranslation(struct Sprite *sprite)
{
    s16 x = (sprite->data[2] - sprite->data[1]) << 8;
    s16 y = (sprite->data[4] - sprite->data[3]) << 8;

    sprite->data[1] = x / sprite->data[0];
    sprite->data[2] = y / sprite->data[0];
    sprite->data[4] = 0;
    sprite->data[3] = 0;
}

void InitAnimLinearTranslation(struct Sprite *sprite)
{
    s32 x = sprite->sTransl_DestX - sprite->sTransl_InitX;
    s32 y = sprite->sTransl_DestY - sprite->sTransl_InitY;
    bool8 movingLeft = x < 0;
    bool8 movingUp = y < 0;
    u16 xDelta = abs(x) << 8;
    u16 yDelta = abs(y) << 8;

    xDelta = xDelta / sprite->sTransl_Speed;
    yDelta = yDelta / sprite->sTransl_Speed;

    if (movingLeft)
        xDelta |= 1;
    else
        xDelta &= ~1;

    if (movingUp)
        yDelta |= 1;
    else
        yDelta &= ~1;

    sprite->data[1] = xDelta;
    sprite->data[2] = yDelta;
    sprite->data[4] = 0;
    sprite->data[3] = 0;
}

void StartAnimLinearTranslation(struct Sprite *sprite)
{
    sprite->sTransl_InitX = sprite->x;
    sprite->sTransl_InitY = sprite->y;
    InitAnimLinearTranslation(sprite);
    sprite->callback = AnimTranslateLinear_WithFollowup;
    sprite->callback(sprite);
}

void PlayerThrowBall_StartAnimLinearTranslation(struct Sprite *sprite)
{
    sprite->sTransl_InitX = sprite->x;
    sprite->sTransl_InitY = sprite->y;
    InitAnimLinearTranslation(sprite);
    sprite->callback = PlayerThrowBall_AnimTranslateLinear_WithFollowup;
    sprite->callback(sprite);
}

bool8 AnimTranslateLinear(struct Sprite *sprite)
{
    u16 v1, v2, x, y;

    if (!sprite->data[0])
        return TRUE;
    v1 = sprite->data[1];
    v2 = sprite->data[2];
    x = sprite->data[3];
    y = sprite->data[4];
    x += v1;
    y += v2;
    if (v1 & 1)
        sprite->x2 = -(x >> 8);
    else
        sprite->x2 = x >> 8;

    if (v2 & 1)
        sprite->y2 = -(y >> 8);
    else
        sprite->y2 = y >> 8;
    sprite->data[3] = x;
    sprite->data[4] = y;
    --sprite->data[0];
    return FALSE;
}

void AnimTranslateLinear_WithFollowup(struct Sprite *sprite)
{
    if (AnimTranslateLinear(sprite))
        SetCallbackToStoredInData6(sprite);
}

static void PlayerThrowBall_AnimTranslateLinear_WithFollowup(struct Sprite *sprite)
{
    UpdatePlayerPosInThrowAnim(sprite);
    if (AnimTranslateLinear(sprite))
        SetCallbackToStoredInData6(sprite);
}

void InitAnimLinearTranslationWithSpeed(struct Sprite *sprite)
{
    s32 v1 = abs(sprite->sTransl_DestX - sprite->sTransl_InitX) << 8;

    sprite->sTransl_Speed = v1 / sprite->sTransl_Duration;
    InitAnimLinearTranslation(sprite);
}

void InitAnimLinearTranslationWithSpeedAndPos(struct Sprite *sprite)
{
    sprite->sTransl_InitX = sprite->x;
    sprite->sTransl_InitY = sprite->y;
    InitAnimLinearTranslationWithSpeed(sprite);
    sprite->callback = AnimTranslateLinear_WithFollowup;
    sprite->callback(sprite);
}

static void InitAnimFastLinearTranslation(struct Sprite *sprite)
{
    s32 xDiff = sprite->sTransl_DestX - sprite->sTransl_InitX;
    s32 yDiff = sprite->sTransl_DestY - sprite->sTransl_InitY;
    bool8 xSign = xDiff < 0;
    bool8 ySign = yDiff < 0;
    u16 x2 = abs(xDiff) << 4;
    u16 y2 = abs(yDiff) << 4;

    x2 /= sprite->sTransl_Duration;
    y2 /= sprite->sTransl_Duration;
    if (xSign)
        x2 |= 1;
    else
        x2 &= ~1;
    if (ySign)
        y2 |= 1;
    else
        y2 &= ~1;
    sprite->data[1] = x2;
    sprite->data[2] = y2;
    sprite->data[4] = 0;
    sprite->data[3] = 0;
}

void InitAndRunAnimFastLinearTranslation(struct Sprite *sprite)
{
    sprite->sTransl_InitX = sprite->x;
    sprite->sTransl_InitY = sprite->y;
    InitAnimFastLinearTranslation(sprite);
    sprite->callback = AnimFastTranslateLinearWaitEnd;
    sprite->callback(sprite);
}

bool8 AnimFastTranslateLinear(struct Sprite *sprite)
{
    u16 v1, v2, x, y;

    if (!sprite->data[0])
        return TRUE;
    v1 = sprite->data[1];
    v2 = sprite->data[2];
    x = sprite->data[3];
    y = sprite->data[4];
    x += v1;
    y += v2;
    if (v1 & 1)
        sprite->x2 = -(x >> 4);
    else
        sprite->x2 = x >> 4;
    if (v2 & 1)
        sprite->y2 = -(y >> 4);
    else
        sprite->y2 = y >> 4;
    sprite->data[3] = x;
    sprite->data[4] = y;
    --sprite->data[0];
    return FALSE;
}

static void AnimFastTranslateLinearWaitEnd(struct Sprite *sprite)
{
    if (AnimFastTranslateLinear(sprite))
        SetCallbackToStoredInData6(sprite);
}

void InitAnimFastLinearTranslationWithSpeed(struct Sprite *sprite)
{
    s32 xDiff = abs(sprite->data[2] - sprite->data[1]) << 4;

    sprite->data[0] = xDiff / sprite->data[0];
    InitAnimFastLinearTranslation(sprite);
}

void InitAnimFastLinearTranslationWithSpeedAndPos(struct Sprite *sprite)
{
    sprite->data[1] = sprite->x;
    sprite->data[3] = sprite->y;
    InitAnimFastLinearTranslationWithSpeed(sprite);
    sprite->callback = AnimFastTranslateLinearWaitEnd;
    sprite->callback(sprite);
}

void SetSpriteRotScale(u8 spriteId, s16 xScale, s16 yScale, u16 rotation)
{
    s32 i;
    struct ObjAffineSrcData src;
    struct OamMatrix matrix;

    src.xScale = xScale;
    src.yScale = yScale;
    src.rotation = rotation;
    if (ShouldRotScaleSpeciesBeFlipped())
        src.xScale = -src.xScale;
    i = gSprites[spriteId].oam.matrixNum;
    ObjAffineSet(&src, &matrix, 1, 2);
    gOamMatrices[i].a = matrix.a;
    gOamMatrices[i].b = matrix.b;
    gOamMatrices[i].c = matrix.c;
    gOamMatrices[i].d = matrix.d;
}

// Pokémon in Contests (except Unown) should be flipped.
static bool8 ShouldRotScaleSpeciesBeFlipped(void)
{
    /*
    if (IsContest())
    {
        if (gSprites[GetAnimBattlerSpriteId(ANIM_ATTACKER)].data[2] == SPECIES_UNOWN)
            return FALSE;
        else
            return TRUE;
    }
    */
    return FALSE;
}

void PrepareBattlerSpriteForRotScale(u8 spriteId, u8 objMode)
{
    u8 battlerId = gSprites[spriteId].data[0];

    if (IsBattlerSpriteVisible(battlerId))
        gSprites[spriteId].invisible = FALSE;
    gSprites[spriteId].oam.objMode = objMode;
    gSprites[spriteId].affineAnimPaused = TRUE;
    if (!gSprites[spriteId].oam.affineMode)
        gSprites[spriteId].oam.matrixNum = gBattleSpritesDataPtr->healthBoxesData[battlerId].matrixNum;
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    CalcCenterToCornerVec(&gSprites[spriteId], gSprites[spriteId].oam.shape, gSprites[spriteId].oam.size, gSprites[spriteId].oam.affineMode);
}

void ResetSpriteRotScale(u8 spriteId)
{
    SetSpriteRotScale(spriteId, 0x100, 0x100, 0);
    gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_NORMAL;
    gSprites[spriteId].oam.objMode = 0;
    gSprites[spriteId].affineAnimPaused = FALSE;
    CalcCenterToCornerVec(&gSprites[spriteId], gSprites[spriteId].oam.shape, gSprites[spriteId].oam.size, gSprites[spriteId].oam.affineMode);
}

// Sets the sprite's y offset equal to the y displacement caused by the
// matrix's rotation.
void SetBattlerSpriteYOffsetFromRotation(u8 spriteId)
{
    u16 matrixNum = gSprites[spriteId].oam.matrixNum;
    // The "c" component of the battler sprite matrix contains the sine of the rotation angle divided by some scale amount.
    s16 c = gOamMatrices[matrixNum].c;

    if (c < 0)
        c = -c;
    gSprites[spriteId].y2 = c >> 3;
}

void TrySetSpriteRotScale(struct Sprite *sprite, bool8 recalcCenterVector, s16 xScale, s16 yScale, u16 rotation)
{
    s32 i;
    struct ObjAffineSrcData src;
    struct OamMatrix matrix;

    if (sprite->oam.affineMode & 1)
    {
        sprite->affineAnimPaused = TRUE;
        if (recalcCenterVector)
            CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
        src.xScale = xScale;
        src.yScale = yScale;
        src.rotation = rotation;
        if (ShouldRotScaleSpeciesBeFlipped())
            src.xScale = -src.xScale;
        i = sprite->oam.matrixNum;
        ObjAffineSet(&src, &matrix, 1, 2);
        gOamMatrices[i].a = matrix.a;
        gOamMatrices[i].b = matrix.b;
        gOamMatrices[i].c = matrix.c;
        gOamMatrices[i].d = matrix.d;
    }
}

void TryResetSpriteAffineState(struct Sprite *sprite)
{
    TrySetSpriteRotScale(sprite, TRUE, 0x100, 0x100, 0);
    sprite->affineAnimPaused = FALSE;
    CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
}

static u16 ArcTan2_(s16 a, s16 b)
{
    return ArcTan2(a, b);
}

u16 ArcTan2Neg(s16 a, s16 b)
{
    u16 var = ArcTan2_(a, b);
    return -var;
}

void SetGreyscaleOrOriginalPalette(u16 paletteNum, bool8 restoreOriginalColor)
{
    s32 i;
    struct PlttData *originalColor;
    struct PlttData *destColor;
    u16 average;

    paletteNum = PLTT_ID(paletteNum);

    if (!restoreOriginalColor)
    {
        for (i = 0; i < 16; ++i)
        {
            originalColor = (struct PlttData *)&gPlttBufferUnfaded[paletteNum + i];
            average = originalColor->r + originalColor->g + originalColor->b;
            average /= 3;
            destColor = (struct PlttData *)&gPlttBufferFaded[paletteNum + i];
            destColor->r = average;
            destColor->g = average;
            destColor->b = average;
        }
    }
    else
    {
        CpuCopy32(&gPlttBufferUnfaded[paletteNum], &gPlttBufferFaded[paletteNum], PLTT_SIZE_4BPP);
    }
}

u32 GetBattlePalettesMask(bool8 battleBackground, bool8 attacker, bool8 target, bool8 attackerPartner, bool8 targetPartner, bool8 anim1, bool8 anim2)
{
    u32 selectedPalettes = 0;
    u32 shift;

    if (battleBackground)
    {
        selectedPalettes = 0xe; // Palettes 1, 2, and 3
    }
    if (attacker)
    {
        shift = gBattleAnimAttacker + 16;
        selectedPalettes |= 1 << shift;
    }
    if (target)
    {
        shift = gBattleAnimTarget + 16;
        selectedPalettes |= 1 << shift;
    }
    if (attackerPartner)
    {
        if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
        {
            shift = BATTLE_PARTNER(gBattleAnimAttacker) + 16;
            selectedPalettes |= 1 << shift;
        }
    }
    if (targetPartner)
    {
        if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimTarget)))
        {
            shift = BATTLE_PARTNER(gBattleAnimTarget) + 16;
            selectedPalettes |= 1 << shift;
        }
    }
    if (anim1)
        selectedPalettes |= 1 << BG_ANIM_PAL_1;

    if (anim2)
        selectedPalettes |= 1 << BG_ANIM_PAL_2;

    return selectedPalettes;
}

u32 GetBattleMonSpritePalettesMask(bool8 playerLeft, bool8 playerRight, bool8 foeLeft, bool8 foeRight)
{
    u32 selectedPalettes = 0;
    u32 shift;

    if (playerLeft)
    {
        if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_PLAYER_LEFT)))
        {
            selectedPalettes |= 1 << (GetBattlerAtPosition(B_POSITION_PLAYER_LEFT) + 16);
        }
    }
    if (playerRight)
    {
        if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT)))
        {
            shift = GetBattlerAtPosition(B_POSITION_PLAYER_RIGHT) + 16;
            selectedPalettes |= 1 << shift;
        }
    }
    if (foeLeft)
    {
        if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT)))
        {
            shift = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT) + 16;
            selectedPalettes |= 1 << shift;
        }
    }
    if (foeRight)
    {
        if (IsBattlerSpriteVisible(GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT)))
        {
            shift = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT) + 16;
            selectedPalettes |= 1 << shift;
        }
    }
    return selectedPalettes;
}

u8 GetSpritePalIdxByBattler(u8 battler)
{
    return battler;
}

// Unused
static u8 GetSpritePalIdxByPosition(u8 position)
{
    return GetBattlerAtPosition(position);
}

void AnimSpriteOnMonPos(struct Sprite *sprite)
{
    bool8 var;

    if (!sprite->data[0])
    {
        if (!gBattleAnimArgs[3])
            var = TRUE;
        else
            var = FALSE;
        if (!gBattleAnimArgs[2])
            InitSpritePosToAnimAttacker(sprite, var);
        else
            InitSpritePosToAnimTarget(sprite, var);
        ++sprite->data[0];

    }
    else if (sprite->animEnded || sprite->affineAnimEnded)
    {
        DestroySpriteAndMatrix(sprite);
    }
}

// Linearly translates a sprite to a target position on the
// other mon's sprite.
// arg 0: initial x offset
// arg 1: initial y offset
// arg 2: target x offset
// arg 3: target y offset
// arg 4: duration
// arg 5: lower 8 bits = location on attacking mon, upper 8 bits = location on target mon pick to target
void TranslateAnimSpriteToTargetMonLocation(struct Sprite *sprite)
{
    bool8 respectMonPicOffsets;
    u8 coordType;

    if (!(gBattleAnimArgs[5] & 0xFF00))
        respectMonPicOffsets = TRUE;
    else
        respectMonPicOffsets = FALSE;
    if (!(gBattleAnimArgs[5] & 0xFF))
        coordType = BATTLER_COORD_Y_PIC_OFFSET;
    else
        coordType = BATTLER_COORD_Y;
    InitSpritePosToAnimAttacker(sprite, respectMonPicOffsets);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
        gBattleAnimArgs[2] = -gBattleAnimArgs[2];
    sprite->data[0] = gBattleAnimArgs[4];
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2) + gBattleAnimArgs[2];
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, coordType) + gBattleAnimArgs[3];
    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

void AnimThrowProjectile(struct Sprite *sprite)
{
    InitSpritePosToAnimAttacker(sprite, 1);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
        gBattleAnimArgs[2] = -gBattleAnimArgs[2];
    sprite->data[0] = gBattleAnimArgs[4];
    sprite->data[2] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_X_2) + gBattleAnimArgs[2];
    sprite->data[4] = GetBattlerSpriteCoord(gBattleAnimTarget, BATTLER_COORD_Y_PIC_OFFSET) + gBattleAnimArgs[3];
    sprite->data[5] = gBattleAnimArgs[5];
    InitAnimArcTranslation(sprite);
    sprite->callback = AnimThrowProjectile_Step;
}

static void AnimThrowProjectile_Step(struct Sprite *sprite)
{
    if (TranslateAnimHorizontalArc(sprite))
        DestroyAnimSprite(sprite);
}

void AnimTravelDiagonally(struct Sprite *sprite)
{
    bool8 r4;
    u8 battlerId, coordType;

    if (!gBattleAnimArgs[6])
    {
        r4 = TRUE;
        coordType = BATTLER_COORD_Y_PIC_OFFSET;
    }
    else
    {
        r4 = FALSE;
        coordType = BATTLER_COORD_Y;
    }
    if (!gBattleAnimArgs[5])
    {
        InitSpritePosToAnimAttacker(sprite, r4);
        battlerId = gBattleAnimAttacker;
    }
    else
    {
        InitSpritePosToAnimTarget(sprite, r4);
        battlerId = gBattleAnimTarget;
    }
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
        gBattleAnimArgs[2] = -gBattleAnimArgs[2];
    InitSpritePosToAnimTarget(sprite, r4);
    sprite->data[0] = gBattleAnimArgs[4];
    sprite->data[2] = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_X_2) + gBattleAnimArgs[2];
    sprite->data[4] = GetBattlerSpriteCoord(battlerId, coordType) + gBattleAnimArgs[3];
    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

s16 CloneBattlerSpriteWithBlend(u8 animBattler)
{
    u16 i;
    u8 spriteId = GetAnimBattlerSpriteId(animBattler);

    if (spriteId != SPRITE_NONE)
    {
        for (i = 0; i < MAX_SPRITES; ++i)
        {
            if (!gSprites[i].inUse)
            {
                gSprites[i] = gSprites[spriteId];
                gSprites[i].oam.objMode = ST_OAM_OBJ_BLEND;
                gSprites[i].invisible = FALSE;
                return i;
            }
        }
    }
    return -1;
}

void DestroySpriteWithActiveSheet(struct Sprite *sprite)
{
    sprite->usingSheet = TRUE;
    DestroySprite(sprite);
}

// Only used to fade Moonlight moon sprite in
void AnimTask_AlphaFadeIn(u8 taskId)
{
    s16 v1 = 0, v2 = 0;

    if (gBattleAnimArgs[2] > gBattleAnimArgs[0])
        v2 = 1;
    if (gBattleAnimArgs[2] < gBattleAnimArgs[0])
        v2 = -1;
    if (gBattleAnimArgs[3] > gBattleAnimArgs[1])
        v1 = 1;
    if (gBattleAnimArgs[3] < gBattleAnimArgs[1])
        v1 = -1;
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = gBattleAnimArgs[4];
    gTasks[taskId].data[2] = 0;
    gTasks[taskId].data[3] = gBattleAnimArgs[0];
    gTasks[taskId].data[4] = gBattleAnimArgs[1];
    gTasks[taskId].data[5] = v2;
    gTasks[taskId].data[6] = v1;
    gTasks[taskId].data[7] = gBattleAnimArgs[2];
    gTasks[taskId].data[8] = gBattleAnimArgs[3];
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gBattleAnimArgs[0], gBattleAnimArgs[1]));
    gTasks[taskId].func = AnimTask_AlphaFadeIn_Step;
}

static void AnimTask_AlphaFadeIn_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (++task->data[0] > task->data[1])
    {
        task->data[0] = 0;
        if (++task->data[2] & 1)
        {
            if (task->data[3] != task->data[7])
                task->data[3] += task->data[5];
        }
        else
        {
            if (task->data[4] != task->data[8])
                task->data[4] += task->data[6];
        }
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(task->data[3], task->data[4]));
        if (task->data[3] == task->data[7] && task->data[4] == task->data[8])
        {
            DestroyAnimVisualTask(taskId);
            return;
        }
    }
}

// Linearly blends a mon's sprite colors with a target color with increasing
// strength, and then blends out to the original color.
// arg 0: anim bank
// arg 1: blend color
// arg 2: target blend coefficient
// arg 3: initial delay
// arg 4: number of times to blend in and out
void AnimTask_BlendMonInAndOut(u8 task)
{
    u8 spriteId = GetAnimBattlerSpriteId(gBattleAnimArgs[0]);

    if (spriteId == 0xFF)
    {
        DestroyAnimVisualTask(task);
        return;
    }
    gTasks[task].data[0] = OBJ_PLTT_ID(gSprites[spriteId].oam.paletteNum) + 1;
    AnimTask_BlendMonInAndOutSetup(&gTasks[task]);
}

static void AnimTask_BlendMonInAndOutSetup(struct Task *task)
{
    task->data[1] = gBattleAnimArgs[1];
    task->data[2] = 0;
    task->data[3] = gBattleAnimArgs[2];
    task->data[4] = 0;
    task->data[5] = gBattleAnimArgs[3];
    task->data[6] = 0;
    task->data[7] = gBattleAnimArgs[4];
    task->func = AnimTask_BlendMonInAndOut_Step;
}

static void AnimTask_BlendMonInAndOut_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (++task->data[4] >= task->data[5])
    {
        task->data[4] = 0;
        if (!task->data[6])
        {
            ++task->data[2];
            BlendPalette(task->data[0], 15, task->data[2], task->data[1]);
            if (task->data[2] == task->data[3])
                task->data[6] = 1;
        }
        else
        {
            --task->data[2];
            BlendPalette(task->data[0], 15, task->data[2], task->data[1]);
            if (!task->data[2])
            {
                if (--task->data[7])
                {
                    task->data[4] = 0;
                    task->data[6] = 0;
                }
                else
                {
                    DestroyAnimVisualTask(taskId);
                    return;
                }
            }
        }
    }
}

// See AnimTask_BlendMonInAndOut. Same, but ANIM_TAG_* instead of mon
void AnimTask_BlendPalInAndOutByTag(u8 taskId)
{
    u8 palette = IndexOfSpritePaletteTag(gBattleAnimArgs[0]);

    if (palette == 0xFF)
    {
        DestroyAnimVisualTask(taskId);
        return;
    }
    gTasks[taskId].data[0] = (palette * 0x10) + 0x101;
    AnimTask_BlendMonInAndOutSetup(&gTasks[taskId]);
}

void PrepareAffineAnimInTaskData(struct Task *task, u8 spriteId, const union AffineAnimCmd *affineAnimCmds)
{
    task->data[7] = 0;
    task->data[8] = 0;
    task->data[9] = 0;
    task->data[15] = spriteId;
    task->data[10] = 0x100;
    task->data[11] = 0x100;
    task->data[12] = 0;
    StorePointerInVars(&task->data[13], &task->data[14], affineAnimCmds);
    PrepareBattlerSpriteForRotScale(spriteId, ST_OAM_OBJ_NORMAL);
}

bool8 RunAffineAnimFromTaskData(struct Task *task)
{
    sAnimTaskAffineAnim = LoadPointerFromVars(task->data[13], task->data[14]) + (task->data[7] << 3);
    switch (sAnimTaskAffineAnim->type)
    {
    default:
        if (!sAnimTaskAffineAnim->frame.duration)
        {
            task->data[10] = sAnimTaskAffineAnim->frame.xScale;
            task->data[11] = sAnimTaskAffineAnim->frame.yScale;
            task->data[12] = sAnimTaskAffineAnim->frame.rotation;
            ++task->data[7];
            ++sAnimTaskAffineAnim;
        }
        task->data[10] += sAnimTaskAffineAnim->frame.xScale;
        task->data[11] += sAnimTaskAffineAnim->frame.yScale;
        task->data[12] += sAnimTaskAffineAnim->frame.rotation;
        SetSpriteRotScale(task->data[15], task->data[10], task->data[11], task->data[12]);
        SetBattlerSpriteYOffsetFromYScale(task->data[15]);
        if (++task->data[8] >= sAnimTaskAffineAnim->frame.duration)
        {
            task->data[8] = 0;
            ++task->data[7];
        }
        break;
    case AFFINEANIMCMDTYPE_JUMP:
        task->data[7] = sAnimTaskAffineAnim->jump.target;
        break;
    case AFFINEANIMCMDTYPE_LOOP:
        if (sAnimTaskAffineAnim->loop.count)
        {
            if (task->data[9])
            {
                if (!--task->data[9])
                {
                    ++task->data[7];
                    break;
                }
            }
            else
            {
                task->data[9] = sAnimTaskAffineAnim->loop.count;
            }
            if (!task->data[7])
            {
                break;
            }
            while (TRUE)
            {
                --task->data[7];
                --sAnimTaskAffineAnim;
                if (sAnimTaskAffineAnim->type == AFFINEANIMCMDTYPE_LOOP)
                {
                    ++task->data[7];
                    return TRUE;
                }
                if (!task->data[7])
                    return TRUE;
            }
        }
        ++task->data[7];
        break;
    case AFFINEANIMCMDTYPE_END:
        gSprites[task->data[15]].y2 = 0;
        ResetSpriteRotScale(task->data[15]);
        return FALSE;
    }
    return TRUE;
}

// Sets the sprite's y offset equal to the y displacement caused by the
// matrix's scale in the y dimension.
void SetBattlerSpriteYOffsetFromYScale(u8 spriteId)
{
    s32 var = MON_PIC_HEIGHT - GetBattlerYDeltaFromSpriteId(spriteId) * 2;
    u16 matrix = gSprites[spriteId].oam.matrixNum;
    s32 var2 = SAFE_DIV(var << 8, gOamMatrices[matrix].d);

    if (var2 > MON_PIC_HEIGHT * 2)
        var2 = MON_PIC_HEIGHT * 2;
    gSprites[spriteId].y2 = (var - var2) / 2;
}

// Sets the sprite's y offset equal to the y displacement caused by another sprite
// matrix's scale in the y dimension.
void SetBattlerSpriteYOffsetFromOtherYScale(u8 spriteId, u8 otherSpriteId)
{
    s32 var = MON_PIC_HEIGHT - GetBattlerYDeltaFromSpriteId(otherSpriteId) * 2;
    u16 matrix = gSprites[spriteId].oam.matrixNum;
    s32 var2 = SAFE_DIV((var << 8), gOamMatrices[matrix].d);

    if (var2 > MON_PIC_HEIGHT * 2)
        var2 = MON_PIC_HEIGHT * 2;
    gSprites[spriteId].y2 = (var - var2) / 2;
}

static u16 GetBattlerYDeltaFromSpriteId(u8 spriteId)
{
    struct BattleSpriteInfo *spriteInfo;
    u8 battlerId = gSprites[spriteId].data[0];
    u16 species;
    u16 i;

    for (i = 0; i < MAX_BATTLERS_COUNT; ++i)
    {
        if (gBattlerSpriteIds[i] == spriteId)
        {
            if (GetBattlerSide(i) == B_SIDE_PLAYER)
            {
                spriteInfo = gBattleSpritesDataPtr->battlerData;
                if (!spriteInfo[battlerId].transformSpecies)
                    species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[i]], MON_DATA_SPECIES);
                else
                    species = spriteInfo[battlerId].transformSpecies;
                return gMonBackPicCoords[species].y_offset;
            }
            else
            {
                spriteInfo = gBattleSpritesDataPtr->battlerData;
                if (!spriteInfo[battlerId].transformSpecies)
                    species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[i]], MON_DATA_SPECIES);
                else
                    species = spriteInfo[battlerId].transformSpecies;
                return gMonFrontPicCoords[species].y_offset;
            }
        }
    }
    return MON_PIC_HEIGHT;
}

void StorePointerInVars(s16 *lo, s16 *hi, const void *ptr)
{
    *lo = ((intptr_t)ptr) & 0xffff;
    *hi = (((intptr_t)ptr) >> 16) & 0xffff;
}

void *LoadPointerFromVars(s16 lo, s16 hi)
{
    return (void *)((u16)lo | ((u16)hi << 16));
}

void BattleAnimHelper_SetSpriteSquashParams(struct Task *task, u8 spriteId, s16 xScaleStart, s16 yScaleStart, s16 xScaleEnd, s16 yScaleEnd, u16 duration)
{
    task->data[8] = duration;
    task->data[15] = spriteId;
    task->data[9] = xScaleStart;
    task->data[10] = yScaleStart;
    task->data[13] = xScaleEnd;
    task->data[14] = yScaleEnd;
    task->data[11] = (xScaleEnd - xScaleStart) / duration;
    task->data[12] = (yScaleEnd - yScaleStart) / duration;
}

u8 BattleAnimHelper_RunSpriteSquash(struct Task *task)
{
    if (!task->data[8])
        return 0;
    if (--task->data[8] != 0)
    {
        task->data[9] += task->data[11];
        task->data[10] += task->data[12];
    }
    else
    {
        task->data[9] = task->data[13];
        task->data[10] = task->data[14];
    }
    SetSpriteRotScale(task->data[15], task->data[9], task->data[10], 0);
    if (task->data[8])
        SetBattlerSpriteYOffsetFromYScale(task->data[15]);
    else
        gSprites[task->data[15]].y2 = 0;
    return task->data[8];
}

void AnimTask_GetFrustrationPowerLevel(u8 taskId)
{
    u16 powerLevel;

    if (gAnimFriendship <= 30)
        powerLevel = 0;
    else if (gAnimFriendship <= 100)
        powerLevel = 1;
    else if (gAnimFriendship <= 200)
        powerLevel = 2;
    else
        powerLevel = 3;
    gBattleAnimArgs[ARG_RET_ID] = powerLevel;
    DestroyAnimVisualTask(taskId);
}

// Unused
static void SetPriorityForVisibleBattlers(u8 priority)
{
    if (IsBattlerSpriteVisible(gBattleAnimTarget))
        gSprites[gBattlerSpriteIds[gBattleAnimTarget]].oam.priority = priority;
    if (IsBattlerSpriteVisible(gBattleAnimAttacker))
        gSprites[gBattlerSpriteIds[gBattleAnimAttacker]].oam.priority = priority;
    if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimTarget)))
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimTarget)]].oam.priority = priority;
    if (IsBattlerSpriteVisible(BATTLE_PARTNER(gBattleAnimAttacker)))
        gSprites[gBattlerSpriteIds[BATTLE_PARTNER(gBattleAnimAttacker)]].oam.priority = priority;
}

void InitPrioritiesForVisibleBattlers(void)
{
    s32 i;

    for (i = 0; i < gBattlersCount; ++i)
    {
        if (IsBattlerSpriteVisible(i))
        {
            gSprites[gBattlerSpriteIds[i]].subpriority = GetBattlerSpriteSubpriority(i);
            gSprites[gBattlerSpriteIds[i]].oam.priority = 2;
        }
    }
}

u8 GetBattlerSpriteSubpriority(u8 battlerId)
{
    u8 subpriority;
    u8 position = GetBattlerPosition(battlerId);

    if (position == B_POSITION_PLAYER_LEFT)
        subpriority = 30;
    else if (position == B_POSITION_PLAYER_RIGHT)
        subpriority = 20;
    else if (position == B_POSITION_OPPONENT_LEFT)
        subpriority = 40;
    else
        subpriority = 50;
    return subpriority;
}

u8 GetBattlerSpriteBGPriority(u8 battlerId)
{
    u8 position = GetBattlerPosition(battlerId);

    if (position == B_POSITION_PLAYER_LEFT || position == B_POSITION_OPPONENT_RIGHT)
        return GetAnimBgAttribute(2, BG_ANIM_PRIORITY);
    else
        return GetAnimBgAttribute(1, BG_ANIM_PRIORITY);
}

u8 GetBattlerSpriteBGPriorityRank(u8 battlerId)
{
    u8 position = GetBattlerPosition(battlerId);

    if (position == B_POSITION_PLAYER_LEFT || position == B_POSITION_OPPONENT_RIGHT)
        return 2;
    else
        return 1;
}

// Create pokemon sprite to be used for a move animation effect (e.g. Role Play / Snatch)
u8 CreateAdditionalMonSpriteForMoveAnim(u16 species, bool8 isBackpic, u8 templateId, s16 x, s16 y, u8 subpriority, u32 personality, u32 trainerId, u32 battlerId, bool32 ignoreDeoxys)
{
    u8 spriteId;
    u16 sheet = LoadSpriteSheet(&sSpriteSheets_MoveEffectMons[templateId]);
    u16 palette = AllocSpritePalette(sSpriteTemplates_MoveEffectMons[templateId].paletteTag);

    if (gMonSpritesGfxPtr != NULL && gMonSpritesGfxPtr->multiUseBuffer == NULL)
        gMonSpritesGfxPtr->multiUseBuffer = AllocZeroed(MON_PIC_SIZE * MAX_MON_PIC_FRAMES);
    if (!isBackpic)
    {
        LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, trainerId, personality), OBJ_PLTT_ID(palette), PLTT_SIZE_4BPP);
        if (ignoreDeoxys == TRUE || ShouldIgnoreDeoxysForm(DEOXYS_CHECK_BATTLE_ANIM, battlerId) == TRUE || gBattleSpritesDataPtr->battlerData[battlerId].transformSpecies != 0)
            LoadSpecialPokePic_DontHandleDeoxys(&gMonFrontPicTable[species],
                                                gMonSpritesGfxPtr->multiUseBuffer,
                                                species,
                                                personality,
                                                TRUE);
        else
            LoadSpecialPokePic(&gMonFrontPicTable[species],
                               gMonSpritesGfxPtr->multiUseBuffer,
                               species,
                               personality,
                               TRUE);
    }
    else
    {
        LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(species, trainerId, personality), OBJ_PLTT_ID(palette), PLTT_SIZE_4BPP);
        if (ignoreDeoxys == TRUE || ShouldIgnoreDeoxysForm(DEOXYS_CHECK_BATTLE_ANIM, battlerId) == TRUE || gBattleSpritesDataPtr->battlerData[battlerId].transformSpecies != 0)
            LoadSpecialPokePic_DontHandleDeoxys(&gMonBackPicTable[species],
                                                gMonSpritesGfxPtr->multiUseBuffer,
                                                species,
                                                personality,
                                                FALSE);
        else
            LoadSpecialPokePic(&gMonBackPicTable[species],
                               gMonSpritesGfxPtr->multiUseBuffer,
                               species,
                               personality,
                               FALSE);
    }
    RequestDma3Copy(gMonSpritesGfxPtr->multiUseBuffer, (void *)(OBJ_VRAM0 + (sheet * 0x20)), 0x800, 1);
    FREE_AND_SET_NULL(gMonSpritesGfxPtr->multiUseBuffer);
    if (!isBackpic)
        spriteId = CreateSprite(&sSpriteTemplates_MoveEffectMons[templateId], x, y + gMonFrontPicCoords[species].y_offset, subpriority);
    else
        spriteId = CreateSprite(&sSpriteTemplates_MoveEffectMons[templateId], x, y + gMonBackPicCoords[species].y_offset, subpriority);
    return spriteId;
}

void DestroySpriteAndFreeResources_(struct Sprite *sprite)
{
    DestroySpriteAndFreeResources(sprite);
}

s16 GetBattlerSpriteCoordAttr(u8 battlerId, u8 attr)
{
    u16 species;
    u32 personality;
    u16 letter;
    u16 unownSpecies;
    s32 ret;
    const struct MonCoords *coords;
    struct BattleSpriteInfo *spriteInfo;

    if (GetBattlerSide(battlerId) == B_SIDE_PLAYER)
    {
        spriteInfo = gBattleSpritesDataPtr->battlerData;
        if (!spriteInfo[battlerId].transformSpecies)
        {
            species = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
            personality = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
        }
        else
        {
            species = spriteInfo[battlerId].transformSpecies;
            personality = gTransformedPersonalities[battlerId];
        }
        if (species == SPECIES_UNOWN)
        {
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                unownSpecies = SPECIES_UNOWN;
            else
                unownSpecies = letter + SPECIES_UNOWN_B - 1;
            coords = &gMonBackPicCoords[unownSpecies];
        }
        else if (species > NUM_SPECIES)
        {
            coords = &gMonBackPicCoords[0];
        }
        else
        {
            coords = &gMonBackPicCoords[species];
        }
    }
    else
    {
        spriteInfo = gBattleSpritesDataPtr->battlerData;
        if (!spriteInfo[battlerId].transformSpecies)
        {
            species = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_SPECIES);
            personality = GetMonData(&gEnemyParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PERSONALITY);
        }
        else
        {
            species = spriteInfo[battlerId].transformSpecies;
            personality = gTransformedPersonalities[battlerId];
        }

        if (species == SPECIES_UNOWN)
        {
            letter = GET_UNOWN_LETTER(personality);
            if (!letter)
                unownSpecies = SPECIES_UNOWN;
            else
                unownSpecies = letter + SPECIES_UNOWN_B - 1;
            coords = &gMonFrontPicCoords[unownSpecies];
        }
        else if (species == SPECIES_CASTFORM)
        {
            coords = &gCastformFrontSpriteCoords[gBattleMonForms[battlerId]];
        }
        else if (species > NUM_SPECIES)
        {
            coords = &gMonFrontPicCoords[0];
        }
        else
        {
            coords = &gMonFrontPicCoords[species];
        }
    }
    switch (attr)
    {
    case BATTLER_COORD_ATTR_HEIGHT:
        return GET_MON_COORDS_HEIGHT(coords->size);
    case BATTLER_COORD_ATTR_WIDTH:
        return GET_MON_COORDS_WIDTH(coords->size);
    case BATTLER_COORD_ATTR_LEFT:
        return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_X_2) - (GET_MON_COORDS_WIDTH(coords->size) / 2);
    case BATTLER_COORD_ATTR_RIGHT:
        return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_X_2) + (GET_MON_COORDS_WIDTH(coords->size) / 2);
    case BATTLER_COORD_ATTR_TOP:
        return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y_PIC_OFFSET) - (GET_MON_COORDS_HEIGHT(coords->size) / 2);
    case BATTLER_COORD_ATTR_BOTTOM:
        return GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y_PIC_OFFSET) + (GET_MON_COORDS_HEIGHT(coords->size) / 2);
    case BATTLER_COORD_ATTR_RAW_BOTTOM:
        ret = GetBattlerSpriteCoord(battlerId, BATTLER_COORD_Y) + 31;
        return ret - coords->y_offset;
    default:
        return 0;
    }
}

void SetAverageBattlerPositions(u8 battlerId, bool8 respectMonPicOffsets, s16 *x, s16 *y)
{
    u8 xCoordType, yCoordType;
    s16 battlerX, battlerY;
    s16 partnerX, partnerY;

    if (!respectMonPicOffsets)
    {
        xCoordType = BATTLER_COORD_X;
        yCoordType = BATTLER_COORD_Y;
    }
    else
    {
        xCoordType = BATTLER_COORD_X_2;
        yCoordType = BATTLER_COORD_Y_PIC_OFFSET;
    }
    battlerX = GetBattlerSpriteCoord(battlerId, xCoordType);
    battlerY = GetBattlerSpriteCoord(battlerId, yCoordType);
    if (IsDoubleBattle())
    {
        partnerX = GetBattlerSpriteCoord(BATTLE_PARTNER(battlerId), xCoordType);
        partnerY = GetBattlerSpriteCoord(BATTLE_PARTNER(battlerId), yCoordType);
    }
    else
    {
        partnerX = battlerX;
        partnerY = battlerY;
    }
    *x = (battlerX + partnerX) / 2;
    *y = (battlerY + partnerY) / 2;
}

u8 CreateInvisibleSpriteCopy(s32 battlerId, u8 spriteId, s32 species)
{
    u8 newSpriteId = CreateInvisibleSpriteWithCallback(SpriteCallbackDummy);

    gSprites[newSpriteId] = gSprites[spriteId];
    gSprites[newSpriteId].usingSheet = TRUE;
    gSprites[newSpriteId].oam.priority = 0;
    gSprites[newSpriteId].oam.objMode = ST_OAM_OBJ_WINDOW;
    gSprites[newSpriteId].oam.tileNum = gSprites[spriteId].oam.tileNum;
    gSprites[newSpriteId].callback = SpriteCallbackDummy;
    return newSpriteId;
}

void AnimTranslateLinearAndFlicker_Flipped(struct Sprite *sprite)
{
    SetSpriteCoordsToAnimAttackerCoords(sprite);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        sprite->x -= gBattleAnimArgs[0];
        gBattleAnimArgs[3] = -gBattleAnimArgs[3];
        sprite->hFlip = TRUE;
    }
    else
    {
        sprite->x += gBattleAnimArgs[0];
    }
    sprite->y += gBattleAnimArgs[1];
    sprite->data[0] = gBattleAnimArgs[2];
    sprite->data[1] = gBattleAnimArgs[3];
    sprite->data[3] = gBattleAnimArgs[4];
    sprite->data[5] = gBattleAnimArgs[5];
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = TranslateSpriteLinearAndFlicker;
}

// Used by three different unused battle anim sprite templates.
void AnimTranslateLinearAndFlicker(struct Sprite *sprite)
{
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
    {
        sprite->x -= gBattleAnimArgs[0];
        gBattleAnimArgs[3] *= -1;
    }
    else
    {
        sprite->x += gBattleAnimArgs[0];
    }
    sprite->y += gBattleAnimArgs[1];
    sprite->data[0] = gBattleAnimArgs[2];
    sprite->data[1] = gBattleAnimArgs[3];
    sprite->data[3] = gBattleAnimArgs[4];
    sprite->data[5] = gBattleAnimArgs[5];
    StartSpriteAnim(sprite, gBattleAnimArgs[6]);
    StoreSpriteCallbackInData6(sprite, DestroySpriteAndMatrix);
    sprite->callback = TranslateSpriteLinearAndFlicker;
}

// Used by Detect/Disable
void AnimSpinningSparkle(struct Sprite *sprite)
{
    SetSpriteCoordsToAnimAttackerCoords(sprite);
    if (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER)
        sprite->x -= gBattleAnimArgs[0];
    else
        sprite->x += gBattleAnimArgs[0];
    sprite->y += gBattleAnimArgs[1];
    sprite->callback = RunStoredCallbackWhenAnimEnds;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}

// Task and sprite data for AnimTask_AttackerPunchWithTrace
#define tBattlerSpriteId data[0]
#define tMoveSpeed       data[1]
#define tState           data[2]
#define tCounter         data[3]
#define tPaletteNum      data[4]
#define tNumTracesActive data[5]
#define tPriority        data[6]

#define sActiveTime data[0]
#define sTaskId     data[1]
#define sSpriteId   data[2]

void AnimTask_AttackerPunchWithTrace(u8 taskId)
{
    u16 src;
    u16 dest;
    struct Task *task = &gTasks[taskId];

    task->tBattlerSpriteId = GetAnimBattlerSpriteId(ANIM_ATTACKER);
    task->tMoveSpeed = (GetBattlerSide(gBattleAnimAttacker) != B_SIDE_PLAYER) ? -8 : 8;
    task->tState = 0;
    task->tCounter = 0;
    gSprites[task->tBattlerSpriteId].x2 -= task->tBattlerSpriteId;
    task->tPaletteNum = AllocSpritePalette(ANIM_TAG_BENT_SPOON);
    task->tNumTracesActive = 0;

    dest = OBJ_PLTT_ID2(task->tPaletteNum);
    src = OBJ_PLTT_ID2(gSprites[task->tBattlerSpriteId].oam.paletteNum);
    
    // Set trace's priority based on battler's subpriority
    task->tPriority = GetBattlerSpriteSubpriority(gBattleAnimAttacker);
    if (task->tPriority == 20 || task->tPriority == 40)
        task->tPriority = 2;
    else
        task->tPriority = 3;

    CpuCopy32(&gPlttBufferUnfaded[src], &gPlttBufferFaded[dest], PLTT_SIZE_4BPP);
    BlendPalette(dest, 16, gBattleAnimArgs[1], gBattleAnimArgs[0]);
    task->func = AnimTask_AttackerPunchWithTrace_Step;
}

static void AnimTask_AttackerPunchWithTrace_Step(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    switch (task->tState)
    {
    case 0:
        // Move forward
        CreateBattlerTrace(task, taskId);
        gSprites[task->tBattlerSpriteId].x2 += task->tMoveSpeed;
        if (++task->tCounter == 5)
        {
            task->tCounter--;
            task->tState++;
        }
        break;
    case 1:
        // Move back (do same number of traces as before)
        CreateBattlerTrace(task, taskId);
        gSprites[task->tBattlerSpriteId].x2 -= task->tMoveSpeed;
        if (--task->tCounter == 0)
        {
            gSprites[task->tBattlerSpriteId].x2 = 0;
            task->tState++;
        }
        break;
    case 2:
        if (task->tNumTracesActive == 0)
        {
            FreeSpritePaletteByTag(ANIM_TAG_BENT_SPOON);
            DestroyAnimVisualTask(taskId);
        }
        break;
    }
}

static void CreateBattlerTrace(struct Task *task, u8 taskId)
{
    s16 spriteId = CloneBattlerSpriteWithBlend(0);
    if (spriteId >= 0)
    {
        gSprites[spriteId].oam.priority = task->tPriority;
        gSprites[spriteId].oam.paletteNum = task->tPaletteNum;
        gSprites[spriteId].sActiveTime = 8;
        gSprites[spriteId].sTaskId = taskId;
        gSprites[spriteId].sSpriteId = spriteId;
        gSprites[spriteId].x2 = gSprites[task->tBattlerSpriteId].x2;
        gSprites[spriteId].callback = AnimBattlerTrace;
        task->tNumTracesActive++;
    }
}

// Just waits until destroyed
static void AnimBattlerTrace(struct Sprite *sprite)
{
    if (--sprite->sActiveTime == 0)
    {
        gTasks[sprite->sTaskId].tNumTracesActive--;
        DestroySpriteWithActiveSheet(sprite);
    }
}

#undef tBattlerSpriteId
#undef tMoveSpeed
#undef tState
#undef tCounter
#undef tPaletteNum
#undef tNumTracesActive
#undef tPriority

#undef sActiveTime
#undef sTaskId
#undef sSpriteId

void AnimWeatherBallUp(struct Sprite *sprite)
{
    sprite->x = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_X_2);
    sprite->y = GetBattlerSpriteCoord(gBattleAnimAttacker, BATTLER_COORD_Y_PIC_OFFSET);
    if (GetBattlerSide(gBattleAnimAttacker) == B_SIDE_PLAYER)
        sprite->data[0] = 5;
    else
        sprite->data[0] = -10;
    sprite->data[1] = -40;
    sprite->callback = AnimWeatherBallUp_Step;
}

static void AnimWeatherBallUp_Step(struct Sprite *sprite)
{
    sprite->data[2] += sprite->data[0];
    sprite->data[3] += sprite->data[1];
    sprite->x2 = sprite->data[2] / 10;
    sprite->y2 = sprite->data[3] / 10;
    if (sprite->data[1] < -20)
        ++sprite->data[1];
    if (sprite->y + sprite->y2 < -32)
        DestroyAnimSprite(sprite);
}

void AnimWeatherBallDown(struct Sprite *sprite)
{
    s32 x;

    sprite->data[0] = gBattleAnimArgs[2];
    sprite->data[2] = sprite->x + gBattleAnimArgs[4];
    sprite->data[4] = sprite->y + gBattleAnimArgs[5];
    if (GetBattlerSide(gBattleAnimTarget) == B_SIDE_PLAYER)
    {
        x = (u16)gBattleAnimArgs[4] + 30;
        sprite->x += x;
        sprite->y = gBattleAnimArgs[5] - 20;
    }
    else
    {
        x = (u16)gBattleAnimArgs[4] - 30;
        sprite->x += x;
        sprite->y = gBattleAnimArgs[5] - 80;
    }
    sprite->callback = StartAnimLinearTranslation;
    StoreSpriteCallbackInData6(sprite, DestroyAnimSprite);
}
