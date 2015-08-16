#include "Wryv.h"
#include "FlyCam.h"
#include "Resource.h"
#include "Building.h"
#include "PlayerControl.h"
#include "TheHUD.h"
#include "Unit.h"
#include "ItemShop.h"
#include "UnitsData.h"
#include "WryvGameInstance.h"
#include "WryvGameMode.h"
#include "Spell.h"
#include "GlobalFunctions.h"
#include "Widget.h"
#include "DrawDebugHelpers.h"
#include "CombatUnit.h"
#include "Widget3D.h"

#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"
//#include "Editor/UnrealEd/Public/AssetThumbnail.h"

ATheHUD::ATheHUD(const FObjectInitializer& PCIP) : Super(PCIP)
{
  LOG( "ATheHUD::ATheHUD(ctor)");
  selectorShopPatron = 0;
  NextAction = NextBuilding = NOTHING;
  WillSelectNextFrame = 0;
  Init = 0; // Have the slots & widgets been initialied yet? can only happen
  // in first call to draw.
}

void ATheHUD::BeginPlay()
{
  Super::BeginPlay();
  LOG( "ATheHUD::BeginPlay()");
}

TArray<FAssetData> ATheHUD::ScanFolder( FName folder )
{
  // Create an AssetRegistry to list folder data
  FAssetRegistryModule& AssetRegistry =
    FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
    FName("AssetRegistry"));
  
  // Switch into the SubPaths folder. Required for 
  // GetAssetsByPath to work properly (even if we don't use the pathList)
  TArray<FString> pathList;
  AssetRegistry.Get().GetSubPaths( folder.ToString(), pathList, 1 );
  TArray<FAssetData> assets;
  AssetRegistry.Get().GetAssetsByPath( folder, assets );
  for( int i = 0; i < assets.Num(); i++ )
  {
    LOG( "%s", *assets[i].AssetName.ToString() );
  }
  return assets;
}

EventCode ATheHUD::BeginBoxSelect( FVector2D mouse ){
  // Set the opening of the select box
  ui->mouseCursor->SelectStart( mouse );
  return NotConsumed;
}

EventCode ATheHUD::Hover( FVector2D mouse ){
  // Regular hover event
  ui->mouseCursor->Set( mouse );
  return NotConsumed;
}

// Mouse motion with mouse down.
EventCode ATheHUD::DragBoxSelect( FVector2D mouse ){
  ui->mouseCursor->DragBox( mouse );
  return NotConsumed;
}

// Mouse up event, after having Dragged 
EventCode ATheHUD::EndBoxSelect( FVector2D mouse ){
  // Cannot select here, must in render call.
  WillSelectNextFrame = 1;
  if( Game->pc->IsAnyKeyDown( { EKeys::LeftShift, EKeys::RightShift } ) )
    selectionParity = Adds;
  else if( Game->pc->IsAnyKeyDown( { EKeys::LeftControl, EKeys::RightControl } ) )
    selectionParity = Subtracts;
  else
    selectionParity = New;
  ui->mouseCursor->SelectEnd();
  return NotConsumed;
}

set<AGameObject*> ATheHUD::Select( set<AGameObject*> objects )
{
  NextAction = NOTHING; // Unset next spell & building
  NextBuilding = NOTHING;
  DestroyAll( selectors );  // Destroy the old selectors
  DestroyAll( selAttackTargets );
  // create & parent the selectors to all selected objects
  for( AGameObject * go : objects )
  {
    AWidget3D* sel = Cast<AWidget3D>( go->MakeChild( Types::UISELECTOR ) );
    if( !sel )  error( "Couldn't create selector object" );
    selectors.push_back( sel );
    // make an attack target if there is an attack target for the gameobject
    if( go->AttackTarget )
    {
      AWidget3D* att = Cast<AWidget3D>( go->MakeChild( Types::UIATTACKSELECTOR ) );
      selAttackTargets.push_back( att );
    }
  }

  // Modify the UI to reflect selected gameobjects
  ui->gameChrome->Select( objects );
  return objects;
}

