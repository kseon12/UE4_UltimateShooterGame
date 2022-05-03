// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/USGAnimInstance.h"
#include "Character/USGCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

////////////////////////////////////////////////////////////

void UUSGAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	USGCharacter = Cast<AUSGCharacter>(TryGetPawnOwner());
}

////////////////////////////////////////////////////////////

void UUSGAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (!USGCharacter)
	{
		USGCharacter = Cast<AUSGCharacter>(TryGetPawnOwner());

		if (!USGCharacter) return;
	}

	FVector Velocity{USGCharacter->GetVelocity()};
	Velocity.Z = 0.0f;

	Speed = Velocity.Size();

	bIsInAir = USGCharacter->GetCharacterMovement()->IsFalling();

	//If acceleration magnitude greater than zero - character is accelerating
	bIsAccelerating = USGCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;
}

////////////////////////////////////////////////////////////
