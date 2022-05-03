// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "USGAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class ULTIMATESHOOTERGAME_API UUSGAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	UFUNCTION(BlueprintCallable)
	void UpdateAnimationProperties(float DeltaTime);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Movement, meta = (AllowPrivateAccess="true"))
	class AUSGCharacter* USGCharacter;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess="true"))
	float Speed;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess="true"))
	bool bIsInAir;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess="true"))
	bool bIsAccelerating;
};
