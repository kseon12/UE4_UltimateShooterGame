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

AUSGCharacter::AUSGCharacter():
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f)
{
	PrimaryActorTick.bCanEverTick = true;

	//Create SpringArmComponent 
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 50.f);
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

			GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);

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

void AUSGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AUSGCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AUSGCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AUSGCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AUSGCharacter::LookUpRate);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AUSGCharacter::FireWeapon);
}

////////////////////////////////////////////////////////////
