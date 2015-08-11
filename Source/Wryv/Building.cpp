#include "Wryv.h"
#include "Building.h"

ABuilding::ABuilding( const FObjectInitializer& PCIP ) : AGameObject(PCIP)
{
  buildProgress = 0.f;
}

void ABuilding::Tick( float t )
{
  buildProgress += t;
  if( buildProgress < Stats.BuildTime )
  {
    // Building not complete yet, so increase HP and
    float perc = t / Stats.BuildTime;

    // increase HP by that fraction
    Hp += perc * Stats.HpMax;
    // The building cannot do anything else while building.
    //return;
  }
}
