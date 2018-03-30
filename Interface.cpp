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
{
}

Interface::~Interface()
{
	mPlug = nullptr;
	mPresetControl = nullptr;
}

void Interface::CreateControls(IGraphics* pGraphics)
{
	pGraphics->HandleMouseOver(true);

	pGraphics->AttachPanelBackground(&Color::Background);

	pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kPlugTitle), &TextStyles::Title, Strings::Title));

	AttachKnob(pGraphics, MakeIRect(kVolumeControl), kVolume, Strings::VolumeLabel);

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

void Interface::BeginMIDILearn(IPlugBase* plug, const int paramIdx1, const int paramIdx2, const int x, const int y)
{
	PLUG_CLASS_NAME *quartzPlug = dynamic_cast<PLUG_CLASS_NAME *>(plug);
	if (NULL != quartzPlug)
	{
		quartzPlug->BeginMIDILearn(paramIdx1, paramIdx2, x, y);
	}
}
