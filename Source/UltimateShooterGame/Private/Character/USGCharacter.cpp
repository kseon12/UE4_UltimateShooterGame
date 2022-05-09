// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/USGCharacter.h"

#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

////////////////////////////////////////////////////////////

AUSGCharacter::AUSGCharacter() :
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	HipTurnRate(90.f),
	//Camera FOV values
	HipLookUpRate(90.f),
	AimingTurnRate(30.f),
	AimingLookUpRate(30.f),
	MouseHipTurnRate(1.f),
	//Controller rates for aiming or not
	MouseHipLookUpRate(1.f),
	MouseAimingTurnRate(0.3f),
	MouseAimingLookUpRate(0.3f),
	bAiming(false),
	//Mouse rates for aiming or not
	CameraDefaultFOV(90.f),
	CameraZoomedFOV(35.f),
	CameraCurrentFOV(90.f),
	ZoomInterpSpeed(20.f)
{
	PrimaryActorTick.bCanEverTick = true;

	//Create SpringArmComponent 
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->TargetArmLength = 180.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);
	CameraBoom->SetupAttachment(RootComponent);

	//Create CameraComponent
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
}

////////////////////////////////////////////////////////////

void AUSGCharacter::MoveForward(float Value)
{
	/** Using function IsNearlyEquil allow to set barrier for joystick drifts */
	if (!Controller || FMath::IsNearlyEqual(Value, 0.0f, 0.001f)) return;

	const FRotator Rotation{ Controller->GetControlRotation() };
	const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

	const FVector Direction{ FRotationMatrix{ YawRotation }.GetUnitAxis(EAxis::X) };

	AddMovementInput(Direction, Value);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::MoveRight(float Value)
{
	/** Using function IsNearlyEquil allow to set barrier for joystick drifts*/
	if (!Controller || FMath::IsNearlyEqual(Value, 0.0f, 0.001f)) return;

	const FRotator Rotation{ Controller->GetControlRotation() };
	const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

	const FVector Direction{ FRotationMatrix{ YawRotation }.GetUnitAxis(EAxis::Y) };

	AddMovementInput(Direction, Value);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

////////////////////////////////////////////////////////////

void AUSGCharacter::LookUpRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

////////////////////////////////////////////////////////////

void AUSGCharacter::Turn(float Value)
{
	float TurnScaleFactor;

	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}

	AddControllerYawInput(Value * TurnScaleFactor);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::LookUp(float Value)
{
	float LookUpScaleFactor;

	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value * LookUpScaleFactor);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::FireWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("Bullet is landed"));

	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}

		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);

		if (bBeamEnd)
		{
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd);
			}

			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(), BeamParticles, SocketTransform);
				if (Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

////////////////////////////////////////////////////////////

bool AUSGCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f - 50.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000.f };

		OutBeamLocation = End;
		GetWorld()->LineTraceSingleByChannel(ScreenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);

		if (ScreenTraceHit.bBlockingHit)
		{
			OutBeamLocation = ScreenTraceHit.Location;

			//Perform second trace, but now from barrel
			FHitResult WeaponTraceHit;
			const FVector WeaponTraceStart{ MuzzleSocketLocation };
			const FVector WeaponTraceEnd{ OutBeamLocation };

			GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd,
			                                     ECollisionChannel::ECC_Visibility);

			//Object between barrel and end point
			if (WeaponTraceHit.bBlockingHit)
			{
				OutBeamLocation = WeaponTraceHit.Location;
			}
		}

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::AimingButtonPressed()
{
	bAiming = true;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::AimingButtonReleased()
{
	bAiming = false;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::CameraInterpZoom(float DeltaTime)
{
	if (bAiming)
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingLookUpRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

////////////////////////////////////////////////////////////

void AUSGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraInterpZoom(DeltaTime);
	SetLookRates();
}

////////////////////////////////////////////////////////////

void AUSGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AUSGCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AUSGCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AUSGCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AUSGCharacter::LookUpRate);
	PlayerInputComponent->BindAxis("Turn", this, &AUSGCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AUSGCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AUSGCharacter::FireWeapon);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AUSGCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AUSGCharacter::AimingButtonReleased);
}

////////////////////////////////////////////////////////////
