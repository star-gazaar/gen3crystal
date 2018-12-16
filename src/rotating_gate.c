#include "global.h"
#include "bike.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "fieldmap.h"
#include "sound.h"
#include "sprite.h"
#include "constants/maps.h"
#include "constants/songs.h"

#define ROTATING_GATE_TILE_TAG 0x1300
#define ROTATING_GATE_PUZZLE_MAX 12
#define GATE_ARM_MAX_LENGTH 2

#define GATE_ROT(rotationDirection, arm, longArm)                                             \
    ((rotationDirection & 15) << 4) | ((arm & 7) << 1) | (longArm & 1)
#define GATE_ROT_CW(arm, longArm) GATE_ROT(ROTATE_CLOCKWISE, arm, longArm)
#define GATE_ROT_ACW(arm, longArm) GATE_ROT(ROTATE_ANTICLOCKWISE, arm, longArm)
#define GATE_ROT_NONE 255

// static functions
static void SpriteCallback_RotatingGate(struct Sprite *sprite);
static u8 RotatingGate_CreateGate(u8 gateId, s16 deltaX, s16 deltaY);
static void RotatingGate_HideGatesOutsideViewport(struct Sprite *sprite);

// enums
enum
{
    /*
    * |
    * +--
    */
    GATE_SHAPE_L1,

    /*
    * |
    * |
    * +--
    */
    GATE_SHAPE_L2,

    /*
    * |
    * +----
    */
    GATE_SHAPE_L3,

    /*
    * |
    * |
    * +----
    */
    GATE_SHAPE_L4,

    /*
    * |
    * +--
    * |
    */
    GATE_SHAPE_T1,

    /*
    * |
    * |
    * +--
    * |
    */
    GATE_SHAPE_T2,

    /*
    * |
    * +----
    * |
    */
    GATE_SHAPE_T3,

    /*
    * An unused T-shape gate
    * |
    * +--
    * |
    * |
    */
    GATE_SHAPE_T4,

    /*
    * An unused T-shape gate
    * |
    * |
    * +----
    * |
    */
    GATE_SHAPE_UNUSED_T1,

    /*
    * An unused T-shape gate
    * |
    * |
    * +--
    * |
    * |
    */
    GATE_SHAPE_UNUSED_T2,

    /*
    * An unused T-shape gate
    * |
    * +----
    * |
    * |
    */
    GATE_SHAPE_UNUSED_T3,

    /*
    * An unused T-shape gate
    * |
    * |
    * +----
    * |
    * |
    */
    GATE_SHAPE_UNUSED_T4,
};

enum
{
    /*
    * 0 degrees (clockwise)
    * |
    * +--
    * |
    */
    GATE_ORIENTATION_0,

    /*
    * 90 degress (clockwise)
    * --+--
    *   |
    */
    GATE_ORIENTATION_90,

    /*
    * 180 degrees (clockwise)
    *   |
    * --+
    *   |
    */
    GATE_ORIENTATION_180,

    /*
    * 270 degrees (clockwise)
    *   |
    * --+--
    */
    GATE_ORIENTATION_270,

    GATE_ORIENTATION_MAX,
};

// Describes the location of the gates "arms" when the gate has not
// been rotated (i.e. rotated 0 degrees)
enum
{
    GATE_ARM_NORTH,
    GATE_ARM_EAST,
    GATE_ARM_SOUTH,
    GATE_ARM_WEST,
};

enum
{
    ROTATE_NONE,
    ROTATE_ANTICLOCKWISE,
    ROTATE_CLOCKWISE,
};

enum
{
    PUZZLE_NONE,
    PUZZLE_FORTREE_CITY_GYM,
    PUZZLE_ROUTE110_TRICK_HOUSE_PUZZLE6,
};

// structure
struct RotatingGatePuzzle
{
    s16 x;
    s16 y;
    u8 shape;
    u8 orientation;
};

