#include "WaveShaper.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "Controls.h"

// The number of presets/programs
const int kNumPrograms = 1;

// name of the section of the INI file we save midi cc mappings to
const char * kMidiControlIni = "midicc";
const IMidiMsg::EControlChangeMsg kUnmappedParam = (IMidiMsg::EControlChangeMsg)128;

#pragma region Param Settings
// move these to sliders at some point.
const double kDefaultMod = 0.5;
const double kMinMod = 0.;
const double kMaxMod = 120.;

const double kDefaultRate = 0.0005;
const double kMinRate = 0.00001;
const double kMaxRate = 0.001;

// range is the noise offset, which basically determines where in the file the center point of scrubbing is.
const double kDefaultRange = 0;
const double kMinRange = -1.; // 0.01;
const double kMaxRange = 1.0; // 0.5f;

// shape is the mapAmplitude, which basically determines how many samples from the source are scrubbed over
const double kDefaultShape = 0.1;
const double kMinShape = 0.05;
const double kMaxShape = 0.35;

const double kEnvAttackMin = 0.005;
const double kEnvAttackMax = 2;
const double kEnvDecayMin = 0.005;
const double kEnvDecayMax = 2;
const double kEnvSustainMin = 0;
const double kEnvSustainMax = 100;
const double kEnvReleaseMin = 0.005;
const double kEnvReleaseMax = 5;

const double kEnvSustainDefault = 75;
const double kEnvReleaseDefault = 0.25;

const double kSecondsStep = 0.005;
const char * kSecondsLabel = "s";

const double kPercentStep = 1;
const char * kPercentLabel = "%";
#pragma  endregion

WaveShaper::WaveShaper(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
#if IPLUG_EDITOR
mInterface(this)
#endif
{
  // Define parameter ranges, display units, labels.
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kVolume)->InitDouble("Volume", kVolumeDefault, kVolumeMin, kVolumeMax, 0.1, "dB");
  GetParam(kVolume)->SetDisplayText((double)kVolumeMin, "-inf");

  GetParam(kNoiseType)->InitEnum("Noise Type", NT_Pink, NT_Count);
  GetParam(kNoiseType)->SetDisplayText(0, "White");
  GetParam(kNoiseType)->SetDisplayText(1, "Pink");
  GetParam(kNoiseType)->SetDisplayText(2, "Red");

  GetParam(kNoiseAmpMod)->InitDouble("Noise Amp Mod", kDefaultMod, kMinMod, kMaxMod, 0.1, "Hz", "", 2.0);
  GetParam(kNoiseRate)->InitDouble("Noise Rate", kDefaultRate, kMinRate, kMaxRate, kMinRate);
  GetParam(kNoiseRange)->InitDouble("Noise Range", kDefaultRange, kMinRange, kMaxRange, 0.01);
  GetParam(kNoiseShape)->InitDouble("Noise Shape", kDefaultShape, kMinShape, kMaxShape, 0.01);

  // when this parameter changes, Noise Amp Mod, Noise Rate, Noise Range, and Noise Shape are all changed
  // by lerping between the values in two Noise Snapshots.
  GetParam(kNoiseSnapshot)->InitDouble("Noise Snapshot", kNoiseSnapshotDefault, kNoiseSnapshotMin, kNoiseSnapshotMax, 0.01, "", IParam::kFlagMeta);

  // ADSR
  {
    GetParam(kEnvAttack)->InitDouble("Attack", kEnvAttackMin, kEnvAttackMin, kEnvAttackMax, kSecondsStep, kSecondsLabel, IParam::kFlagsNone, "ADSR");
    GetParam(kEnvDecay)->InitDouble("Decay", kEnvDecayMin, kEnvDecayMin, kEnvDecayMax, kSecondsStep, kSecondsLabel, IParam::kFlagsNone, "ADSR");
    GetParam(kEnvSustain)->InitDouble("Sustain", kEnvSustainDefault, kEnvSustainMin, kEnvSustainMax, kPercentStep, kPercentLabel, IParam::kFlagsNone, "ADSR");
    GetParam(kEnvRelease)->InitDouble("Release", kEnvReleaseDefault, kEnvReleaseMin, kEnvReleaseMax, kSecondsStep, kSecondsLabel, IParam::kFlagsNone, "ADSR");
  }

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    mInterface.CreateControls(pGraphics);

//    pGraphics->AttachCornerResizer(kUIResizerScale, false);
//    pGraphics->AttachPanelBackground(COLOR_GRAY);
//    pGraphics->HandleMouseOver(true);
////    pGraphics->EnableLiveEdit(true);
//    pGraphics->LoadFont("Roboto-Regular", ROBOTTO_FN);
//    const IRECT b = pGraphics->GetBounds();
//    pGraphics->AttachControl(new IVKeyboardControl(IRECT(10, 335, PLUG_WIDTH-10, PLUG_HEIGHT-10)));
//    pGraphics->AttachControl(new IVMultiSliderControl<8>(b.GetGridCell(0, 2, 2).GetPadded(-30)));
//    const IRECT controls = b.GetGridCell(1, 2, 2);
//    pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 2, 6).GetCentredInside(90), kParamGain, "Gain", true));
//    pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(1, 2, 6).GetCentredInside(90), kParamNoteGlideTime, "Glide"));
//    const IRECT sliders = controls.GetGridCell(2, 2, 6).Union(controls.GetGridCell(3, 2, 6));
//    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(0, 1, 4).GetMidHPadded(10.), kParamAttack));
//    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(1, 1, 4).GetMidHPadded(10.), kParamDecay));
//    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(2, 1, 4).GetMidHPadded(10.), kParamSustain));
//    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(3, 1, 4).GetMidHPadded(10.), kParamRelease));
//    pGraphics->AttachControl(new IVMeterControl<1>(controls.GetFromRight(100).GetPadded(-30)), kCtrlTagMeter);
  };
