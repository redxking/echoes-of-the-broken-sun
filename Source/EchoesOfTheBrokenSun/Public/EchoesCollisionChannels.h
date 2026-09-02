#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

// Two questions, two channels.
//
// ECC_Visibility answers "where on the battlefield is the cursor?". Command
// sites read HitResult.Location from that trace as a ground position - build
// placement, formation destinations, ability points, order markers - so the
// only things that may block it are the ground and the entity bodies that
// stand on it. Nothing drawn above an entity is allowed to join it, because a
// hit on a floating overlay would hand those sites a point in the air.
//
// ECC_EchoesEntityPick answers "which entity is under the cursor?". It is
// declared in Config/DefaultEngine.ini as a trace channel named
// EchoesEntityPick whose default response is Ignore, so the world, terrain and
// every unrelated actor fall through it. Only the presentation components that
// represent an entity the player can see block it, which lets context-order
// resolution find an entity over its whole readable footprint without moving
// the ground answer by a single centimetre.
inline constexpr ECollisionChannel ECC_EchoesEntityPick = ECC_GameTraceChannel1;
