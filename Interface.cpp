#include "Interface.h"
#include "TextBox.h"
#include "KnobLineCoronaControl.h"

#define str(s) #s
#define HEADER(CLASS) str(CLASS.h)

#include HEADER(PLUG_CLASS_NAME)
#include "Params.h"
#include "Controls.h"

#include "IconsFontaudio.h"

//#define ENABLE_DUMP

enum ELayout
{
  kEnumHeight = 20,
  kButtonHeight = 15,
  kLargeKnobSize = 30,
  kSmallKnobSize = 20,
  kControlPointSize = 15,
  kSnapshotSliderHandle = 15,

  kPlugTitle_W = 200,
  kPlugTitle_H = 15,
  kPlugTitle_X = PLUG_WIDTH - 10 - kPlugTitle_W,
  kPlugTitle_Y = PLUG_HEIGHT - kPlugTitle_H - 10,

  kPresetRestoreControl_X = 10,
  kPresetRestoreControl_Y = 10,
  kPresetRestoreControl_W = 205,
  kPresetRestoreControl_H = kButtonHeight,

  kPeaksControl_W = 500,
  kPeaksControl_H = 50,
  kPeaksControl_X = 50,
  kPeaksControl_Y = 15,

  kControlSurface_W = kPeaksControl_W,
  kControlSurface_H = kControlSurface_W,
  kControlSurface_X = kPeaksControl_X,
  kControlSurface_Y = kPeaksControl_Y + kPeaksControl_H + 10,

  kControlSnapshot_S = 5,
  kControlSnapshot_X = kControlSurface_X + kControlSurface_W + 10,
  kControlSnapshot_Y = kControlSurface_Y,
  kControlSnapshot_H = (kControlSurface_H - kControlSnapshot_S * kNoiseSnapshotMax) / kNoiseSnapshotCount,
  kControlSnapshot_W = kControlSnapshot_H,
  kControlSnapshot_R = 3,

  kControlSnapshotBang_W = 15,
  kControlSnapshotBang_H = 15,
  kControlSnapshotBang_X = kControlSnapshot_X + kControlSnapshot_W - kControlSnapshotBang_W - 4,
  kControlSnapshotBang_Y = kControlSnapshot_Y + 4,

  kControlSnapshotSlider_X = kControlSnapshot_X + kControlSnapshot_W + 15,
  kControlSnapshotSlider_Y = kControlSnapshot_Y + kControlSnapshot_H / 2 - kSnapshotSliderHandle / 2,
  kControlSnapshotSlider_H = (kControlSnapshot_H + kControlSnapshot_S) * kNoiseSnapshotMax + kSnapshotSliderHandle,

  kLoadAudioControl_X = kPeaksControl_X + kPeaksControl_W - kControlPointSize,
  kLoadAudioControl_Y = kPeaksControl_Y,
  kLoadAudioControl_W = kControlPointSize,
  kLoadAudioControl_H = kPeaksControl_H,

  kVolumeControl_W = kLargeKnobSize,
  kVolumeControl_H = kLargeKnobSize,
  kVolumeControl_X = kLoadAudioControl_X + kLoadAudioControl_W + 25,
  kVolumeControl_Y = 20,

  kNoiseTypeControl_W = 30,
  kNoiseTypeControl_H = kLargeKnobSize*4,
  kNoiseTypeControl_X = kControlSurface_X - kNoiseTypeControl_W - 10,
  kNoiseTypeControl_Y = kControlSurface_Y + kControlSurface_H/2 - kNoiseTypeControl_H/2,

  kEnvelopeControl_W = kLargeKnobSize,
  kEnvelopeControl_S = kEnvelopeControl_W + 20,
  kEnvelopeControl_H = kLargeKnobSize,
  kEnvelopeControl_X = kControlSurface_X + kControlSurface_W / 2 - kEnvelopeControl_W - kEnvelopeControl_S,
  kEnvelopeControl_Y = kControlSurface_Y + kControlSurface_H + 10,  

  kPlayStopControl_W = 30,
  kPlayStopControl_H = 30,
  kPlayStopControl_X = kControlSurface_X + kControlSurface_W - kPlayStopControl_W,
  kPlayStopControl_Y = kControlSurface_Y + kControlSurface_H + 10,
};

namespace Color
{
  const IColor Background(255, 10, 10, 10);