set<AGameObject*> ATheHUD::Pick( FBox2DU box )
{
  // Give CombatUnits priority.
  TArray<AActor*> selectedActors;
  GetActorsInSelectionRectangle( TSubclassOf<ACombatUnit>( ACombatUnit::StaticClass() ),
    box.TL(), box.BR(), selectedActors );
  info( FS( "Selected %d CombatUnits", selectedActors.Num() ) );

  // If you've selected buildings & combat units, you select only the combat units.
  // Else, if there are no combatunits selected, you've selected buildings.
  if( !selectedActors.Num() )
  {
    // Try & select buildings
    GetActorsInSelectionRectangle( TSubclassOf< ABuilding >( ABuilding::StaticClass() ),
      box.TL(), box.BR(), selectedActors );
  }

  // Make a new set of selected units (don't use previously selected units)
  set<AGameObject*> selected;
  for( int i = 0; i < selectedActors.Num(); i++ )
    if( AGameObject* go = Cast<AGameObject>( selectedActors[i] ) )
      selected.insert( go );
  return selected;
}

void ATheHUD::Unselect( AGameObject* go )
{
  // check if parent of any selector
  for( int i = selectors.size()-1; i >= 0; i-- )
  {
    if( go->isParentOf( selectors[i] ) )
    {
      selectors[i]->Destroy();
      removeIndex( selectors, i );
    }
  }
}

void ATheHUD::UnselectAsTarget( AGameObject* go )
{
  for( int i = selAttackTargets.size()-1; i >= 0; i-- )
  {
    if( go->isParentOf( selAttackTargets[i] ) )
    {
      selAttackTargets[i]->Destroy();
      removeIndex( selAttackTargets, i );
    }
  }
}

EventCode ATheHUD::TogglePause()
{
  // pauses or unpauses the game
  Game->pc->SetPause( !Game->pc->IsPaused() );
  if( Game->pc->IsPaused() )
    ui->gameChrome->controls->Pause->Tex = ResumeButtonTexture;
  else
    ui->gameChrome->controls->Pause->Tex = PauseButtonTexture;
  return Consumed;
}

