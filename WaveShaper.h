#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Params.h"
#include "Interface.h"
#include "IMidiQueue.h"

#include "FileLoader.h"
#include "MultiChannelBuffer.h"
#include "Line.h"
#include "Noise.h"

namespace Minim
{
	class TickRate;
	class Multiplier;
	class Oscil;
	class Pan;
	class Summer;
	class WaveShaper;
	class Constant;
}

/*

 WaveShaper - an IPlug project template that includes controls and features typically used in Damien Quartz's IPlugs.

*/

class WaveShaper : public IPlug
{
public:

	WaveShaper(IPlugInstanceInfo instanceInfo);
	~WaveShaper();

	void Reset() override;
	void OnParamChange(int paramIdx) override;

	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

	bool SerializeState(ByteChunk* pChunk) override;
	int UnserializeState(ByteChunk* pChunk, int startPos) override;
	bool CompareState(const unsigned char* incomingState, int startPos) override;

	void PresetsChangedByHost() override;
	bool HostRequestingAboutBox() override;

	void ProcessMidiMsg(IMidiMsg* pMsg) override;

	// our additions
	void BeginMIDILearn(int param1, int param2, int x, int y);

	// getters used by UI classes to draw things
	float GetNoiseOffset() const;
	float GetNoiseRate() const;
	float GetShape() const;
	float GetShaperSize() const;
	float GetShaperMapValue() const;

	struct NoiseSnapshot
	{
		double AmpMod;
		double Rate;
		double Range;
		double Shape;
	};

	void UpdateNoiseSnapshot(int idx);

	const NoiseSnapshot& GetNoiseSnapshot(int idx) const 
	{
		return mNoiseSnapshots[idx];
	}

	// converted the requested snapshot to normalized values before returning
	NoiseSnapshot GetNoiseSnapshotNormalized(int idx);

	// called from the UI for the Load and Save buttons.
	// we need to wrap LoadProgramFromFXP and SaveProgramAsFXP
	// to prevent our presets from getting overwritten.
	void HandleSave(WDL_String* fileName, WDL_String* directory);
	void HandleLoad(WDL_String* fileName, WDL_String* directory);
	void DumpPresetSrc();

private:

	void SetParamBlend(int paramIdx, double begin, double end, double blend);
	void HandleMidiControlChange(IMidiMsg* pMsg);
	void SetControlChangeForParam(const IMidiMsg::EControlChangeMsg cc, const int paramIdx);
	// helper to send Midi Messages when a midi-mapped parameter changes
	void BroadcastParamChange(const int paramIdx);

	Interface mInterface;

	// params
	double mVolume;

	NoiseSnapshot mNoiseSnapshots[kNoiseSnapshotCount];

	// midi
	IMidiQueue mMidiQueue;
	IMidiMsg::EControlChangeMsg mControlChangeForParam[kNumParams];
	int mMidiLearnParamIdx;

	// audio
	FileLoader mFileLoader;
	Minim::MultiChannelBuffer mBuffer; // contains the currently loaded audio file
	Minim::Noise::Tint   mNoiseTint;
	
	Minim::Noise	   * mNoize;
	Minim::TickRate	   * mNoizeRate;
	Minim::Multiplier  * mNoizeAmp;
	Minim::Oscil	   * mNoizeMod;
	Minim::Summer	   * mNoizeSum;
	Minim::Constant	   * mNoizeOffset;
	Minim::WaveShaper  * mNoizeShaperLeft;
	Minim::WaveShaper  * mNoizeShaperRight;
	Minim::Pan		   * mPanLeft;
	Minim::Pan		   * mPanRight;
	Minim::Summer	   * mMainSignal;
	Minim::Multiplier  * mMainSignalVol;

	// controls
	Minim::Line			 mRateCtrl;
	Minim::Line		     mModCtrl;
	Minim::Line		     mRangeCtrl;
	Minim::Line          mShapeCtrl;
};
