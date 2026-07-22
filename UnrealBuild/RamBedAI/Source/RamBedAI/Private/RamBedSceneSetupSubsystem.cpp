#include "RamBedSceneSetupSubsystem.h"

#include "HaAnchoredDisplayActor.h"
#include "StadiumScreenActor.h"

#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

void URamBedSceneSetupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!HaDisplayClass)
	{
		HaDisplayClass = AHaAnchoredDisplayActor::StaticClass();
	}

	if (!StadiumScreenClass)
	{
		StadiumScreenClass = AStadiumScreenActor::StaticClass();
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &URamBedSceneSetupSubsystem::HandlePostLoadMap);
}

void URamBedSceneSetupSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Super::Deinitialize();
}

void URamBedSceneSetupSubsystem::HandlePostLoadMap(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || !World->IsGameWorld()
		|| World->WorldType == EWorldType::EditorPreview)
	{
		return;
	}

	if (ConfiguredWorlds.Contains(World))
	{
		return;
	}

	ConfiguredWorlds.Add(World);
	EnsureEnvironment(World);
	SchedulePlayerRelativeSetup(World);
}

void URamBedSceneSetupSubsystem::SchedulePlayerRelativeSetup(UWorld* World)
{
	if (!World)
	{
		return;
	}

	FTimerHandle SetupTimer;
	World->GetTimerManager().SetTimer(
		SetupTimer,
		FTimerDelegate::CreateUObject(this, &URamBedSceneSetupSubsystem::FinalizeSceneSetup, World),
		0.75f,
		false);
}

void URamBedSceneSetupSubsystem::FinalizeSceneSetup(UWorld* World)
{
	if (!World)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!TryGetPlayerView(World, ViewLocation, ViewRotation))
	{
		UE_LOG(LogTemp, Warning, TEXT("RamBed setup: player view not ready, using default spawn transforms"));
		if (bAutoSpawnHaDashboard)
		{
			const FVector DashboardLocation = FVector(180.0f, -90.0f, 130.0f);
			const FRotator DashboardRotation = FRotator(0.0f, 0.0f, 0.0f);
			EnsureHaDashboard(World, DashboardLocation, DashboardRotation);
		}
		if (bAutoSpawnStadiumScreen)
		{
			EnsureStadiumScreen(World, StadiumSpawnLocation, StadiumSpawnRotation);
		}
		return;
	}

	const FVector Forward = ViewRotation.Vector().GetSafeNormal2D();
	const FVector Right = FRotationMatrix(ViewRotation).GetScaledAxis(EAxis::Y);

	if (bPositionDisplaysNearPlayer)
	{
		if (bAutoSpawnHaDashboard)
		{
			const FVector DashboardLocation = ViewLocation
				+ (Forward * ScreenDistanceFromPlayer)
				+ (Right * DashboardSideOffset)
				+ FVector(0.0f, 0.0f, DashboardHeightOffset);
			const FRotator DashboardRotation = (ViewLocation - DashboardLocation).Rotation();
			EnsureHaDashboard(World, DashboardLocation, DashboardRotation);
		}
	}

	if (bAutoSpawnStadiumScreen)
	{
		const FVector StadiumLocation = ViewLocation
			+ (Forward * ScreenDistanceFromPlayer)
			+ (Right * StadiumSideOffset)
			+ FVector(0.0f, 0.0f, StadiumHeightOffset);
		const FRotator StadiumRotation = (ViewLocation - StadiumLocation).Rotation();
		EnsureStadiumScreen(World, StadiumLocation, StadiumRotation);
	}
}

bool URamBedSceneSetupSubsystem::IsHaDashboardActor(const AActor* Actor)
{
	return Actor && Actor->IsA<AHaAnchoredDisplayActor>();
}

void URamBedSceneSetupSubsystem::RemoveMisconfiguredHaDisplays(UWorld* World)
{
	if (!World)
	{
		return;
	}

	TArray<AActor*> ActorsToRemove;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("BP_HA_AnchoredDisplay"))
			&& !It->IsA<AHaAnchoredDisplayActor>())
		{
			UE_LOG(
				LogTemp, Warning,
				TEXT("RamBed setup: removing misconfigured %s (not parented from HA Anchored Display C++)"),
				*It->GetName());
			ActorsToRemove.Add(*It);
		}
	}

	for (AActor* Actor : ActorsToRemove)
	{
		Actor->Destroy();
	}
}

bool URamBedSceneSetupSubsystem::TryGetPlayerView(
	UWorld* World, FVector& OutLocation, FRotator& OutRotation) const
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		PlayerController->GetPlayerViewPoint(OutLocation, OutRotation);
		return true;
	}

	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		OutLocation = PlayerPawn->GetActorLocation();
		OutRotation = PlayerPawn->GetViewRotation();
		return true;
	}

	return false;
}

