#include "HaAnchoredDisplayActor.h"

#include "HaDashboardWidget.h"

#include "Blueprint/UserWidget.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AHaAnchoredDisplayActor::AHaAnchoredDisplayActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DisplayFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayFrame"));
	DisplayFrame->SetupAttachment(Root);
	DisplayFrame->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DisplayFrame->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		DisplayFrame->SetStaticMesh(PlaneMesh.Object);
	}
	DisplayFrame->SetRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	DisplayFrame->SetRelativeScale3D(FVector(2.8f, 1.6f, 1.0f));

	DisplayWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DisplayWidget"));
	DisplayWidget->SetupAttachment(DisplayFrame);
	DisplayWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
	DisplayWidget->SetRelativeRotation(FRotator::ZeroRotator);
	DisplayWidget->SetWidgetSpace(EWidgetSpace::World);
	DisplayWidget->SetDrawSize(DisplayDrawSize);
	DisplayWidget->SetTwoSided(true);
	DisplayWidget->SetPivot(FVector2D(0.5f, 0.5f));
	DisplayWidget->SetRedrawTime(0.0f);
	DisplayWidget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DisplayWidget->SetWidgetClass(UHaDashboardWidget::StaticClass());

	LoadConfig();
}

void AHaAnchoredDisplayActor::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* FrameMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this))
		{
			FrameMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.12f, 0.08f, 1.0f));
			DisplayFrame->SetMaterial(0, FrameMaterial);
		}
	}

	InitializeDashboard();
	SetDisplayScale(DefaultDisplayScale);
}

void AHaAnchoredDisplayActor::InitializeDashboard()
{
	if (!DisplayWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("HA Dashboard: DisplayWidget component is missing on %s"), *GetName());
		return;
	}

	DisplayWidget->SetDrawSize(DisplayDrawSize);
	DisplayWidget->SetWidgetSpace(EWidgetSpace::World);
	DisplayWidget->SetTwoSided(true);

	if (!DisplayWidget->GetWidgetClass())
	{
		DisplayWidget->SetWidgetClass(UHaDashboardWidget::StaticClass());
	}

	DisplayWidget->InitWidget();

	UUserWidget* WidgetInstance = DisplayWidget->GetWidget();
	DashboardWidget = Cast<UHaDashboardWidget>(WidgetInstance);
	if (!DashboardWidget)
	{
		UE_LOG(
			LogTemp, Warning,
			TEXT("HA Dashboard: widget class on %s is %s, expected Ha Dashboard Widget"),
			*GetName(),
			WidgetInstance ? *WidgetInstance->GetClass()->GetName() : TEXT("(none)"));
		return;
	}

	if (DashboardUrl.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("HA Dashboard: DashboardUrl is empty on %s"), *GetName());
		return;
	}

	DashboardWidget->LoadDashboardUrl(DashboardUrl);
	UE_LOG(LogTemp, Log, TEXT("HA Dashboard: loading %s on %s"), *DashboardUrl, *GetName());
}

void AHaAnchoredDisplayActor::SetDisplayScale(float UniformScale)
{
	SetActorScale3D(FVector(UniformScale));
}