#endif
}

#if IPLUG_DSP
void WaveShaper::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
  
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }

  mMeterBallistics.ProcessBlock(outputs, nFrames);
}

void WaveShaper::OnIdle()
{
  mMeterBallistics.TransmitData(*this);
}

void WaveShaper::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void WaveShaper::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void WaveShaper::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kParamNoteGlideTime:
      mDSP.mSynth.SetNoteGlideTime(GetParam(paramIdx)->Value());
      break;
      
    default:
      break;
  }
}
#endif

void WaveShaper::HandleSave(WDL_String* fileName, WDL_String* directory)
{
  if (strcmp(fileName->get_fileext(), "fxp") == 0)
  {
    const char * programName = fileName->get_filepart();
    // change the current preset without actually loading it.
    mCurrentPresetIdx = 0;
    // modify this preset so that the preset name saved to the file is correct.
    ModifyCurrentPreset(programName);
    SaveProgramAsFXP(fileName->Get());
    // notify the host because we changed the settings of the first preset
    InformHostOfProgramChange();

    mInterface.OnPresetChanged();
  }
}

void WaveShaper::HandleLoad(WDL_String* fileName, WDL_String* directory)
{
  if (strcmp(fileName->get_fileext(), "fxp") == 0)
  {
    // change preset index to first preset so that the program will load there
    // instead of overwriting a real preset.
    mCurrentPresetIdx = 0;
    LoadProgramFromFXP(fileName->Get());

    mInterface.OnPresetChanged();
  }
  else
  {
    // we can load without locking cause mBuffer is not used by the DSP chain,
    // so it's better to hang the UI thread than the audio thread.
    mFileLoader.Load(fileName->Get(), mBuffer);
    mInterface.RebuildPeaks(mBuffer);

    // #TODO update the DSP
    //mNoizeShaperLeft->getWavetable().setWaveform(mBuffer.getChannel(0), mBuffer.getBufferSize());
    //if (mBuffer.getChannelCount() == 1)
    //{
    //  mNoizeShaperRight->getWavetable().setWaveform(mBuffer.getChannel(0), mBuffer.getBufferSize());
    //}
    //else
    //{
    //  mNoizeShaperRight->getWavetable().setWaveform(mBuffer.getChannel(1), mBuffer.getBufferSize());
    //}
  }
}


void WaveShaper::HandleAction(BangControl::Action action)
{
  const int snapshotIdx = (int)action - (int)BangControl::ActionCustom;
  mInterface.UpdateSnapshot(snapshotIdx);
}

// modified version of DumpPresetSrcCode 
void WaveShaper::DumpPresetSrc()
{
  //IMutexLock lock(this);

  WDL_String path;
  DesktopPath(path);
  path.Append("\\dump.txt");
  FILE* fp = fopen(path.Get(), "w");
  // all the param indices we *should* include.
  // we collect these first because we need the count for the second argument
  // of MakePresetFromNamedParams
  std::vector<int> paramsForDump;
  for (int i = 0; i < kNumParams; ++i)
  {
    IParam* param = GetParam(i);
    if (param->Value() != param->GetDefault())
    {
      paramsForDump.push_back(i);
    }
  }
  const int paramCount = paramsForDump.size();
  WDL_String name;
  name.Set(GetPresetName(0));
  name.remove_fileext();
  fprintf(fp, "\tMakePresetFromNamedParams(\"%s\", %d", name.Get(), paramCount);
  for (int i = 0; i < paramCount; ++i)
  {
    const int paramIdx = paramsForDump[i];
    IParam* pParam = GetParam(paramIdx);
    char paramVal[32];
    switch (pParam->Type())
    {
      case IParam::kTypeBool:
        sprintf(paramVal, "%s", (pParam->Bool() ? "true" : "false"));
        break;
      case IParam::kTypeInt:
        sprintf(paramVal, "%d", pParam->Int());
        break;
      case IParam::kTypeEnum:
        sprintf(paramVal, "%d", pParam->Int());
        break;
      case IParam::kTypeDouble:
      default:
        sprintf(paramVal, "%.6f", pParam->Value());
        break;
    }
    fprintf(fp, "\n\t\t, %d, %s // %s", paramIdx, paramVal, pParam->GetNameForHost());
  }
  fprintf(fp, "\n\t);\n");
  fclose(fp);
}