void ATheHUD::InitWidgets()
{
  if( Init ) return;
  Init = 1;
  LOG( "InitWidgets()" );

  // Initialize the widgets that show the player gold, lumber, stone counts.
  ResourcesWidget::GoldTexture = GoldIconTexture;
  ResourcesWidget::LumberTexture = LumberIconTexture;
  ResourcesWidget::StoneTexture = StoneIconTexture;

  SolidWidget::SolidWhiteTexture = SolidWhiteTexture;
  SlotPalette::SlotPaletteTexture = SlotPaletteTexture;
  StackPanel::StackPanelTexture = StackPanelTexture;
  AbilitiesPanel::BuildButtonTexture = BuildButtonTexture;
  ImageWidget::NullTexture = NullTexture;

  ui = new UserInterface(FVector2D(Canvas->SizeX, Canvas->SizeY));
  GameChrome *gChrome = ui->gameChrome;

  // Main UI functions, which connect to HUD code.
  ui->OnMouseDownLeft = [this]( FVector2D mouse ){ return BeginBoxSelect( mouse ); };
  ui->OnHover = [this]( FVector2D mouse ) { return Hover( mouse ); };
  ui->OnMouseDragLeft = [this]( FVector2D mouse ) { return DragBoxSelect( mouse ); };
  ui->OnMouseUpLeft = [this]( FVector2D mouse ){ return EndBoxSelect( mouse ); };

  gChrome->rightPanel = new SidePanel( RightPanelTexture, PortraitTexture,
    MinimapTexture, FVector2D( 280, Canvas->SizeY ), FVector2D(8,8) );
  gChrome->rightPanel->Align = TopRight;
  gChrome->Add( gChrome->rightPanel );
  
  Controls *controls = gChrome->controls = new Controls( PauseButtonTexture );
  gChrome->rightPanel->Add( controls );
  controls->Align = Top | ToLeftOfParent;
  controls->Margin = FVector2D( 4, - gChrome->rightPanel->Pad.Y );
  controls->Pause->OnMouseDownLeft = [this](FVector2D mouse){
    return TogglePause();
  };
  
  // Attach functionality for minimap
  Minimap* minimap = gChrome->rightPanel->minimap;
  minimap->OnMouseDownLeft = [minimap](FVector2D mouse){
    Game->flycam->SetCameraPosition( mouse / minimap->Size );
    return Consumed;
  };
  minimap->OnMouseDragLeft = [minimap](FVector2D mouse){
    Game->flycam->SetCameraPosition( mouse / minimap->Size );
    return Consumed;
  };

  // Keep one of these for showing costs on flyover
  gChrome->costWidget = new CostWidget( TooltipBackgroundTexture );
  gChrome->Add( gChrome->costWidget );
  gChrome->costWidget->Align = CenterCenter;
  gChrome->costWidget->Hide();

  ITextWidget* tooltip = gChrome->tooltip = new ITextWidget( "tooltip", 
    TooltipBackgroundTexture, "tip", Alignment::CenterCenter );
  tooltip->Pad = FVector2D( 8, 8 );
  gChrome->Add( gChrome->tooltip );
  tooltip->Hide();
  tooltip->OnMouseDownLeft = [tooltip](FVector2D mouse){
    LOG( "Hiding the tooltip" );
    tooltip->Hide();
    return Consumed;
  };

  gChrome->buffs = new Buffs( "Buffs", 0 );
  gChrome->Add( gChrome->buffs );

  // buildQueue appears
  gChrome->buildQueue = new BuildQueue( "Building Queue", FVector2D( 128, 128 ) );
  gChrome->Add( gChrome->buildQueue );
  //gChrome->buildQueue->Hide();

  // Create the panel for containing items/inventory
  gChrome->itemBelt = new ItemBelt( SlotPaletteTexture, 1, 4, FVector2D( 100, 100 ), FVector2D( 8, 8 ) );
  gChrome->Add( gChrome->itemBelt );

  // Map selection screen
  MapSelectionScreen *mss = ui->mapSelectionScreen = 
    new MapSelectionScreen( TitleLogoTexture, SolidWhiteTexture,
      MapSlotEntryBackgroundTexture, PortraitTexture,
      FVector2D( 120, 24 ), largeFont );

  // Construct the other screens
  ui->titleScreen = new TitleScreen( TitleScreenTexture );
  
  ui->mapSelectionScreen->Align = CenterCenter;
  ui->Add( ui->mapSelectionScreen );
  ui->mapSelectionScreen->OKButton->OnMouseDownLeft = [mss](FVector2D mouse){
    // OK button clicked, so load the map if there is a selected widget
    // else display error message
    if( mss->Selected )
      Game->flycam->LoadLevel( FName( *mss->Selected->GetText() ) );
    else
      Game->hud->ui->statusBar->Set( "Select a map to load first" ) ;
    return NotConsumed;
  };

  /////
  // List the maps in the folder at the left side
  TArray<FAssetData> maps = ScanFolder( "/Game/Maps" );

  for( int i = 0; i < maps.Num(); i++ )
    ui->mapSelectionScreen->AddText( maps[i].AssetName.ToString(), CenterCenter );

  ui->missionObjectivesScreen = new MissionObjectivesScreen(
    MapSlotEntryBackgroundTexture, MapSlotEntryBackgroundTexture, 
    FVector2D( 300, 100 ), FVector2D( 8, 8 ) );
  ui->Add( ui->missionObjectivesScreen );

  // Add the mouseCursor last so that it renders last.
  MouseWidget *mousePointer = ui->mouseCursor = new MouseWidget( "mouse cursor", MouseCursorHand.Texture );
  ui->Add( ui->mouseCursor );

  /// Set the screen's to correct one for the gamestate
  ui->SetScreen( Game->gm->state );

}

void ATheHUD::Setup()
{
  LOG( "ATheHUD::Setup()");
  rendererIcon = GetComponentByName<USceneCaptureComponent2D>( this, "rendererIcon" );
  rendererMinimap = GetComponentByName<USceneCaptureComponent2D>( this, "rendererMinimap" );
}