  const IColor KnobLine(255, 255, 255, 255);
  const IColor KnobCorona(255, 255, 255, 255);

  const IColor Label(255, 208, 208, 216);

  const IColor EnumBackground(255, 90, 90, 90);
  const IColor EnumBorder = KnobLine;

  const IColor BangOn(255, 200, 200, 200);
  const IColor BangOff(EnumBackground);

  const IColor Title(255, 30, 30, 30);

  const IColor PeaksForeground(255, 100, 100, 100);
  const IColor PeaksBackground(255, 60, 60, 60);

  const IColor ControlSurfaceBackground(255, 60, 60, 60);
  const IColor ControlPointA(255, 170, 170, 0);
  const IColor ControlPointB(255, 0, 170, 170);

  const IColor ShaperBracket(255, 0, 200, 200);
  const IColor ShaperLine(255, 200, 200);

  const IColor SnapshotSliderLine(255, 200, 200, 200);
  const IColor SnapshotSliderHandle(128, 255, 255, 255);
  const IColor SnapshotSliderHandleHighlight(200, 255, 255, 255);

  const IVColorSpec SnapshotSliderColors({ DEFAULT_BGCOLOR, SnapshotSliderHandle, DEFAULT_PRCOLOR, DEFAULT_FRCOLOR, DEFAULT_HLCOLOR, SnapshotSliderLine, DEFAULT_X1COLOR, DEFAULT_X2COLOR, DEFAULT_X3COLOR });

  const IColor PlayStopBackground(EnumBackground);
  const IColor PlayStopForeground(EnumBorder);
}

namespace TextStyles
{
#ifdef OS_WIN
  const float ControlTextSize = 14;
  const float LabelTextSize = 14;
  const float ButtonTextSize = 14;
  const char* ControlFont = "ControlFont";
  const char* LabelFont = "LabelFont";
  const char* FontName = "Segoe UI";
#else
  const int ControlTextSize = 14;
  const int LabelTextSize = 12;
  const int ButtonTextSize = 12;
  const char* ControlFont = "ControlFont";
  const char* LabelFont = "LabelFont";
  const char* FontName = "Helvetica Neue";
#endif

  const char * AudioFont = "AudioFont";

  const IText Title(LabelTextSize + 8, Color::Title, LabelFont, EAlign::Far);
  const IText Label(LabelTextSize, Color::Label, LabelFont, EAlign::Center);
  const IText Enum(ControlTextSize, Color::Label, ControlFont, EAlign::Center, EVAlign::Middle, 0, Color::EnumBackground, Color::EnumBorder);
  const IText TextBox(ControlTextSize, Color::Label, ControlFont, EAlign::Center, EVAlign::Middle, 0, Color::EnumBackground, Color::EnumBorder);
  const IText StepMode(ControlTextSize - 2, Color::Label, ControlFont, EAlign::Center, EVAlign::Middle, 0, Color::EnumBackground, Color::EnumBorder);
  const IText ButtonLabel(ButtonTextSize, Color::Label, ControlFont, EAlign::Center);
  const IText Load(ControlTextSize * 2, Color::Label, ControlFont, EAlign::Far, EVAlign::Middle, -90, Color::EnumBackground, Color::EnumBorder);
  const IText Icon(ControlTextSize, Color::Label, AudioFont, EAlign::Center, EVAlign::Middle, 0, Color::EnumBackground, Color::EnumBorder);
}

namespace Strings
{
  const char* Title = PLUG_NAME " " PLUG_VERSION_STR;
  const char* PresetsLabel = "Presets";
  const char* VolumeLabel = "Volume";
  const char* EnvAttackLabel = "Attack";
  const char* EnvDecayLabel = "Decay";
  const char* EnvSustainLabel = "Sustain";
  const char* EnvReleaseLabel = "Release";
  const char* NoiseTypeLabel = "WaveShape";

  const char* LoadAudioLabel = ". . .";
  const char* AudioFileTypes = "wav au snd aif aiff flac ogg";

  const char* UpdateSnapshot = "+";
  const char* SnapshotSliderLabel = "";
}

Interface::Interface(PLUG_CLASS_NAME* inPlug)
	: mPlug(inPlug)
	, mPresetControl(nullptr)
	, mPeaksControl(nullptr)
{
	memset(mSnapshotControls, 0, sizeof(mSnapshotControls));
}

