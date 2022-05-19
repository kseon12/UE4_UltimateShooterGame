// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/USGPlayerController.h"

#include "Blueprint/UserWidget.h"

////////////////////////////////////////////////////////////


AUSGPlayerController::AUSGPlayerController()
{

}

////////////////////////////////////////////////////////////

void AUSGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if(HUDOverlayClass)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if(HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

////////////////////////////////////////////////////////////

