#include "global.h"
#include "battle.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_animation.h"
#include "sprite.h"
#include "task.h"
#include "trig.h"
#include "util.h"
#include "data.h"
#include "constants/battle_anim.h"
#include "constants/rgb.h"

// file has been severely gutted as we didn't need any of these affines. 

/*
    This file handles the movements of the Pokémon intro animations.

    Each animation type is identified by an ANIM_* constant that
    refers to a sprite callback to start the animation. These functions
    are named Anim_<name> or Anim_<name>_<variant>. Many of these
    functions share additional movement functions to do a variation of the
    same movement (e.g. a faster or larger movement).
    Vertical and Horizontal are frequently shortened to V and H.

    Every front animation uses 1 of these ANIMs, and every back animation
    uses a BACK_ANIM_* that refers to a set of 3 ANIM functions. Which of the
    3 that gets used depends on the Pokémon's nature (see sBackAnimationIds).

    The table linking species to a BACK_ANIM is in this file (sSpeciesToBackAnimSet)
    while the table linking species to an ANIM for their front animation is in
    pokemon.c (sMonFrontAnimIdsTable).

    These are the functions that will start an animation:
    - LaunchAnimationTaskForFrontSprite
    - LaunchAnimationTaskForBackSprite
    - StartMonSummaryAnimation
*/

#define sDontFlip data[1]  // TRUE if a normal animation, FALSE if Summary Screen animation

struct PokemonAnimData
{
    u16 delay;
    s16 speed; // Only used by 2 sets of animations
    s16 runs; // Number of times to do the animation
    s16 rotation;
    s16 data; // General use
};

struct YellowFlashData
{
    bool8 isYellow;
    u8 time;
};

static void Anim_None(struct Sprite *sprite);

static void WaitAnimEnd(struct Sprite *sprite);

static struct PokemonAnimData sAnims[MAX_BATTLERS_COUNT];
static u8 sAnimIdx;
static bool32 sIsSummaryAnim;

static const u8 sSpeciesToBackAnimSet[] =
{
};

// Equivalent to struct YellowFlashData, but doesn't match as a struct
static const u8 sYellowFlashData[][2] =
{
    {FALSE,  5},
    { TRUE,  1},
    {FALSE, 15},
    { TRUE,  4},
    {FALSE,  2},
    { TRUE,  2},
    {FALSE,  2},
    { TRUE,  2},
    {FALSE,  2},
    { TRUE,  2},
    {FALSE,  2},
    { TRUE,  2},
    {FALSE,  2},
    {FALSE, -1}
};

static const u8 sVerticalShakeData[][2] =
{
    { 6,  30},
    {-2,  15},
    { 6,  30},
    {-1,   0}
};

static void (* const sMonAnimFunctions[])(struct Sprite *sprite) =
{
    [BACK_ANIM_NONE]                         = Anim_None,
};

// Each back anim set has 3 possible animations depending on nature
// Each of the 3 animations is a slight variation of the others
// BACK_ANIM_NONE is skipped below. GetSpeciesBackAnimSet subtracts 1 from the back anim id
static const u8 sBackAnimationIds[] =
{
};

static const u8 sBackAnimNatureModTable[NUM_NATURES] =
{
    [NATURE_HARDY]   = 0,
    [NATURE_LONELY]  = 2,
    [NATURE_BRAVE]   = 0,
    [NATURE_ADAMANT] = 0,
    [NATURE_NAUGHTY] = 0,
    [NATURE_BOLD]    = 1,
    [NATURE_DOCILE]  = 1,
    [NATURE_RELAXED] = 1,
    [NATURE_IMPISH]  = 0,
    [NATURE_LAX]     = 1,
    [NATURE_TIMID]   = 2,
    [NATURE_HASTY]   = 0,
    [NATURE_SERIOUS] = 1,
    [NATURE_JOLLY]   = 0,
    [NATURE_NAIVE]   = 0,
    [NATURE_MODEST]  = 2,
    [NATURE_MILD]    = 2,
    [NATURE_QUIET]   = 2,
    [NATURE_BASHFUL] = 2,
    [NATURE_RASH]    = 1,
    [NATURE_CALM]    = 1,
    [NATURE_GENTLE]  = 2,
    [NATURE_SASSY]   = 1,
    [NATURE_CAREFUL] = 2,
    [NATURE_QUIRKY]  = 1,
};

