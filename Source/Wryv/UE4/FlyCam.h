#pragma once

#include <vector>
#include <set>
using namespace std;

#include <GameFramework/Pawn.h>

#include "Game/Enums.h"
#include "Game/FogOfWar.h"
#include "Util/SoundEffect.h"
#include "FlyCam.generated.h"

class ABuilding;
struct Edge;
struct GraphNode;
class AGameObject;
class AGroundPlane;
class Pathfinder;
class ATheHUD;
class APlayerControl;
class AWidget3D;

inline bool operator<( const FLinearColor& c1, const FLinearColor& c2 )
{
  return c1.Quantize().AlignmentDummy < c2.Quantize().AlignmentDummy;
}

UCLASS()
class WRYV_API AFlyCam : public APawn
{
	GENERATED_BODY()
public:
  AGroundPlane* floor; // Every level must have an object called the floor.
  TSubclassOf<AShape> FloorPlaneBackupClass; // This should be a simple unit cube.

  ABuilding* ghost;    // ghosts of pending buildings being set for placement
  //vector<ABuilding*> ghosts; // Shift+Click allows series of buildings for placement

  Pathfinder* pathfinder;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Cam )  USceneComponent* DummyRoot;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Cam )  UCameraComponent* MainCamera; // UPROPERTY type listing doesn't make it appear in listing
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Cam )  UCameraComponent* OrthoCam;
  AFogOfWar* fogOfWar;
  
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Pathfinding )  int32 Rows;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Pathfinding )  int32 Cols;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Pathfinding )  bool VizGrid;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Pathfinding )  bool VizPassibles;

  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Pathfinding )  float CheckerSphereRadius;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Doodads )  TSubclassOf<AShape> VizClass;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Doodads )  TSubclassOf<AShape> CheckerClass;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Doodads )  TSubclassOf<AShape> LineClass;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Doodads )  TSubclassOf<AShape> WaypointFlagClass;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Doodads )  TSubclassOf<AWidget3D> SelectorClass;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Doodads )  TSubclassOf<AShape> GroundMarkerClass;

  // Use a TARRAY to keep sfx organized, includes music & effects
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Sounds )  TArray<FSoundEffect> SFX;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Colors )  UMaterialInterface* BaseWhiteInterface;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Colors )  FLinearColor BlockedColor;

  //UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = Colors )  UMaterial* BaseWhiteMaterial;
  UPROPERTY() TArray<UMaterialInstanceDynamic*> ColorMaterials; //Reference counted collection of created colors.
  map<FLinearColor, UMaterialInstanceDynamic*> Colors;
  AShape* GroundMarker;
  
  // the name of the title level
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = UOptions )  FName LevelMap;

  UAudioComponent* music;
  float sfxVolume; // volume at which new SFX are played
  
  // this flag indicates if the level has been initialized yet
  bool setupLevel;
  UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = UOptions )
  float CameraMovementSpeed;
  
  UFloatingPawnMovement *MovementComponent;
  UInputComponent* InputComponent;
  vector<AGameObject*> viz;
  static FName AttackTargetName;
  static FName FollowTargetName;
  static FName SelectedTargetName;

  // Sets default values for this pawn's properties
	AFlyCam( const FObjectInitializer& PCIP );
  
  virtual void BeginPlay() override;

  /// Tells you if a building is being placed by the UI
  /// by the `ghost` variable not being NULL.
  /// If it is NULL, then NO building is being placed.
  bool IsBuildingBeingPlaced() { return ghost != NULL; }
	  
  // Called to bind functionality to input.
  // SetupPlayerInputComponent only exists in APawn, so we attach
  // functionality to each input component
  virtual void SetupPlayerInputComponent( UInputComponent* InputComponent ) override;
  void MarkAsSelected( AGameObject* object );
  void MarkAsFollow( AGameObject* object );
  void MarkAsAttack( AGameObject* object );

  void ClearGhost();
  // Start to load the map in levelName
  void LoadLevel( FName levelName );
  void OnLevelLoaded();
  void SetObjectsOnGround();
  void InitializePathfinding();
  void UnloadLevel();
  
  void SetCameraPosition( FVector2D perc );
  UMaterialInterface* GetMaterial( FLinearColor color );
  // Series of points to visualize
  void Visualize( FVector& v, float s, FLinearColor color, float time );
  void Visualize( vector<FVector>& v, float s, FLinearColor startColor, FLinearColor endColor, float time );
  void DrawDebug( Ray ray, float size, FLinearColor color, float time );
  void DrawDebug( FVector pt, float size, FLinearColor color, float time );
  void DrawDebug( FVector start, FVector end, float thickness, FLinearColor color, float time );
  void ClearViz();
  AGameObject* MakeLine( FVector Start, FVector End, FLinearColor color );
  
  void RetrievePointers();
  void debug( int slot, FColor color, FString mess );
  
  // Picks from screen coords, closest object
  AGameObject* Pick( const FVector2D& screenCoords );
  FVector2D GetMousePos();
  // Modifies vector if can hit ground. If not, it just stays floating.
  bool SetOnGround( FVector& v );
  
  void FindFloor();
  vector<FVector> GenerateGroundPositions( FVector P, int numGridPos );
  void Target();
  void MouseUpLeft();
  void MouseDownLeft();
  void MouseUpRight();
  void MouseDownRight();
  void MouseMoved();
  void MouseMovedX( float amount );
  void MouseMovedY( float amount );
  
  void MoveCameraZUp( float amount );
  void MoveCameraZDown( float amount );
  void MoveCameraPitchUp( float amount );
  void MoveCameraPitchDown( float amount );
  void MoveCameraRotateCW( float amount );
  void MoveCameraRotateCCW( float amount );
  void MoveForward( float amount );
  void MoveBack( float amount );
  void MoveLeft( float amount );
  void MoveRight( float amount );
  void PlaySound( UISounds sound ){
    UGameplayStatics::SpawnSoundAttached( SFX[ sound ].Sound, RootComponent );
  }

	virtual void Tick( float t ) override;
	
};
