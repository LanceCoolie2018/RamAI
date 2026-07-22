#include "StadiumScreenActor.h"

#include "RamMqttSubsystem.h"
#include "StadiumStreamWidget.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AStadiumScreenActor::AStadiumScreenActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ScreenFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenFrame"));
	ScreenFrame->SetupAttachment(Root);
	ScreenFrame->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScreenFrame->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		ScreenFrame->SetStaticMesh(PlaneMesh.Object);
	}
	ScreenFrame->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	ScreenFrame->SetRelativeScale3D(FVector(3.2f, 1.8f, 1.0f));

	ScreenWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ScreenWidget"));
	ScreenWidget->SetupAttachment(ScreenFrame);
	ScreenWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
	ScreenWidget->SetRelativeRotation(FRotator::ZeroRotator);
	ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
	ScreenWidget->SetDrawSize(ScreenDrawSize);
	ScreenWidget->SetTwoSided(true);
	ScreenWidget->SetPivot(FVector2D(0.5f, 0.5f));
	ScreenWidget->SetRedrawTime(0.0f);
	ScreenWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScreenWidget->SetWidgetClass(UStadiumStreamWidget::StaticClass());

	LoadConfig();
}

void AStadiumScreenActor::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* FrameMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this))
		{
			FrameMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.08f, 0.35f, 1.0f));
			ScreenFrame->SetMaterial(0, FrameMaterial);
		}
	}

	ScreenWidget->InitWidget();
	StreamWidget = Cast<UStadiumStreamWidget>(ScreenWidget->GetWidget());

	bool bLoadedStream = false;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		MqttSubsystem = GameInstance->GetSubsystem<URamMqttSubsystem>();
		if (MqttSubsystem)
		{
			MqttSubsystem->OnStreamChannelChanged.AddDynamic(this, &AStadiumScreenActor::HandleStreamChannelChanged);
			if (MqttSubsystem->HasActiveChannel())
			{
				LoadChannel(MqttSubsystem->GetActiveChannel());
				bLoadedStream = true;
			}
		}
	}

	if (!bLoadedStream && !TestStreamUrl.IsEmpty())
	{
		FRamStreamChannel TestChannel;
		TestChannel.Id = TEXT("test");
		TestChannel.Name = TEXT("Test Stream");
		TestChannel.Url = TestStreamUrl;
		LoadChannel(TestChannel);
	}

	SetScreenScale(DefaultScreenScale);
}

void AStadiumScreenActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MqttSubsystem)
	{
		MqttSubsystem->OnStreamChannelChanged.RemoveDynamic(this, &AStadiumScreenActor::HandleStreamChannelChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void AStadiumScreenActor::HandleStreamChannelChanged(const FRamStreamChannel& Channel)
{
	LoadChannel(Channel);
}

void AStadiumScreenActor::LoadChannel(const FRamStreamChannel& Channel)
{
	if (StreamWidget)
	{
		StreamWidget->LoadStreamUrl(Channel.Url);
		UE_LOG(LogTemp, Log, TEXT("StadiumScreen: loading %s"), *Channel.Name);
	}
}

void AStadiumScreenActor::SetScreenScale(float UniformScale)
{
	SetActorScale3D(FVector(UniformScale));
}