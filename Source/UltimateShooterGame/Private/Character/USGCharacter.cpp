// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/USGCharacter.h"
#include "Items/BaseItem.h"
#include "Items/Weapon/BaseWeapon.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
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
	ZoomInterpSpeed(20.f),
	//Crosshair default values
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),
	//Bullet fire timer variables
	ShootTimeDuration(0.05f),
	bFiringBullet(false),
	//Automatic gun fire rate
	bFireButtonPressed(false),
	bShouldFire(true),
	AutomaticFireRate(0.1f),
	//Item trace variables
	bShouldTraceForItem(false),
	//Item iterp values
	CameraInterpDistance(250.f),
	CameraInterpElevation(65.f),
	//Starting Ammo
	Starting9mmAmmo(100),
	StartingARAmmo(200)
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

	EquipWeapon(SpawnDefaultWeapon());

	InitializeAmmoMap();
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

	StartCrosshairBulletFire();
}

////////////////////////////////////////////////////////////

bool AUSGCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		OutBeamLocation = CrosshairHitResult.Location;
	}

	//Perform second trace, but now from barrel
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - MuzzleSocketLocation };
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f };

	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);

	//Object between barrel and end point
	if (WeaponTraceHit.bBlockingHit)
	{
		OutBeamLocation = WeaponTraceHit.Location;

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

void AUSGCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	//CrosshairVelocityFactor calculation
	FVector2D WalkSpeedRange{ 0.f, 600.f };
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	//CrosshairInAirFactor calculation
	if (GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	//CrosshairAimFactor
	if (bAiming)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -0.5f, DeltaTime, 10.f);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}

	if (bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	}

	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor + CrosshairShootingFactor;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AUSGCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;

	StartFireTimer();
}

////////////////////////////////////////////////////////////

void AUSGCharacter::FireButtonRelease()
{
	bFireButtonPressed = false;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::StartFireTimer()
{
	if (bShouldFire)
	{
		FireWeapon();
		bShouldFire = false;

		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AUSGCharacter::AutoFireReset, AutomaticFireRate);
	}
}

////////////////////////////////////////////////////////////

void AUSGCharacter::AutoFireReset()
{
	bShouldFire = true;

	if (bFireButtonPressed)
	{
		StartFireTimer();
	}
}

////////////////////////////////////////////////////////////

bool AUSGCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50'000.f };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::TraceForItems()
{
	if (bShouldTraceForItem)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<ABaseItem>(ItemTraceResult.Actor);
			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
			}

			//If item we looking at changed - turn of widget of last item
			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
				}
			}

			//Save new item reference for next frame
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		//Also turn off item widget if we no longer looking at any item
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
	}
}

////////////////////////////////////////////////////////////

ABaseWeapon* AUSGCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<ABaseWeapon>(DefaultWeaponClass);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::EquipWeapon(ABaseWeapon* WeaponToEquip)
{
	if (WeaponToEquip)
	{
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));

		if (HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

////////////////////////////////////////////////////////////

void AUSGCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

////////////////////////////////////////////////////////////

void AUSGCharacter::SelectButtonPressed()
{
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
	}
}

/////////////////////////////////////T///////////////////////

void AUSGCharacter::SelectButtonReleased()
{
}

////////////////////////////////////////////////////////////

void AUSGCharacter::SwapWeapon(ABaseWeapon* Weapon)
{
	DropWeapon();
	EquipWeapon(Weapon);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraInterpZoom(DeltaTime);
	SetLookRates();
	CalculateCrosshairSpread(DeltaTime);

	TraceForItems();
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

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AUSGCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AUSGCharacter::FireButtonRelease);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AUSGCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AUSGCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AUSGCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AUSGCharacter::SelectButtonReleased);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::IncrementOverlappedICount()
{
	++OverlappedItemCount;
	bShouldTraceForItem = true;
}

////////////////////////////////////////////////////////////

void AUSGCharacter::DecrementOverlappedCount()
{
	if (--OverlappedItemCount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItem = false;
	}
}

float AUSGCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

////////////////////////////////////////////////////////////

FVector AUSGCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation{ FollowCamera->GetComponentLocation() };
	const FVector CameraForward{ FollowCamera->GetForwardVector() };

	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.f, 0.f, CameraInterpElevation);
}

////////////////////////////////////////////////////////////

void AUSGCharacter::GetPickupItem(ABaseItem* Item)
{
	ABaseWeapon* Weapon = Cast<ABaseWeapon>(Item);
	if (Weapon)
	{
		SwapWeapon(Weapon);
	}
}

////////////////////////////////////////////////////////////
