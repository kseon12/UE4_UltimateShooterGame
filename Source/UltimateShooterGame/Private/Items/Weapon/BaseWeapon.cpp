// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/BaseWeapon.h"

////////////////////////////////////////////////////////////

ABaseWeapon::ABaseWeapon():
	ThrowWeaponTime(0.7f),
	bIsFalling(false),
	Ammo(0),
	MagazineCapacity(30),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9mm),
	ReloadMontageSection(FName(TEXT("ReloadSMG")))
{
	PrimaryActorTick.bCanEverTick = true;
}

////////////////////////////////////////////////////////////

void ABaseWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetItemState() == EItemState::EIS_Falling && bIsFalling)
	{
		const FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

////////////////////////////////////////////////////////////

void ABaseWeapon::ThrowWeapon()
{
	FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	FVector MeshForward{ GetItemMesh()->GetForwardVector() };
	const FVector MeshRight{ GetItemMesh()->GetRightVector() };
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

	float RandomRotation = FMath::FRandRange(10.f, 50.f);

	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
	ImpulseDirection *= 20'000.f;
	GetItemMesh()->AddImpulse(ImpulseDirection);

	bIsFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &ABaseWeapon::StopFalling, ThrowWeaponTime);
}

////////////////////////////////////////////////////////////

void ABaseWeapon::DecrementAmmo()
{
	--Ammo;

	// Ammo can't be less than zero
	if (Ammo < 0)
	{
		Ammo = 0;
	}
}

////////////////////////////////////////////////////////////

void ABaseWeapon::ReloadAmmo(int32 Amount)
{
	Ammo += Amount;
	if(Ammo> MagazineCapacity)
	{
		Ammo = MagazineCapacity;
	}
}

////////////////////////////////////////////////////////////

void ABaseWeapon::StopFalling()
{
	bIsFalling = false;
	SetItemState(EItemState::EIS_Pickup);
}

////////////////////////////////////////////////////////////