Interface::~Interface()
{
	mPlug = nullptr;
	mPresetControl = nullptr;
	mPeaksControl = nullptr;
}

void Interface::CreateControls(IGraphics* pGraphics)
{
  pGraphics->LoadFont(TextStyles::ControlFont, TextStyles::FontName, ETextStyle::Bold);
  pGraphics->LoadFont(TextStyles::LabelFont, TextStyles::FontName, ETextStyle::Normal);
  pGraphics->LoadFont(TextStyles::AudioFont, FONTAUDIO_FN);
  pGraphics->LoadFont(DEFAULT_FONT, ROBOTO_FN);
  pGraphics->HandleMouseOver(true);

  pGraphics->AttachPanelBackground(Color::Background);

  pGraphics->AttachControl(new ITextControl(MakeIRect(kPlugTitle), Strings::Title, TextStyles::Title));

  AttachKnob(pGraphics, MakeIRect(kVolumeControl), kVolume, Strings::VolumeLabel);

  IVStyle style = DEFAULT_STYLE.WithValueText(TextStyles::Icon).WithColor(kFG, COLOR_TRANSPARENT).WithColor(kPR, COLOR_DARK_GRAY);
  pGraphics->AttachControl(new ShapeControl(MakeIRect(kNoiseTypeControl),
                                            { ICON_FAU_MODRANDOM, ICON_FAU_MODRANDOM, ICON_FAU_MODRANDOM },
                                            { COLOR_WHITE, IColor::FromColorCode(0xff6ec7), COLOR_RED },
                                            Strings::NoiseTypeLabel, style, EVShape::Rectangle, EDirection::Vertical));

  // waveform view and viz overlay showing the selected section and playhead
  {
    IRECT rect = MakeIRect(kPeaksControl).GetHPadded(-2 - kControlPointSize);
    mPeaksControl = new PeaksControl(rect, Color::PeaksBackground, Color::PeaksForeground);
    pGraphics->AttachControl(mPeaksControl);
    pGraphics->AttachControl(new ShaperVizControl(rect, Color::ShaperBracket, Color::ShaperLine));
  }

  IRECT controlRect = MakeIRect(kControlSurface);
  pGraphics->AttachControl(new IPanelControl(controlRect, Color::ControlSurfaceBackground));
  pGraphics->AttachControl(new XYControl(controlRect.GetPadded(-2), kNoiseAmpMod, kNoiseRate, kControlPointSize, Color::ControlPointA, ControlPoint::Diamond));
  pGraphics->AttachControl(new XYControl(controlRect.GetPadded(-2), kNoiseRange, kNoiseShape, kControlPointSize*0.85f, Color::ControlPointB, ControlPoint::Square));

  pGraphics->AttachControl(new BangControl(MakeIRect(kLoadAudioControl), BangControl::ActionLoad, Color::BangOn, Color::BangOff, &TextStyles::Load, Strings::LoadAudioLabel, -1, Strings::AudioFileTypes));

  for (int i = 0; i < kNoiseSnapshotCount; ++i)
  {
    int voff = (kControlSnapshot_H + kControlSnapshot_S) * i;
    int snapshotIdx = kNoiseSnapshotMax - i;
    mSnapshotControls[snapshotIdx] = new SnapshotControl(MakeIRectVOffset(kControlSnapshot, voff), kNoiseSnapshot, snapshotIdx, kControlSnapshot_R, Color::ControlSurfaceBackground, Color::ControlPointA, ControlPoint::Diamond, Color::ControlPointB, ControlPoint::Square);
    pGraphics->AttachControl(mSnapshotControls[snapshotIdx]);

    BangControl::Action bangAction = (BangControl::Action)(BangControl::ActionCustom + snapshotIdx);
    pGraphics->AttachControl(new BangControl(MakeIRectVOffset(kControlSnapshotBang, voff), bangAction, Color::BangOn, Color::BangOff, &TextStyles::Enum, Strings::UpdateSnapshot));
  }

  // snapshot slider
  {    
    IVStyle style = DEFAULT_STYLE;
    style.colorSpec.mColors[kFG] = Color::SnapshotSliderHandle;
    style.colorSpec.mColors[kSH] = Color::SnapshotSliderLine;
    style.colorSpec.mColors[kHL] = Color::SnapshotSliderHandleHighlight;
    style.showLabel = false;
    style.showValue = false;
    style.drawShadows = false;
    style.drawFrame = false;
    pGraphics->AttachControl(new SnapshotSlider(kControlSnapshotSlider_X, kControlSnapshotSlider_Y, kControlSnapshotSlider_H, kSnapshotSliderHandle, kNoiseSnapshot, Strings::SnapshotSliderLabel, style));
  }

  // ADSR
  {
    AttachKnob(pGraphics, MakeIRect(kEnvelopeControl), kEnvAttack, Strings::EnvAttackLabel);
    AttachKnob(pGraphics, MakeIRectHOffset(kEnvelopeControl, kEnvelopeControl_S), kEnvDecay, Strings::EnvDecayLabel);
    AttachKnob(pGraphics, MakeIRectHOffset(kEnvelopeControl, kEnvelopeControl_S * 2), kEnvSustain, Strings::EnvSustainLabel);
    AttachKnob(pGraphics, MakeIRectHOffset(kEnvelopeControl, kEnvelopeControl_S * 3), kEnvRelease, Strings::EnvReleaseLabel);
  }

  pGraphics->AttachControl(new PlayStopControl(MakeIRect(kPlayStopControl), Color::PlayStopBackground, Color::PlayStopForeground));

  // Presets section
  if (mPlug->NPresets() > 1)
  {
    mPresetControl = AttachEnum(pGraphics, MakeIRect(kPresetRestoreControl), kNumParams);
  }
}

