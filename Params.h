#pragma once

enum EParams
{
	kVolume,

	kNoiseType,
	kNoiseAmpMod,
	kNoiseRate,
	kNoiseRange,
	kNoiseShape,

	// parameter for automating the scrub between saved Noise Snapshots
	kNoiseSnapshot,

	kEnvAttack,
	kEnvDecay,
	kEnvSustain,
	kEnvRelease,

	kNumParams,
};

enum EParamSettings
{
	// these are in dB
	kVolumeMin = -48,
	kVolumeMax = 12,
	kVolumeDefault = 0,

	kNoiseSnapshotMin = 0,
	kNoiseSnapshotMax = 7,
	kNoiseSnapshotCount,
	kNoiseSnapshotDefault = 0,
};

enum ENoiseType
{
	NT_White,
	NT_Pink,
	NT_Brown,

	NT_Count,
};