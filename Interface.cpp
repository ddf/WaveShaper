#include "Interface.h"

#define str(s) #s
#define HEADER(CLASS) str(CLASS.h)

#include HEADER(PLUG_CLASS_NAME)
#include "Params.h"
#include "Controls.h"

//#define ENABLE_DUMP

enum ELayout
{
	kEnumHeight = 20,
	kButtonHeight = 15,
	kLargeKnobSize = 30,
	kSmallKnobSize = 20,
	kControlPointSize = 30,
	kSnapshotSliderHandle = 15,

	kPlugTitle_W = 200,
	kPlugTitle_H = 15,
	kPlugTitle_X = GUI_WIDTH - 10 - kPlugTitle_W,
	kPlugTitle_Y = GUI_HEIGHT - kPlugTitle_H - 10,

	kPresetRestoreControl_X = 10,
	kPresetRestoreControl_Y = 10,
	kPresetRestoreControl_W = 205,
	kPresetRestoreControl_H = kButtonHeight,

	kVolumeControl_W = kLargeKnobSize,
	kVolumeControl_H = kLargeKnobSize,
	kVolumeControl_X = GUI_WIDTH - kVolumeControl_W - 10,
	kVolumeControl_Y = 15,

	kPeaksControl_W = 500,
	kPeaksControl_H = 50,
	kPeaksControl_X = 25, // GUI_WIDTH / 2 - kPeaksControl_W / 2,
	kPeaksControl_Y = 15,

	kControlSurface_W = 500,
	kControlSurface_H = 500,
	kControlSurface_X = kPeaksControl_X, // GUI_WIDTH / 2 - kControlSurface_W / 2,
	kControlSurface_Y = kPeaksControl_Y + kPeaksControl_H,

	kControlSnapshot_S = 5,
	kControlSnapshot_X = kControlSurface_X + kControlSurface_W + 10,
	kControlSnapshot_Y = kControlSurface_Y,
	kControlSnapshot_H = (kControlSurface_H - kControlSnapshot_S * kNoiseSnapshotMax) / kNoiseSnapshotCount,
	kControlSnapshot_W = kControlSnapshot_H,
	kControlSnapshot_R = 3,


	kNoiseTypeControl_W = 100,
	kNoiseTypeControl_H = kEnumHeight,
	kNoiseTypeControl_X = kControlSurface_X,
	kNoiseTypeControl_Y = kControlSurface_Y + kControlSurface_H + 10,
};

namespace Color
{
	const IColor Background(255, 10, 10, 10);

	const IColor KnobLine(255, 255, 255, 255);
	const IColor KnobCorona(255, 255, 255, 255);
	
	const IColor Label(255, 208, 208, 216);

	const IColor EnumBackground(255, 125, 125, 125);
	const IColor EnumBorder = KnobLine;

	const IColor Title(255, 30, 30, 30);

	const IColor PeaksForeground(255, 100, 100, 100);
	const IColor PeaksBackground(255, 20, 20, 20);

	const IColor ShaperBracket(255, 192, 0, 0);
	const IColor ShaperLine(255, 0, 255, 0);

	const IColor ControlSurfaceBackground(255, 60, 60, 60);
	const IColor ControlPointA(255, 123, 123, 0);
	const IColor ControlPointB(255, 0, 123, 123);

	const IColor SnapshotSliderLine(255, 200, 200, 200);
	const IColor SnapshotSliderHandle(128, 255, 255, 255);
}

namespace TextStyles
{
#ifdef OS_WIN
	const int ControlTextSize = 12;
	const int LabelTextSize = 12;
	const int ButtonTextSize = 12;
	char * ControlFont = 0;
	char * LabelFont = 0;
#else
	const int ControlTextSize = 14;
	const int LabelTextSize = 12;
	const int ButtonTextSize = 12;
	char * ControlFont = 0;
	char * LabelFont = "Helvetica Neue";
#endif
	// can't be const because of stupid ITextControl constructor
	IText  Title(LabelTextSize+8, &Color::Title, LabelFont, IText::kStyleBold, IText::kAlignFar);
	IText  Label(LabelTextSize, &Color::Label, LabelFont, IText::kStyleBold, IText::kAlignCenter);
	IText  Enum(ControlTextSize, &Color::Label, ControlFont, IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault, &Color::EnumBackground, &Color::EnumBorder);
	IText  TextBox(ControlTextSize, &Color::Label, ControlFont, IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault, &Color::EnumBackground, &Color::EnumBorder);
	IText  StepMode(ControlTextSize-2, &Color::Label, ControlFont, IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault, &Color::EnumBackground, &Color::EnumBorder);
	IText  ButtonLabel(ButtonTextSize, &Color::Label, ControlFont, IText::kStyleNormal, IText::kAlignCenter);
}

namespace Strings
{
	const char * Title = PLUG_NAME " " VST3_VER_STR;
	const char * PresetsLabel = "Presets";
	const char * VolumeLabel = "Volume";
}

Interface::Interface(PLUG_CLASS_NAME* inPlug)
	: mPlug(inPlug)
	, mPresetControl(nullptr)
	, mPeaksControl(nullptr)
{
}

Interface::~Interface()
{
	mPlug = nullptr;
	mPresetControl = nullptr;
	mPeaksControl = nullptr;
}