IControl* Interface::AttachEnum(IGraphics* pGraphics, IRECT rect, const int paramIdx, const char * label /*= nullptr*/)
{
	IControl* control = new EnumControl(rect, paramIdx, TextStyles::Enum);
	pGraphics->AttachControl(control);

	if (label != nullptr)
	{
		rect.B = rect.T;
		rect.T -= 20;
		pGraphics->AttachControl(new ITextControl(rect, label, TextStyles::Label));
	}

	return control;
}

IControl* Interface::AttachTextBox(IGraphics* pGraphics, IRECT rect, const int paramIdx, const float scrollSpeed, const char * maxValue, const char * label /*= nullptr*/)
{
	IControl* control = new TextBox(rect, paramIdx, TextStyles::TextBox, pGraphics, maxValue, false, scrollSpeed);
	pGraphics->AttachControl(control);

	if (label != nullptr)
	{
		rect.B = rect.T;
		rect.T -= 20;
		pGraphics->AttachControl(new ITextControl(rect, label, TextStyles::Label));
	}

	return control;
}

KnobLineCoronaControl* Interface::AttachKnob(IGraphics* pGraphics, IRECT rect, const int paramIdx, const char * label /*= nullptr*/)
{
	KnobLineCoronaControl* knob = new KnobLineCoronaControl(rect, paramIdx, Color::KnobLine, Color::KnobCorona);
	pGraphics->AttachControl(knob);
	
	if (label != nullptr)
	{
		rect.T = rect.B;
		rect.B += 15;
		rect.L -= 15;
		rect.R += 15;
		ITextControl* labelControl = new ITextControl(rect, label, TextStyles::Label);
		pGraphics->AttachControl(labelControl);
		knob->SetLabelControl(labelControl, label);
	}

	return knob;
}

void Interface::OnPresetChanged()
{
	if (mPresetControl != nullptr)
	{
		mPresetControl->SetDirty(false);
	}
}

void Interface::UpdateSnapshot(const int idx)
{
	if (mSnapshotControls != nullptr)
	{
		mSnapshotControls[idx]->Update();
	}
}

void Interface::RebuildPeaks(const Minim::MultiChannelBuffer& forSamples)
{
	if (mPeaksControl != nullptr)
	{
		mPeaksControl->UpdatePeaks(forSamples);
	}
}

void Interface::BeginMIDILearn(IEditorDelegate* plug, const int paramIdx1, const int paramIdx2, const int x, const int y)
{
	PLUG_CLASS_NAME *quartzPlug = dynamic_cast<PLUG_CLASS_NAME *>(plug);
	if (NULL != quartzPlug)
	{
		quartzPlug->BeginMIDILearn(paramIdx1, paramIdx2, x, y);
	}
}
