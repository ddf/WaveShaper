#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Params.h"
#include "WaveShaper_DSP.h"
#include "IVMeterControl.h"

#include "Interface.h"
#include "Controls.h"
#include "FileLoader.h"
#include "MultiChannelBuffer.h"

class WaveShaper : public IPlug
{
public:
  WaveShaper(IPlugInstanceInfo instanceInfo);

  // called from the UI for the Load and Save buttons.
  // we need to wrap LoadProgramFromFXP and SaveProgramAsFXP
  // to prevent our presets from getting overwritten.
  void HandleSave(WDL_String* fileName, WDL_String* directory);
  void HandleLoad(WDL_String* fileName, WDL_String* directory);
  void HandleAction(BangControl::Action action);
  void DumpPresetSrc();

  // getters used by UI classes to draw things
  float GetNoiseOffset() const;
  float GetNoiseRate() const;
  float GetShape() const;
  float GetShaperSize() const;
  float GetShaperMapValue() const;

  // #TODO switch everything over to MidiMapper
  void BeginMIDILearn(int param1, int param2, int x, int y) {}

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

private:
  FileLoader mFileLoader;
  Minim::MultiChannelBuffer mBuffer;

  NoiseSnapshot mNoiseSnapshots[kNoiseSnapshotCount];

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
public:
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
private:
  WaveShaperDSP mDSP {16};
  IVMeterControl<1>::IVMeterBallistics mMeterBallistics {kCtrlTagMeter};
#endif

#if IPLUG_EDITOR
private:
  Interface mInterface;
#endif
};
