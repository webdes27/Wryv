#include "Wryv.h"
#include "Projectile.h"
#include "WryvGameInstance.h"
#include "WryvGameMode.h"
#include "GlobalFunctions.h"
#include "FlyCam.h"

AProjectile::AProjectile(const FObjectInitializer& PCIP) : AGameObject(PCIP)
{
  PrimaryActorTick.bCanEverTick = true;
  Shooter = 0;
  Gravity = -10.f;
}

void AProjectile::ai( float t )
{
  // The AI for the spell is basically nothing at this point,
  // though we might want to add spin/curl to some spells
}

void AProjectile::Move( float t )
{
  Vel.Z += Gravity * t;
  Pos += Vel*t;

  Vel.Rotation();
  SetRot( Vel.GetSafeNormal().Rotation() );

  AGameObject::Move( t ); // Calls flush, so we put it last
}

void AProjectile::SetDestinationArc( FVector start, FVector end, float speed, float h )
{
  FVector dir = end - start;
  float len = dir.Size();
  Vel = dir;
  Vel.Z = 0.f;
  Vel.Normalize();
  Vel *= speed; // XY Velocity

  float travelTime = len / speed;
  float t = travelTime/2.f; // Time to reach full height.
  // Simultaneously solve:
  //   v_2^2 = v_1^2 + 2*a*len
  //   h = v_1*t + .5*a*t*t
  Vel.Z = 2.f*h/t;
  Gravity = -Vel.Z*Vel.Z / (2.f*h); // Acceleration for a velocity of 0 @ len/2.

  SetPosition( start );
}