#include "Wryv.h"
#include "HotSpot.h"
#include "TextWidget.h"

void HotSpot::HotSpotDefaults()
{
  SetName( "HotSpot" );
  //TooltipText = "Tip";

  Align = Alignment::TopLeft;
  Layout = Pixels; // pixel positioning (can also use percentages of parent widths)
  hidden = 0;
  eternal = 1;
  displayTime = FLT_MAX; // Amount of time remaining before removal
  // assumes extremely large number (1e37 seconds which is practically infinite)
  Margin = Pad = FVector2D(0,0);
  Size = FVector2D(32,32);
  Dead = 0;
  Color = FLinearColor::White;
  Parent = 0;
  dirty = 1;
  //bubbleUp = 1; // events by default bubble up thru to the next widget
  AbsorbsMouseUp = 0;

  OnHover = [this](FVector2D mouse) -> EventCode {
    if( !TooltipText.IsEmpty() )
      TooltipWidget->Set( TooltipText );
    return NotConsumed;
  };
}