static const union AffineAnimCmd sMonAffineAnim_0[] =
{
    AFFINEANIMCMD_FRAME(256, 256, 0, 0),
    {AFFINEANIMCMDTYPE_END}
};

static const union AffineAnimCmd sMonAffineAnim_1[] =
{
    AFFINEANIMCMD_FRAME(-256, 256, 0, 0),
    {AFFINEANIMCMDTYPE_END}
};

static const union AffineAnimCmd *const sMonAffineAnims[] =
{
    sMonAffineAnim_0,
    sMonAffineAnim_1
};

static void MonAnimDummySpriteCallback(struct Sprite *sprite)
{
}

static void SetPosForRotation(struct Sprite *sprite, u16 index, s16 amplitudeX, s16 amplitudeY)
{
    s16 xAdder, yAdder;

    amplitudeX *= -1;
    amplitudeY *= -1;

    xAdder = Cos(index, amplitudeX) - Sin(index, amplitudeY);
    yAdder = Cos(index, amplitudeY) + Sin(index, amplitudeX);

    amplitudeX *= -1;
    amplitudeY *= -1;

    sprite->x2 = xAdder + amplitudeX;
    sprite->y2 = yAdder + amplitudeY;
}

u8 GetSpeciesBackAnimSet(u16 species)
{
    if (sSpeciesToBackAnimSet[species] != BACK_ANIM_NONE)
        return sSpeciesToBackAnimSet[species] - 1;
    else
        return 0;
}

#define tState  data[0]
#define tPtrHi  data[1]
#define tPtrLo  data[2]
#define tAnimId data[3]
#define tBattlerId data[4]
#define tSpeciesId data[5]

// BUG: In vanilla, tPtrLo is read as an s16, so if bit 15 of the
// address were to be set it would cause the pointer to be read
// as 0xFFFFXXXX instead of the desired 0x02YYXXXX.
// By dumb luck, this is not an issue in vanilla. However,
// changing the link order revealed this bug.
#if MODERN || defined(BUGFIX)
#define ANIM_SPRITE(taskId)   ((struct Sprite *)((gTasks[taskId].tPtrHi << 16) | ((u16)gTasks[taskId].tPtrLo)))
#else
#define ANIM_SPRITE(taskId)   ((struct Sprite *)((gTasks[taskId].tPtrHi << 16) | (gTasks[taskId].tPtrLo)))
#endif //MODERN || BUGFIX

static void Task_HandleMonAnimation(u8 taskId)
{
    u32 i;
    struct Sprite *sprite = ANIM_SPRITE(taskId);

    if (gTasks[taskId].tState == 0)
    {
        gTasks[taskId].tBattlerId = sprite->data[0];
        gTasks[taskId].tSpeciesId = sprite->data[2];
        sprite->sDontFlip = TRUE;
        sprite->data[0] = 0;

        for (i = 2; i < ARRAY_COUNT(sprite->data); i++)
            sprite->data[i] = 0;

        sprite->callback = sMonAnimFunctions[gTasks[taskId].tAnimId];
        sIsSummaryAnim = FALSE;

        gTasks[taskId].tState++;
    }
    if (sprite->callback == SpriteCallbackDummy)
    {
        sprite->data[0] = gTasks[taskId].tBattlerId;
        sprite->data[2] = gTasks[taskId].tSpeciesId;
        sprite->data[1] = 0;

        DestroyTask(taskId);
    }
}

void LaunchAnimationTaskForFrontSprite(struct Sprite *sprite, u8 frontAnimId)
{
    u8 taskId = CreateTask(Task_HandleMonAnimation, 128);
    gTasks[taskId].tPtrHi = (u32)(sprite) >> 16;
    gTasks[taskId].tPtrLo = (u32)(sprite);
    gTasks[taskId].tAnimId = frontAnimId;
}

void StartMonSummaryAnimation(struct Sprite *sprite, u8 frontAnimId)
{
    // sDontFlip is expected to still be FALSE here, not explicitly cleared
    sIsSummaryAnim = TRUE;
    sprite->callback = sMonAnimFunctions[frontAnimId];
}

