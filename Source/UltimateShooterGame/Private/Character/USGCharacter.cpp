// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/USGCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
	CameraBoom->SetupAttachment(RootComponent);

	//Create CameraComponent
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
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
	/** Using function IsNearlyEquil allow to set barrier for joystick drifts*/
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
}

////////////////////////////////////////////////////////////