void ATheHUD::UpdateDisplayedResources()
{
  // spent=200, diff=-200
  //    diff = 200 - 400;
  float diff = Game->gm->playersTeam->Gold - displayedGold;
  diff *= 0.1; // jump by 10%
  displayedGold += diff;

  diff = Game->gm->playersTeam->Lumber - displayedLumber;
  diff *= 0.1; // jump by 10%
  displayedLumber += diff;

  diff = Game->gm->playersTeam->Stone - displayedStone;
  diff *= 0.1; // jump by 10%
  displayedStone += diff;

  ui->gameChrome->resources->SetValues( FMath::RoundToInt( displayedGold ),
    FMath::RoundToInt( displayedLumber ), FMath::RoundToInt( displayedStone ) );
}

void ATheHUD::DrawMaterial(UCanvas* Canvas, UMaterialInterface* Material, float ScreenX, float ScreenY, 
  float ScreenW, float ScreenH, float MaterialU, float MaterialV, float MaterialUWidth, float MaterialVHeight)
{
  FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Material->GetRenderProxy(0), 
    FVector2D( ScreenW, ScreenH ) * 1.f, FVector2D( MaterialU, MaterialV ), 
    FVector2D( MaterialU + MaterialUWidth, MaterialV + MaterialVHeight ) );
	TileItem.Rotation = FRotator(0, 0, 0);
	TileItem.PivotPoint = FVector2D::ZeroVector;
	Canvas->DrawItem( TileItem );
}

void ATheHUD::DrawMaterial(UCanvas* Canvas, UMaterialInterface* Material, float ScreenX, float ScreenY, 
  float ScreenW, float ScreenH, float MaterialU, float MaterialV, float MaterialUWidth, float MaterialVHeight, 
  float Scale, bool bScalePosition, float Rotation, FVector2D RotPivot)
{
  FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Material->GetRenderProxy(0), 
    FVector2D( ScreenW, ScreenH ) * Scale, FVector2D( MaterialU, MaterialV ), 
    FVector2D( MaterialU + MaterialUWidth, MaterialV + MaterialVHeight ) );
	TileItem.Rotation = FRotator(0, Rotation, 0);
	TileItem.PivotPoint = RotPivot;
	Canvas->DrawItem( TileItem );
}

void ATheHUD::DrawTexture(UCanvas* canvas, UTexture* Texture, float ScreenX, float ScreenY,
  float ScreenW, float ScreenH, float TextureU, float TextureV,
  float TextureUWidth, float TextureVHeight, FLinearColor Color,
  EBlendMode BlendMode, float Scale, bool bScalePosition, float Rotation, FVector2D RotPivot)
{
	if (IsCanvasValid_WarnIfNot() && Texture)
	{
		FCanvasTileItem TileItem( FVector2D( ScreenX, ScreenY ), Texture->Resource, FVector2D( ScreenW, ScreenH ) * Scale, FVector2D( TextureU, TextureV ), FVector2D( TextureU + TextureUWidth, TextureV + TextureVHeight ), Color );
		TileItem.Rotation = FRotator(0, Rotation, 0);
		TileItem.PivotPoint = RotPivot;
		if (bScalePosition)
		{
			TileItem.Position *= Scale;
		}
		TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend( BlendMode );
		Canvas->DrawItem( TileItem );
	}
}

void ATheHUD::DrawPortrait()
{
  // Draws the portrait of the first selected object
  if( Selected.size() )
  {
    AGameObject* selected = *Selected.begin();
    // Portrait: render last-clicked object to texture zoom back by radius of bounding sphere of clicked object
    FVector camDir( .5f, .5f, -FMath::Sqrt( 2.f ) );
    RenderScreen( rendererIcon, PortraitTexture, 
      selected->Pos, selected->GetBoundingRadius(), camDir );
  }
}

void ATheHUD::DrawHUD()
{
  // Canvas is only initialized here.
  Super::DrawHUD();
  HotSpot::hud = this;
  InitWidgets();

  if( WillSelectNextFrame )
  {
    set<AGameObject*> selected = Pick( ui->mouseCursor->selectBox->Box );
    if( selectionParity == New )  Selected = selected;
    else if( selectionParity == Adds )  Selected += selected;
    else if( selectionParity == Subtracts )  Selected -= selected;
    Select( Selected );
    WillSelectNextFrame = 0;
  }

  UpdateDisplayedResources();
  DrawPortrait();
  
  // Render the minimap, only if the floor is present
  FBox box = Game->flycam->floorBox;
  FVector p = box.GetCenter();
  RenderScreen( rendererMinimap, MinimapTexture, p, box.GetExtent().GetMax(), FVector( 0, 0, -1 ) );
  
  ui->SetSize( FVector2D( Canvas->SizeX, Canvas->SizeY ) );
  ui->Move( Game->gm->T ); // Ticked here, in case reflow is needed
  ui->render();
}