// .rodata
// Fortree
static const struct RotatingGatePuzzle sRotatingGate_FortreePuzzleConfig[] =
{
    { 6,  7, GATE_SHAPE_T2, GATE_ORIENTATION_90},
    { 9, 15, GATE_SHAPE_T2, GATE_ORIENTATION_180},
    { 3, 19, GATE_SHAPE_T2, GATE_ORIENTATION_90},
    { 2,  6, GATE_SHAPE_T1, GATE_ORIENTATION_90},
    { 9, 12, GATE_SHAPE_T1, GATE_ORIENTATION_0},
    { 6, 23, GATE_SHAPE_T1, GATE_ORIENTATION_0},
    {12, 22, GATE_SHAPE_T1, GATE_ORIENTATION_0},
    { 6,  3, GATE_SHAPE_L4, GATE_ORIENTATION_180},
};

// Trickhouse
static const struct RotatingGatePuzzle sRotatingGate_TrickHousePuzzleConfig[] =
{
    {14,  5, GATE_SHAPE_T1, GATE_ORIENTATION_90},
    {10,  6, GATE_SHAPE_L2, GATE_ORIENTATION_180},
    { 6,  6, GATE_SHAPE_L4, GATE_ORIENTATION_90},
    {14,  8, GATE_SHAPE_T1, GATE_ORIENTATION_90},
    { 3, 10, GATE_SHAPE_L3, GATE_ORIENTATION_270},
    { 9, 14, GATE_SHAPE_L1, GATE_ORIENTATION_90},
    { 3, 15, GATE_SHAPE_T3, GATE_ORIENTATION_0},
    { 2, 17, GATE_SHAPE_L2, GATE_ORIENTATION_180},
    {12, 18, GATE_SHAPE_T3, GATE_ORIENTATION_270},
    { 5, 18, GATE_SHAPE_L4, GATE_ORIENTATION_90},
    {10, 19, GATE_SHAPE_L3, GATE_ORIENTATION_180},
};

static const u8 sRotatingGateTiles_1[] = INCBIN_U8("graphics/misc/rotating_gate_1.4bpp");
static const u8 sRotatingGateTiles_2[] = INCBIN_U8("graphics/misc/rotating_gate_2.4bpp");
static const u8 sRotatingGateTiles_3[] = INCBIN_U8("graphics/misc/rotating_gate_3.4bpp");
static const u8 sRotatingGateTiles_4[] = INCBIN_U8("graphics/misc/rotating_gate_4.4bpp");
static const u8 sRotatingGateTiles_5[] = INCBIN_U8("graphics/misc/rotating_gate_5.4bpp");
static const u8 sRotatingGateTiles_6[] = INCBIN_U8("graphics/misc/rotating_gate_6.4bpp");
static const u8 sRotatingGateTiles_7[] = INCBIN_U8("graphics/misc/rotating_gate_7.4bpp");
static const u8 sRotatingGateTiles_8[] = INCBIN_U8("graphics/misc/rotating_gate_8.4bpp");

static const struct OamData sOamData_RotatingGateLarge =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = 0,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = ST_OAM_SQUARE,
    .x = 0,
    .matrixNum = 0,
    .size = 3,
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 2,
    .affineParam = 0,
};

static const struct OamData sOamData_RotatingGateRegular =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = 0,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = ST_OAM_SQUARE,
    .x = 0,
    .matrixNum = 0,
    .size = 2,
    .tileNum = 0,
    .priority = 2,
    .paletteNum = 2,
    .affineParam = 0,
};

static const struct SpriteSheet sRotatingGatesGraphicsTable[] =
{
    {sRotatingGateTiles_1, 0x200, ROTATING_GATE_TILE_TAG + GATE_SHAPE_L1},
    {sRotatingGateTiles_2, 0x800, ROTATING_GATE_TILE_TAG + GATE_SHAPE_L2},
    {sRotatingGateTiles_3, 0x800, ROTATING_GATE_TILE_TAG + GATE_SHAPE_L3},
    {sRotatingGateTiles_4, 0x800, ROTATING_GATE_TILE_TAG + GATE_SHAPE_L4},
    {sRotatingGateTiles_5, 0x200, ROTATING_GATE_TILE_TAG + GATE_SHAPE_T1},
    {sRotatingGateTiles_6, 0x800, ROTATING_GATE_TILE_TAG + GATE_SHAPE_T2},
    {sRotatingGateTiles_7, 0x800, ROTATING_GATE_TILE_TAG + GATE_SHAPE_T3},
    {sRotatingGateTiles_8, 0x800, ROTATING_GATE_TILE_TAG + GATE_SHAPE_T4},
    {NULL},
};

