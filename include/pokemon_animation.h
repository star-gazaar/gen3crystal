#ifndef GUARD_POKEMON_ANIMATION_H
#define GUARD_POKEMON_ANIMATION_H

u8 GetSpeciesBackAnimSet(u16 species);
void LaunchAnimationTaskForFrontSprite(struct Sprite *sprite, u8 frontAnimId);
void StartMonSummaryAnimation(struct Sprite *sprite, u8 frontAnimId);
void LaunchAnimationTaskForBackSprite(struct Sprite *sprite, u8 backAnimSet);
void SetSpriteCB_MonAnimDummy(struct Sprite *sprite);

// Pokémon back animation sets
#define BACK_ANIM_NONE                         0

// Pokémon animation function ids (for front and back)
// Each front anim uses 1, and each back anim uses a set of 3
#define ANIM_NONE                               0

#endif // GUARD_POKEMON_ANIMATION_H