HotSpot* ATheHUD::MouseDownLeft( FVector2D mouse )
{
  LOG( "ATheHUD::MouseDownLeft()");
  return ui->MouseDownLeft( mouse );
}

HotSpot* ATheHUD::MouseUpLeft( FVector2D mouse )
{
  LOG( "ATheHUD::MouseUpLeft()");
  return ui->MouseUpLeft( mouse );
}

HotSpot* ATheHUD::MouseDownRight( FVector2D mouse )
{
  LOG( "ATheHUD::MouseDownRight()");
  return ui->MouseDownRight( mouse );
}

HotSpot* ATheHUD::MouseUpRight( FVector2D mouse )
{
  // Right up isn't handled.
  return ui->MouseUpRight( mouse );
}

HotSpot* ATheHUD::MouseMoved( FVector2D mouse )
{
  if( !Init )  return 0;

  // Drag events are when the left mouse button is down.
  if( Game->pc->IsKeyDown( EKeys::LeftMouseButton ) )
    return ui->MouseDraggedLeft( mouse );
  else
    return ui->Hover( mouse );  // Check against all ui widgets that require hover.

  return 0;
}

void ATheHUD::Tick( float t )
{
  Super::Tick( t );
}

// Returns to you the distance that you should set
// the camera away from tt to get a render on tt
// that has RadiusOnScreen
float ATheHUD::GetZDistance( float radiusWorldUnits, float radiusOnScreenPx, float texW, float fovyDegrees )
{
  // ZBack = radiusWorldUnits*screenWidthPX /
  //         (tan( fovy / 2 )*radiusOnScreenPX)
  float den = tanf( FMath::DegreesToRadians( fovyDegrees / 2.f ) ) * radiusOnScreenPx;
  return radiusWorldUnits*texW / den;
}

float ATheHUD::GetPxWidth( float radiusWorldUnits, float distanceToObject, float texW, float fovyDegrees )
{
  // radiusOnScreenPX = radiusWorldUnits*screenWidthPX / 
  //                    (tan( fovy / 2 )*distanceToObject)
  float den = tanf( FMath::DegreesToRadians( fovyDegrees / 2.f ) )*distanceToObject;
  return radiusWorldUnits*texW / den;
}

// Render onto tt (using renderer) sitting @ cameraPos,
// facing cameraDir, an object with radiusWorldUnits.
void ATheHUD::RenderScreen( USceneCaptureComponent2D* renderer,
  UTextureRenderTarget2D* tt, FVector objectPos, float radiusWorldUnits,
  FVector cameraDir )
{
  renderer->TextureTarget = tt;
  // http://stackoverflow.com/questions/3717226/
  // radiusOnScreenPX = radiusWorldUnits*SW/(tan(fov / 2) * Z);
  // ZBack = radiusWorldUnits*SW/(tan( fovy / 2 )*radiusOnScreenPX)
  // Calculate Z distance back for a given pixel radius
  // Set particular render properties & render the screen
  // to texture in w.
  float D = GetZDistance( radiusWorldUnits, tt->GetSurfaceWidth(),
    tt->GetSurfaceWidth(), renderer->FOVAngle );
  FVector renderPos = objectPos - cameraDir * D;
  renderer->SetRelativeLocationAndRotation( renderPos, cameraDir.Rotation().Quaternion() );

  for( int i = 0; i < renderer->AttachChildren.Num(); i++ )
  {
    USceneComponent* c = renderer->AttachChildren[i];
    LOG( "Component [%d] = %s", i, *renderer->AttachChildren[i]->GetName() );
  }
}

void ATheHUD::BeginDestroy()
{
  LOG( "ATheHUD::BeginDestroy()");
  if( ui ) {
    delete ui;
    ui = 0;
  }
  Super::BeginDestroy();  // PUT THIS LAST or the object may become invalid
}