static const union AnimCmd sSpriteAnim_RotatingGateLarge[] =
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_RotatingGateRegular[] =
{
    ANIMCMD_FRAME(0, 0), ANIMCMD_END,
};

static const union AnimCmd *const sSpriteAnimTable_RotatingGateLarge[] =
{
    sSpriteAnim_RotatingGateLarge,
};

static const union AnimCmd *const sSpriteAnimTable_RotatingGateRegular[] =
{
    sSpriteAnim_RotatingGateRegular,
};

static const union AffineAnimCmd sSpriteAffineAnim_Rotated0[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd sSpriteAffineAnim_Rotated90[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -64, 0),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd sSpriteAffineAnim_Rotated180[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -128, 0),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd sSpriteAffineAnim_Rotated270[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 64, 0),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise0to90[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise90to180[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise180to270[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -128, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise270to360[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise360to270[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise270to180[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise180to90[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -128, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise90to0[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 4, 16),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise0to90Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise90to180Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise180to270Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -128, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingClockwise270to360Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, -8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise360to270Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 0, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise270to180Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, 64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise180to90Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -128, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd sSpriteAffineAnim_RotatingAnticlockwise90to0Faster[] =
{
    AFFINEANIMCMD_FRAME(0x100, 0x100, -64, 0),
    AFFINEANIMCMD_FRAME(0x0, 0x0, 8, 8),
    AFFINEANIMCMD_END,
};

static const union AffineAnimCmd *const sSpriteAffineAnimTable_RotatingGate[] =
{
    sSpriteAffineAnim_Rotated0,
    sSpriteAffineAnim_Rotated90,
    sSpriteAffineAnim_Rotated180,
    sSpriteAffineAnim_Rotated270,
    sSpriteAffineAnim_RotatingAnticlockwise360to270,
    sSpriteAffineAnim_RotatingAnticlockwise90to0,
    sSpriteAffineAnim_RotatingAnticlockwise180to90,
    sSpriteAffineAnim_RotatingAnticlockwise270to180,
    sSpriteAffineAnim_RotatingClockwise0to90,
    sSpriteAffineAnim_RotatingClockwise90to180,
    sSpriteAffineAnim_RotatingClockwise180to270,
    sSpriteAffineAnim_RotatingClockwise270to360,
    sSpriteAffineAnim_RotatingAnticlockwise360to270Faster,
    sSpriteAffineAnim_RotatingAnticlockwise90to0Faster,
    sSpriteAffineAnim_RotatingAnticlockwise180to90Faster,
    sSpriteAffineAnim_RotatingAnticlockwise270to180Faster,
    sSpriteAffineAnim_RotatingClockwise0to90Faster,
    sSpriteAffineAnim_RotatingClockwise90to180Faster,
    sSpriteAffineAnim_RotatingClockwise180to270Faster,
    sSpriteAffineAnim_RotatingClockwise270to360Faster,
};


static const struct SpriteTemplate sSpriteTemplate_RotatingGateLarge =
{
    .tileTag = ROTATING_GATE_TILE_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sOamData_RotatingGateLarge,
    .anims = sSpriteAnimTable_RotatingGateLarge,
    .images = NULL,
    .affineAnims = sSpriteAffineAnimTable_RotatingGate,
    .callback = SpriteCallback_RotatingGate,
};

static const struct SpriteTemplate sSpriteTemplate_RotatingGateRegular =
{
    .tileTag = ROTATING_GATE_TILE_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sOamData_RotatingGateRegular,
    .anims = sSpriteAnimTable_RotatingGateRegular,
    .images = NULL,
    .affineAnims = sSpriteAffineAnimTable_RotatingGate,
    .callback = SpriteCallback_RotatingGate,
};

// These structures describe what happens to the gate if you hit it at
// a given coordinate in a 4x4 grid when walking in the specified
// direction. Either the gate does not rotate, or it rotates in the
// given direction. This information is compared against the gate
// "arm" layout to see if there is an arm at the position in order to
// produce the final rotation.
static const u8 sRotatingGate_RotationInfoNorth[4 * 4] =
{
    GATE_ROT_NONE,                 GATE_ROT_NONE,                 GATE_ROT_NONE,                  GATE_ROT_NONE,
    GATE_ROT_CW(GATE_ARM_WEST, 1), GATE_ROT_CW(GATE_ARM_WEST, 0), GATE_ROT_ACW(GATE_ARM_EAST, 0), GATE_ROT_ACW(GATE_ARM_EAST, 1),
    GATE_ROT_NONE,                 GATE_ROT_NONE,                 GATE_ROT_NONE,                  GATE_ROT_NONE,
    GATE_ROT_NONE,                 GATE_ROT_NONE,                 GATE_ROT_NONE,                  GATE_ROT_NONE,
};

static const u8 sRotatingGate_RotationInfoSouth[4 * 4] =
{
    GATE_ROT_NONE,                  GATE_ROT_NONE,                  GATE_ROT_NONE,                 GATE_ROT_NONE,
    GATE_ROT_NONE,                  GATE_ROT_NONE,                  GATE_ROT_NONE,                 GATE_ROT_NONE,
    GATE_ROT_ACW(GATE_ARM_WEST, 1), GATE_ROT_ACW(GATE_ARM_WEST, 0), GATE_ROT_CW(GATE_ARM_EAST, 0), GATE_ROT_CW(GATE_ARM_EAST, 1),
    GATE_ROT_NONE,                  GATE_ROT_NONE,                  GATE_ROT_NONE,                 GATE_ROT_NONE,
};

static const u8 sRotatingGate_RotationInfoWest[4 * 4] =
{
    GATE_ROT_NONE, GATE_ROT_ACW(GATE_ARM_NORTH, 1), GATE_ROT_NONE, GATE_ROT_NONE,
    GATE_ROT_NONE, GATE_ROT_ACW(GATE_ARM_NORTH, 0), GATE_ROT_NONE, GATE_ROT_NONE,
    GATE_ROT_NONE, GATE_ROT_CW(GATE_ARM_SOUTH, 0),  GATE_ROT_NONE, GATE_ROT_NONE,
    GATE_ROT_NONE, GATE_ROT_CW(GATE_ARM_SOUTH, 1),  GATE_ROT_NONE, GATE_ROT_NONE,
};

static const u8 sRotatingGate_RotationInfoEast[4 * 4] =
{
    GATE_ROT_NONE, GATE_ROT_NONE, GATE_ROT_CW(GATE_ARM_NORTH, 1),  GATE_ROT_NONE,
    GATE_ROT_NONE, GATE_ROT_NONE, GATE_ROT_CW(GATE_ARM_NORTH, 0),  GATE_ROT_NONE,
    GATE_ROT_NONE, GATE_ROT_NONE, GATE_ROT_ACW(GATE_ARM_SOUTH, 0), GATE_ROT_NONE,
    GATE_ROT_NONE, GATE_ROT_NONE, GATE_ROT_ACW(GATE_ARM_SOUTH, 1), GATE_ROT_NONE,
};

// These tables describe the relative coordinate positions the arms
// must move through in order to be rotated.
static const struct Coords8 sRotatingGate_ArmPositionsClockwiseRotation[] = {
    { 0, -1 }, { 1, -2 }, { 0, 0 }, { 1, 0 }, { -1, 0 }, { -1, 1 }, { -1, -1 }, { -2, -1 },
};

static const struct Coords8 sRotatingGate_ArmPositionsAntiClockwiseRotation[] = {
    { -1, -1 }, { -1, -2 }, { 0, -1 }, { 1, -1 }, { 0, 0 }, { 0, 1 }, { -1, 0 }, { -2, 0 },
};

// Describes where the gates "arms" are in the order north, east, south, west.
// These are adjusted using the current orientation to perform collision checking
static const u8 sRotatingGate_ArmLayout[][4 * 2] =
{
    // L-shape gates
    {
        1, 0,
        1, 0,
        0, 0,
        0, 0,
    },
    {
        1, 1,
        1, 0,
        0, 0,
        0, 0,
    },
    {
        1, 0,
        1, 1,
        0, 0,
        0, 0,
    },
    {
        1, 1,
        1, 1,
        0, 0,
        0, 0,
    },

    // T-shape gates
    {
        1, 0,
        1, 0,
        1, 0,
        0, 0,
    },
    {
        1, 1,
        1, 0,
        1, 0,
        0, 0,
    },
    {
        1, 0,
        1, 1,
        1, 0,
        0, 0,
    },
    {
        1, 0,
        1, 0,
        1, 1,
        0, 0,
    },

    // Unused T-shape gates
    // These have 2-3 long arms and cannot actually be used anywhere
    // since configuration for them is missing from the other tables.
    {
        1, 1,
        1, 1,
        1, 0,
        0, 0,
    },
    {
        1, 1,
        1, 0,
        1, 1,
        0, 0,
    },
    {
        1, 0,
        1, 1,
        1, 1,
        0, 0,
    },
    {
        1, 1,
        1, 1,
        1, 1,
        0, 0,
    },
};

// ewram
static EWRAM_DATA u8 gRotatingGate_GateSpriteIds[ROTATING_GATE_PUZZLE_MAX] = {0};
static EWRAM_DATA const struct RotatingGatePuzzle *gRotatingGate_PuzzleConfig = NULL;
static EWRAM_DATA u8 gRotatingGate_PuzzleCount = 0;

// text
static s32 GetCurrentMapRotatingGatePuzzleType(void)
{
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(FORTREE_CITY_GYM) &&
        gSaveBlock1Ptr->location.mapNum == MAP_NUM(FORTREE_CITY_GYM))
    {
        return PUZZLE_FORTREE_CITY_GYM;
    }

    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(ROUTE110_TRICK_HOUSE_PUZZLE6) &&
        gSaveBlock1Ptr->location.mapNum == MAP_NUM(ROUTE110_TRICK_HOUSE_PUZZLE6))
    {
        return PUZZLE_ROUTE110_TRICK_HOUSE_PUZZLE6;
    }

    return PUZZLE_NONE;
}

static void RotatingGate_ResetAllGateOrientations(void)
{
    s32 i;
    u8 *ptr = (u8 *)GetVarPointer(VAR_TEMP_0);

    for (i = 0; i < gRotatingGate_PuzzleCount; i++)
    {
        ptr[i] = gRotatingGate_PuzzleConfig[i].orientation;
    }
}

static s32 RotatingGate_GetGateOrientation(u8 gateId)
{
    return ((u8 *)GetVarPointer(VAR_TEMP_0))[gateId];
}

static void RotatingGate_SetGateOrientation(u8 gateId, u8 orientation)
{
    ((u8 *)GetVarPointer(VAR_TEMP_0))[gateId] = orientation;
}

static void RotatingGate_RotateInDirection(u8 gateId, u32 rotationDirection)
{
    u8 orientation = RotatingGate_GetGateOrientation(gateId);

    if (rotationDirection == ROTATE_ANTICLOCKWISE)
    {
        if (orientation)
            orientation--;
        else
            orientation = GATE_ORIENTATION_270;
    }
    else
    {
        orientation = ++orientation % GATE_ORIENTATION_MAX;
    }
    RotatingGate_SetGateOrientation(gateId, orientation);
}

static void RotatingGate_LoadPuzzleConfig(void)
{
    s32 puzzleType = GetCurrentMapRotatingGatePuzzleType();
    u32 i;

    switch (puzzleType)
    {
    case PUZZLE_FORTREE_CITY_GYM:
        gRotatingGate_PuzzleConfig = sRotatingGate_FortreePuzzleConfig;
        gRotatingGate_PuzzleCount =
            sizeof(sRotatingGate_FortreePuzzleConfig) / sizeof(struct RotatingGatePuzzle);
        break;
    case PUZZLE_ROUTE110_TRICK_HOUSE_PUZZLE6:
        gRotatingGate_PuzzleConfig = sRotatingGate_TrickHousePuzzleConfig;
        gRotatingGate_PuzzleCount =
            sizeof(sRotatingGate_TrickHousePuzzleConfig) / sizeof(struct RotatingGatePuzzle);
        break;
    case PUZZLE_NONE:
    default:
        return;
    }

    for (i = 0; i < ROTATING_GATE_PUZZLE_MAX - 1; i++)
    {
        gRotatingGate_GateSpriteIds[i] = MAX_SPRITES;
    }
}

static void RotatingGate_CreateGatesWithinViewport(s16 deltaX, s16 deltaY)
{
    u8 i;

    // Calculate the bounding box of the camera
    // Same as RotatingGate_DestroyGatesOutsideViewport
    s16 x = gSaveBlock1Ptr->pos.x - 2;
    s16 x2 = gSaveBlock1Ptr->pos.x + 0x11;
    s16 y = gSaveBlock1Ptr->pos.y - 2;
    s16 y2 = gSaveBlock1Ptr->pos.y + 0xe;

    for (i = 0; i < gRotatingGate_PuzzleCount; i++)
    {
        s16 x3 = gRotatingGate_PuzzleConfig[i].x + 7;
        s16 y3 = gRotatingGate_PuzzleConfig[i].y + 7;

        if (y <= y3 && y2 >= y3 && x <= x3 && x2 >= x3 &&
            gRotatingGate_GateSpriteIds[i] == MAX_SPRITES)
        {
            gRotatingGate_GateSpriteIds[i] = RotatingGate_CreateGate(i, deltaX, deltaY);
        }
    }
}

static u8 RotatingGate_CreateGate(u8 gateId, s16 deltaX, s16 deltaY)
{
    struct Sprite *sprite;
    struct SpriteTemplate template;
    u8 spriteId;
    s16 x, y;

    const struct RotatingGatePuzzle *gate = &gRotatingGate_PuzzleConfig[gateId];

    if (gate->shape == GATE_SHAPE_L1 || gate->shape == GATE_SHAPE_T1)
        template = sSpriteTemplate_RotatingGateRegular;
    else
        template = sSpriteTemplate_RotatingGateLarge;

    template.tileTag = gate->shape + ROTATING_GATE_TILE_TAG;

    spriteId = CreateSprite(&template, 0, 0, 0x94);
    if (spriteId == MAX_SPRITES)
        return MAX_SPRITES;

    x = gate->x + 7;
    y = gate->y + 7;

    sprite = &gSprites[spriteId];
    sprite->data[0] = gateId;
    sprite->coordOffsetEnabled = 1;

    sub_8092FF0(x + deltaX, y + deltaY, &sprite->pos1.x, &sprite->pos1.y);
    RotatingGate_HideGatesOutsideViewport(sprite);
    StartSpriteAffineAnim(sprite, RotatingGate_GetGateOrientation(gateId));

    return spriteId;
}

static void SpriteCallback_RotatingGate(struct Sprite *sprite)
{
    u8 affineAnimation;
    u8 rotationDirection = sprite->data[1];
    u8 orientation = sprite->data[2];

    RotatingGate_HideGatesOutsideViewport(sprite);

    if (rotationDirection == ROTATE_ANTICLOCKWISE)
    {
        affineAnimation = orientation + 4;

        if (GetPlayerSpeed() != 1)
            affineAnimation += 8;

        PlaySE(SE_HI_TURUN);
        StartSpriteAffineAnim(sprite, affineAnimation);
    }
    else if (rotationDirection == ROTATE_CLOCKWISE)
    {
        affineAnimation = orientation + 8;

        if (GetPlayerSpeed() != 1)
            affineAnimation += 8;

        PlaySE(SE_HI_TURUN);
        StartSpriteAffineAnim(sprite, affineAnimation);
    }

    sprite->data[1] = ROTATE_NONE;
}

static void RotatingGate_HideGatesOutsideViewport(struct Sprite *sprite)
{
    u16 x, y;
    s16 x2, y2;

    sprite->invisible = FALSE;
    x = sprite->pos1.x + sprite->pos2.x + sprite->centerToCornerVecX + gSpriteCoordOffsetX;
    y = sprite->pos1.y + sprite->pos2.y + sprite->centerToCornerVecY + gSpriteCoordOffsetY;

    x2 = x + 0x40; // Dimensions of the rotating gate
    y2 = y + 0x40;

    if ((s16)x > DISPLAY_WIDTH + 0x10 - 1 || x2 < -0x10)
    {
        sprite->invisible = TRUE;
    }

    if ((s16)y > DISPLAY_HEIGHT + 0x10 - 1 || y2 < -0x10)
    {
        sprite->invisible = TRUE;
    }
}

static void LoadRotatingGatePics(void)
{
    LoadSpriteSheets(sRotatingGatesGraphicsTable);
}

static void RotatingGate_DestroyGatesOutsideViewport(void)
{
    s32 i;

    // Same as RotatingGate_CreateGatesWithinViewport
    s16 x = gSaveBlock1Ptr->pos.x - 2;
    s16 x2 = gSaveBlock1Ptr->pos.x + 0x11;
    s16 y = gSaveBlock1Ptr->pos.y - 2;
    s16 y2 = gSaveBlock1Ptr->pos.y + 0xe;

    for (i = 0; i < gRotatingGate_PuzzleCount; i++)
    {
        s16 xGate = gRotatingGate_PuzzleConfig[i].x + 7;
        s16 yGate = gRotatingGate_PuzzleConfig[i].y + 7;

        if (gRotatingGate_GateSpriteIds[i] == MAX_SPRITES)
            continue;

        if (xGate < x || xGate > x2 || yGate < y || yGate > y2)
        {
            struct Sprite *sprite = &gSprites[gRotatingGate_GateSpriteIds[i]];
            FreeSpriteOamMatrix(sprite);
            DestroySprite(sprite);
            gRotatingGate_GateSpriteIds[i] = MAX_SPRITES;
        }
    }
}

static s32 RotatingGate_CanRotate(u8 gateId, s32 rotationDirection)
{
    const struct Coords8 *armPos;
    u8 orientation;
    s16 x, y;
    u8 shape;
    s32 i, j;

    if (rotationDirection == ROTATE_ANTICLOCKWISE)
        armPos = sRotatingGate_ArmPositionsAntiClockwiseRotation;
    else if (rotationDirection == ROTATE_CLOCKWISE)
        armPos = sRotatingGate_ArmPositionsClockwiseRotation;
    else
        return FALSE;

    orientation = RotatingGate_GetGateOrientation(gateId);

    shape = gRotatingGate_PuzzleConfig[gateId].shape;
    x = gRotatingGate_PuzzleConfig[gateId].x + 7;
    y = gRotatingGate_PuzzleConfig[gateId].y + 7;

    // Loop through the gate's "arms" clockwise (north, south, east, west)
    for (i = GATE_ARM_NORTH; i <= GATE_ARM_WEST; i++)
    {
        // Ensure that no part of the arm collides with the map
        for (j = 0; j < GATE_ARM_MAX_LENGTH; j++)
        {
            u8 armIndex = 2 * ((orientation + i) % 4) + j;

            if (sRotatingGate_ArmLayout[shape][2 * i + j])
            {
                if (MapGridIsImpassableAt(x + armPos[armIndex].x, y + armPos[armIndex].y) == TRUE)
                    return FALSE;
            }
        }
    }

    return TRUE;
}

static s32 RotatingGate_HasArm(u8 gateId, u8 armInfo)
{
    s32 arm = armInfo / 2;
    s32 isLongArm = armInfo % 2;

    s8 armOrientation = (arm - RotatingGate_GetGateOrientation(gateId) + 4) % 4;
    s32 shape = gRotatingGate_PuzzleConfig[gateId].shape;
    return sRotatingGate_ArmLayout[shape][armOrientation * 2 + isLongArm];
}

static void RotatingGate_TriggerRotationAnimation(u8 gateId, s32 rotationDirection)
{
    if (gRotatingGate_GateSpriteIds[gateId] != MAX_SPRITES)
    {
        struct Sprite *sprite = &gSprites[gRotatingGate_GateSpriteIds[gateId]];
        sprite->data[1] = rotationDirection;
        sprite->data[2] = RotatingGate_GetGateOrientation(gateId);
    }
}

static u8 RotatingGate_GetRotationInfo(u8 direction, s16 x, s16 y)
{
    const u8 *ptr;

    if (direction == DIR_NORTH)
        ptr = sRotatingGate_RotationInfoNorth;
    else if (direction == DIR_SOUTH)
        ptr = sRotatingGate_RotationInfoSouth;
    else if (direction == DIR_WEST)
        ptr = sRotatingGate_RotationInfoWest;
    else if (direction == DIR_EAST)
        ptr = sRotatingGate_RotationInfoEast;
    else
        return GATE_ROT_NONE;

    return ptr[y * 4 + x];
}

void RotatingGate_InitPuzzle(void)
{
    if (GetCurrentMapRotatingGatePuzzleType())
    {
        RotatingGate_LoadPuzzleConfig();
        RotatingGate_ResetAllGateOrientations();
    }
}

void RotatingGatePuzzleCameraUpdate(u16 deltaX, u16 deltaY)
{
    if (GetCurrentMapRotatingGatePuzzleType())
    {
        RotatingGate_CreateGatesWithinViewport(deltaX, deltaY);
        RotatingGate_DestroyGatesOutsideViewport();
    }
}

void RotatingGate_InitPuzzleAndGraphics(void)
{
    if (GetCurrentMapRotatingGatePuzzleType())
    {
        LoadRotatingGatePics();
        RotatingGate_LoadPuzzleConfig();
        RotatingGate_CreateGatesWithinViewport(0, 0);
    }
}

bool8 CheckForRotatingGatePuzzleCollision(u8 direction, s16 x, s16 y)
{
    s32 i;

    if (!GetCurrentMapRotatingGatePuzzleType())
        return FALSE;
    for (i = 0; i < gRotatingGate_PuzzleCount; i++)
    {
        s16 gateX = gRotatingGate_PuzzleConfig[i].x + 7;
        s16 gateY = gRotatingGate_PuzzleConfig[i].y + 7;

        if (gateX - 2 <= x && x <= gateX + 1 && gateY - 2 <= y && y <= gateY + 1)
        {
            s16 centerX = x - gateX + 2;
            s16 centerY = y - gateY + 2;
            u8 rotationInfo = RotatingGate_GetRotationInfo(direction, centerX, centerY);

            if (rotationInfo != GATE_ROT_NONE)
            {
                u8 rotationDirection = ((rotationInfo & 0xF0) >> 4);
                u8 armInfo = rotationInfo & 0xF;

                if (RotatingGate_HasArm(i, armInfo))
                {
                    if (RotatingGate_CanRotate(i, rotationDirection))
                    {
                        RotatingGate_TriggerRotationAnimation(i, rotationDirection);
                        RotatingGate_RotateInDirection(i, rotationDirection);
                        return FALSE;
                    }
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

bool8 CheckForRotatingGatePuzzleCollisionWithoutAnimation(u8 direction, s16 x, s16 y)
{
    s32 i;

    if (!GetCurrentMapRotatingGatePuzzleType())
        return FALSE;
    for (i = 0; i < gRotatingGate_PuzzleCount; i++)
    {
        s16 gateX = gRotatingGate_PuzzleConfig[i].x + 7;
        s16 gateY = gRotatingGate_PuzzleConfig[i].y + 7;

        if (gateX - 2 <= x && x <= gateX + 1 && gateY - 2 <= y && y <= gateY + 1)
        {
            s16 centerX = x - gateX + 2;
            s16 centerY = y - gateY + 2;
            u8 rotationInfo = RotatingGate_GetRotationInfo(direction, centerX, centerY);

            if (rotationInfo != GATE_ROT_NONE)
            {
                u8 rotationDirection = ((rotationInfo & 0xF0) >> 4);
                u8 armInfo = rotationInfo & 0xF;

                if (RotatingGate_HasArm(i, armInfo))
                {
                    if (!RotatingGate_CanRotate(i, rotationDirection))
                    {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}
