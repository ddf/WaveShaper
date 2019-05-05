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

enum ECtrlTags
{
  kCtrlTagMeter = 0,
  kMidiMapper,
  kNumCtrlTags
};

// used by the UI to send messages to the main plugin class.
enum EMessages
{
  kSetMidiMapping,
};

// data payload for the SetMidiMapping message
struct MidiMapping
{
  enum CC
  {
    kNone = 128
  };

  const int param;
  const CC midiCC;

  MidiMapping(int p, CC cc = kNone) : param(p), midiCC(cc) {}
};