void URamBedSceneSetupSubsystem::EnsureEnvironment(UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (bSpawnPlayAreaFloor)
	{
		bool bHasFloor = false;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (It->GetName().Contains(TEXT("RamBedFloor")))
			{
				bHasFloor = true;
				break;
			}
		}

		if (!bHasFloor)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = FName(TEXT("RamBedFloor"));
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(
				AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams))
			{
				if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
					nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
				{
					Floor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
					Floor->SetActorScale3D(FVector(8.0f, 8.0f, 0.1f));
					Floor->SetActorLocation(FVector(0.0f, 0.0f, -10.0f));
					UE_LOG(LogTemp, Log, TEXT("RamBed setup: spawned play-area floor"));
				}
			}
		}
	}

	if (bSpawnDirectionalLight)
	{
		bool bHasSun = false;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			bHasSun = true;
			break;
		}

		if (!bHasSun)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 500.0f), FRotator(-45.0f, 45.0f, 0.0f), SpawnParams))
			{
				UE_LOG(LogTemp, Log, TEXT("RamBed setup: spawned directional light"));
			}
		}
	}
}

void URamBedSceneSetupSubsystem::EnsureHaDashboard(
	UWorld* World, const FVector& Location, const FRotator& Rotation)
{
	if (!bAutoSpawnHaDashboard || !HaDisplayClass || !World)
	{
		return;
	}

	RemoveMisconfiguredHaDisplays(World);

	if (SpawnedHaDashboard.IsValid())
	{
		SpawnedHaDashboard->SetActorLocationAndRotation(Location, Rotation);
		SpawnedHaDashboard->SetDisplayScale(1.5f);
		SpawnedHaDashboard->InitializeDashboard();
		UE_LOG(LogTemp, Log, TEXT("RamBed setup: repositioned HA dashboard to %s"), *Location.ToString());
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (IsHaDashboardActor(*It))
		{
			SpawnedHaDashboard = Cast<AHaAnchoredDisplayActor>(*It);
			It->SetActorLocationAndRotation(Location, Rotation);
			if (AHaAnchoredDisplayActor* HaDisplay = Cast<AHaAnchoredDisplayActor>(*It))
			{
				HaDisplay->SetDisplayScale(1.5f);
				HaDisplay->InitializeDashboard();
			}
			UE_LOG(LogTemp, Log, TEXT("RamBed setup: positioned HA dashboard at %s"), *Location.ToString());
			return;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHaAnchoredDisplayActor* HaDashboard = World->SpawnActor<AHaAnchoredDisplayActor>(
		HaDisplayClass, Location, Rotation, SpawnParams);

	if (!HaDashboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("RamBed setup: failed to spawn HA dashboard"));
		return;
	}

	SpawnedHaDashboard = HaDashboard;
	HaDashboard->SetDisplayScale(1.5f);
	HaDashboard->InitializeDashboard();
	UE_LOG(LogTemp, Log, TEXT("RamBed setup: spawned HA dashboard at %s"), *Location.ToString());
}

void URamBedSceneSetupSubsystem::EnsureStadiumScreen(
	UWorld* World, const FVector& Location, const FRotator& Rotation)
{
	if (!bAutoSpawnStadiumScreen || !StadiumScreenClass || !World)
	{
		return;
	}

	if (SpawnedStadiumScreen.IsValid())
	{
		SpawnedStadiumScreen->SetActorLocationAndRotation(Location, Rotation);
		SpawnedStadiumScreen->SetScreenScale(StadiumSpawnScale);
		UE_LOG(LogTemp, Log, TEXT("RamBed setup: repositioned stadium screen to %s"), *Location.ToString());
		return;
	}

	for (TActorIterator<AStadiumScreenActor> It(World); It; ++It)
	{
		SpawnedStadiumScreen = *It;
		It->SetActorLocationAndRotation(Location, Rotation);
		It->SetScreenScale(StadiumSpawnScale);
		UE_LOG(LogTemp, Log, TEXT("RamBed setup: repositioned existing stadium screen to %s"), *Location.ToString());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStadiumScreenActor* StadiumScreen = World->SpawnActor<AStadiumScreenActor>(
		StadiumScreenClass, Location, Rotation, SpawnParams);

	if (!StadiumScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("RamBed setup: failed to spawn stadium screen"));
		return;
	}

	SpawnedStadiumScreen = StadiumScreen;
	StadiumScreen->SetScreenScale(StadiumSpawnScale);
	UE_LOG(LogTemp, Log, TEXT("RamBed setup: spawned stadium screen at %s"), *Location.ToString());
}