void LaunchAnimationTaskForBackSprite(struct Sprite *sprite, u8 backAnimSet)
{
    u8 nature, taskId, animId, battlerId;

    taskId = CreateTask(Task_HandleMonAnimation, 128);
    gTasks[taskId].tPtrHi = (u32)(sprite) >> 16;
    gTasks[taskId].tPtrLo = (u32)(sprite);

    battlerId = sprite->data[0];
    nature = GetNature(&gPlayerParty[gBattlerPartyIndexes[battlerId]]);

    // * 3 below because each back anim has 3 variants depending on nature
    animId = 3 * backAnimSet + sBackAnimNatureModTable[nature];
    gTasks[taskId].tAnimId = sBackAnimationIds[animId];
}

#undef tState
#undef tPtrHi
#undef tPtrLo
#undef tAnimId
#undef tBattlerId
#undef tSpeciesId

void SetSpriteCB_MonAnimDummy(struct Sprite *sprite)
{
    sprite->callback = MonAnimDummySpriteCallback;
}

static void SetAffineData(struct Sprite *sprite, s16 xScale, s16 yScale, u16 rotation)
{
    u8 matrixNum;
    struct ObjAffineSrcData affineSrcData;
    struct OamMatrix dest;

    affineSrcData.xScale = xScale;
    affineSrcData.yScale = yScale;
    affineSrcData.rotation = rotation;

    matrixNum = sprite->oam.matrixNum;

    ObjAffineSet(&affineSrcData, &dest, 1, 2);
    gOamMatrices[matrixNum].a = dest.a;
    gOamMatrices[matrixNum].b = dest.b;
    gOamMatrices[matrixNum].c = dest.c;
    gOamMatrices[matrixNum].d = dest.d;
}

static void HandleStartAffineAnim(struct Sprite *sprite)
{
    sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
    sprite->affineAnims = sMonAffineAnims;

    if (sIsSummaryAnim == TRUE)
        InitSpriteAffineAnim(sprite);

    if (!sprite->sDontFlip)
        StartSpriteAffineAnim(sprite, 1);
    else
        StartSpriteAffineAnim(sprite, 0);

    CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);
    sprite->affineAnimPaused = TRUE;
}

static void HandleSetAffineData(struct Sprite *sprite, s16 xScale, s16 yScale, u16 rotation)
{
    if (!sprite->sDontFlip)
    {
        xScale *= -1;
        rotation *= -1;
    }

    SetAffineData(sprite, xScale, yScale, rotation);
}

static void TryFlipX(struct Sprite *sprite)
{
    if (!sprite->sDontFlip)
        sprite->x2 *= -1;
}

static bool32 InitAnimData(u8 id)
{
    if (id >= MAX_BATTLERS_COUNT)
    {
        return FALSE;
    }
    else
    {
        sAnims[id].rotation = 0;
        sAnims[id].delay = 0;
        sAnims[id].runs = 1;
        sAnims[id].speed = 0;
        sAnims[id].data = 0;
        return TRUE;
    }
}

static u8 AddNewAnim(void)
{
    sAnimIdx = (sAnimIdx + 1) % MAX_BATTLERS_COUNT;
    InitAnimData(sAnimIdx);
    return sAnimIdx;
}

static void ResetSpriteAfterAnim(struct Sprite *sprite)
{
    sprite->oam.affineMode = ST_OAM_AFFINE_NORMAL;
    CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, sprite->oam.affineMode);

    if (sIsSummaryAnim == TRUE)
    {
        if (!sprite->sDontFlip)
            sprite->hFlip = TRUE;
        else
            sprite->hFlip = FALSE;

        FreeOamMatrix(sprite->oam.matrixNum);
        sprite->oam.matrixNum |= (sprite->hFlip << 3);
        sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
    }
#ifdef BUGFIX
    else
    {
        // FIX: Reset these back to normal after they were changed so Poké Ball catch/release
        // animations without a screen transition in between don't break
        sprite->affineAnims = gAffineAnims_BattleSpriteOpponentSide;
    }
#endif // BUGFIX
}

static void Anim_None(struct Sprite *sprite)
{
	sprite->callback = WaitAnimEnd;
}

static void WaitAnimEnd(struct Sprite *sprite)
{
    if (sprite->animEnded)
        sprite->callback = SpriteCallbackDummy;
}
