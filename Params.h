#pragma once

enum EParams
{
	kVolume,

	kNoiseAmpMod,
	kNoiseRate,

	kNumParams,
};

enum EParamSettings
{
	// these are in dB
	kVolumeMin = -48,
	kVolumeMax = 12,
	kVolumeDefault = 0,
};