void Interface::CreateControls(IGraphics* pGraphics)
{
	pGraphics->HandleMouseOver(true);

	pGraphics->AttachPanelBackground(&Color::Background);

	pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kPlugTitle), &TextStyles::Title, Strings::Title));

	AttachKnob(pGraphics, MakeIRect(kVolumeControl), kVolume, Strings::VolumeLabel);

	pGraphics->AttachControl(new EnumControl(mPlug, MakeIRect(kNoiseTypeControl), kNoiseType, &TextStyles::Enum));

	mPeaksControl = new PeaksControl(mPlug, MakeIRect(kPeaksControl), Color::PeaksBackground, Color::PeaksForeground);
	pGraphics->AttachControl(mPeaksControl);
	pGraphics->AttachControl(new ShaperVizControl(mPlug, MakeIRect(kPeaksControl), Color::ShaperBracket, Color::ShaperLine));

	IRECT controlRect = MakeIRect(kControlSurface);
	pGraphics->AttachControl(new IPanelControl(mPlug, controlRect, &Color::ControlSurfaceBackground));
	pGraphics->AttachControl(new XYControl(mPlug, controlRect, kNoiseAmpMod, kNoiseRate, kControlPointSize, Color::ControlPointA));
	pGraphics->AttachControl(new XYControl(mPlug, controlRect, kNoiseRange, kNoiseShape, kControlPointSize, Color::ControlPointB));

	for(int i = 0; i < kNoiseSnapshotCount; ++i)
	{
		int voff = (kControlSnapshot_H + kControlSnapshot_S) * i;
		int snapshotIdx = kNoiseSnapshotMax - i;
		pGraphics->AttachControl(new SnapshotControl(mPlug, MakeIRectVOffset(kControlSnapshot, voff), kNoiseSnapshot, snapshotIdx, kControlSnapshot_R, Color::ControlSurfaceBackground, Color::ControlPointA, Color::ControlPointB));
	}

	{
		const int x = kControlSnapshot_X + kControlSnapshot_W;
		const int y = kControlSnapshot_Y + kControlSnapshot_H / 2 - kSnapshotSliderHandle;
		const int len = (kControlSnapshot_H + kControlSnapshot_S)*kNoiseSnapshotMax + kSnapshotSliderHandle*2;
		pGraphics->AttachControl(new SnapshotSlider(mPlug, x, y, len, kSnapshotSliderHandle, kNoiseSnapshot, Color::SnapshotSliderLine, Color::SnapshotSliderHandle));
	}

	// Presets section
	if ( mPlug->NPresets() > 1 )
	{
		mPresetControl = AttachEnum(pGraphics, MakeIRect(kPresetRestoreControl), kNumParams);
	}
}

IControl* Interface::AttachEnum(IGraphics* pGraphics, IRECT rect, const int paramIdx, const char * label /*= nullptr*/)
{
	IControl* control = new EnumControl(mPlug, rect, paramIdx, &TextStyles::Enum);
	pGraphics->AttachControl(control);

	if (label != nullptr)
	{
		rect.B = rect.T;
		rect.T -= 20;
		pGraphics->AttachControl(new ITextControl(mPlug, rect, &TextStyles::Label, const_cast<char*>(label)));
	}

	return control;
}

IControl* Interface::AttachTextBox(IGraphics* pGraphics, IRECT rect, const int paramIdx, const float scrollSpeed, const char * maxValue, const char * label /*= nullptr*/)
{
	IControl* control = new TextBox(mPlug, rect, paramIdx, &TextStyles::TextBox, pGraphics, maxValue, false, scrollSpeed);
	pGraphics->AttachControl(control);

	if (label != nullptr)
	{
		rect.B = rect.T;
		rect.T -= 20;
		pGraphics->AttachControl(new ITextControl(mPlug, rect, &TextStyles::Label, const_cast<char*>(label)));
	}

	return control;
}

KnobLineCoronaControl* Interface::AttachKnob(IGraphics* pGraphics, IRECT rect, const int paramIdx, const char * label /*= nullptr*/)
{
	KnobLineCoronaControl* knob = new KnobLineCoronaControl(mPlug, rect, paramIdx, &Color::KnobLine, &Color::KnobCorona);
	pGraphics->AttachControl(knob);
	
	if (label != nullptr)
	{
		rect.T = rect.B;
		rect.B += 15;
		rect.L -= 15;
		rect.R += 15;
		ITextControl* labelControl = new ITextControl(mPlug, rect, &TextStyles::Label, const_cast<char*>(label));
		pGraphics->AttachControl(labelControl);
		knob->SetLabelControl(labelControl);
	}

	return knob;
}

void Interface::OnPresetChanged()
{
	if (mPresetControl != nullptr)
	{
		mPresetControl->SetDirty(false);
		mPresetControl->Redraw();
	}
}

void Interface::RebuildPeaks(const Minim::MultiChannelBuffer& forSamples)
{
	if (mPeaksControl != nullptr)
	{
		mPeaksControl->UpdatePeaks(forSamples);
	}
}

void Interface::BeginMIDILearn(IPlugBase* plug, const int paramIdx1, const int paramIdx2, const int x, const int y)
{
	PLUG_CLASS_NAME *quartzPlug = dynamic_cast<PLUG_CLASS_NAME *>(plug);
	if (NULL != quartzPlug)
	{
		quartzPlug->BeginMIDILearn(paramIdx1, paramIdx2, x, y);
	}
}
