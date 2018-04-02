#pragma once

enum EParams
{
	kVolume,

	kNoiseType,
	kNoiseAmpMod,
	kNoiseRate,
	kNoiseRange,
	kNoiseShape,	

	kNumParams,
};

enum EParamSettings
{
	// these are in dB
	kVolumeMin = -48,
	kVolumeMax = 12,
	kVolumeDefault = 0,
};

enum ENoiseType
{
	NT_White,
	NT_Pink,
	NT_Brown,

	NT_Count,
};