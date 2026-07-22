#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RamBedSceneSetupSubsystem.generated.h"

class AHaAnchoredDisplayActor;
class AStadiumScreenActor;
class UWorld;

UCLASS(Config = Game)
class RAMBEDAI_API URamBedSceneSetupSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|HA")
	bool bAutoSpawnHaDashboard = true;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Stadium")
	bool bAutoSpawnStadiumScreen = true;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	bool bSpawnPlayAreaFloor = false;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	bool bSpawnDirectionalLight = false;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	bool bPositionDisplaysNearPlayer = true;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	float ScreenDistanceFromPlayer = 180.0f;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	float DashboardHeightOffset = 130.0f;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	float StadiumHeightOffset = 150.0f;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	float DashboardSideOffset = -90.0f;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Environment")
	float StadiumSideOffset = 90.0f;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Stadium")
	FVector StadiumSpawnLocation = FVector(180.0f, 90.0f, 150.0f);

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Stadium")
	FRotator StadiumSpawnRotation = FRotator(0.0f, 180.0f, 0.0f);

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Stadium")
	float StadiumSpawnScale = 1.5f;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|HA")
	TSubclassOf<AHaAnchoredDisplayActor> HaDisplayClass;

	UPROPERTY(EditAnywhere, Config, Category = "RamBed|Stadium")
	TSubclassOf<AStadiumScreenActor> StadiumScreenClass;

private:
	void HandlePostLoadMap(UWorld* World);
	void SchedulePlayerRelativeSetup(UWorld* World);
	void FinalizeSceneSetup(UWorld* World);
	void EnsureEnvironment(UWorld* World);
	void EnsureHaDashboard(UWorld* World, const FVector& Location, const FRotator& Rotation);
	void EnsureStadiumScreen(UWorld* World, const FVector& Location, const FRotator& Rotation);
	bool TryGetPlayerView(UWorld* World, FVector& OutLocation, FRotator& OutRotation) const;
	static bool IsHaDashboardActor(const AActor* Actor);
	void RemoveMisconfiguredHaDisplays(UWorld* World);

	FDelegateHandle PostLoadMapHandle;
	TSet<TWeakObjectPtr<const UWorld>> ConfiguredWorlds;
	TWeakObjectPtr<AHaAnchoredDisplayActor> SpawnedHaDashboard;
	TWeakObjectPtr<AStadiumScreenActor> SpawnedStadiumScreen;
};