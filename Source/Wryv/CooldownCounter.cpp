#include "Wryv.h"
#include "CooldownCounter.h"
#include "WryvGameInstance.h"
#include "GameObject.h"

CooldownCounter::CooldownCounter()
{
  Type = NOTHING;
  Time = 0.f;
  TotalTime = 0.f; // no cooldown by default
  go = 0;
}

CooldownCounter::CooldownCounter( Types type )
{
  Type = type;
  Time = 0.f;
  TotalTime = Game->unitsData[ Type ].TimeLength;
  go = 0;
}
  
CooldownCounter::CooldownCounter( Types type, AGameObject* object )
{
  Type = type;
  Time = 0.f;
  TotalTime = Game->unitsData[ Type ].TimeLength;
  go = object;
}

float CooldownCounter::Percent()
{
  if( TotalTime )
    return Time / TotalTime;
  else
    return 1.f; // always recharged.
}
