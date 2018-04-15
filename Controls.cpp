#include "Controls.h"
#include "Interface.h"
#include "WaveShaper.h"
#include "Params.h"
#include "MultiChannelBuffer.h"
#include "Interp.h"

#pragma  region KnobLineCoronaControl
KnobLineCoronaControl::KnobLineCoronaControl(IPlugBase* pPlug, IRECT pR, int paramIdx,
											 const IColor* pColor, const IColor* pCoronaColor,
											 float coronaThickness,
											 double innerRadius, double outerRadius,
											 double minAngle, double maxAngle,
											 EDirection direction, double gearing)
: IKnobLineControl(pPlug, pR, paramIdx, pColor, innerRadius, outerRadius, minAngle, maxAngle, direction, gearing)
, mCX(mRECT.MW())
, mCY(mRECT.MH())
, mCoronaColor(*pCoronaColor)
, mCoronaBlend(IChannelBlend::kBlendAdd, coronaThickness)
, mLabelControl(nullptr)
, mSharedLabel(false)
, mHasMouse(false)
{
}

bool KnobLineCoronaControl::Draw(IGraphics* pGraphics)
{
	float v = mMinAngle + (float)mValue * (mMaxAngle - mMinAngle);
	for (int i = 0; i <= mCoronaBlend.mWeight; ++i)
	{
		IColor color = mCoronaColor;
		pGraphics->DrawArc(&color, mCX, mCY, mOuterRadius - i, mMinAngle, v, nullptr, true);
		color.R /= 2;
		color.G /= 2;
		color.B /= 2;
		pGraphics->DrawArc(&color, mCX, mCY, mOuterRadius - i, v, mMaxAngle, nullptr, true);
	}
	float sinV = (float)sin(v);
	float cosV = (float)cos(v);
	float x1 = mCX + mInnerRadius * sinV, y1 = mCY - mInnerRadius * cosV;
	float x2 = mCX + mOuterRadius * sinV, y2 = mCY - mOuterRadius * cosV;
	return pGraphics->DrawLine(&mColor, x1, y1, x2, y2, &mBlend, true);
}

void KnobLineCoronaControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (pMod->R)
	{
		Interface::BeginMIDILearn(mPlug, mParamIdx, -1, x, y);
	}
	else
	{
		mHasMouse = true;
	}
}

void KnobLineCoronaControl::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
	double gearing = mGearing;
	
#ifdef PROTOOLS
#ifdef OS_WIN
	if (pMod->C) gearing *= 10.0;
#else
	if (pMod->R) gearing *= 10.0;
#endif
#else
	if (pMod->C || pMod->S) gearing *= 10.0;
#endif
	
	mValue += (double)dY / (double)(mRECT.T - mRECT.B) / gearing;
	mValue += (double)dX / (double)(mRECT.R - mRECT.L) / gearing;
	
	SetDirty();
}

void KnobLineCoronaControl::OnMouseUp(int x, int y, IMouseMod* pMod)
{
	if (!mRECT.Contains(x, y))
	{
		HideLabel();
	}

	mHasMouse = false;
}

void KnobLineCoronaControl::OnMouseOver(int x, int y, IMouseMod* pMod)
{
	ShowLabel();
}

void KnobLineCoronaControl::OnMouseOut()
{
	if (!mHasMouse)
	{
		HideLabel();
	}
}

void KnobLineCoronaControl::ShowLabel()
{
	if (mLabelControl != nullptr)
	{
		// if our label was hidden when we attached it, 
		// that means we should reposition it below the knob before displaying it.
		if (mSharedLabel)
		{
			IRECT targetRect = mRECT;
			targetRect.T = targetRect.B - 16;
			IRECT& labelRect = *mLabelControl->GetRECT();
			labelRect = targetRect;
			mLabelControl->SetTargetArea(targetRect);
		}
		SetValDisplayControl(mLabelControl);
		SetDirty();
	}
}

void KnobLineCoronaControl::HideLabel()
{
	SetValDisplayControl(nullptr);
	if (mLabelControl != nullptr)
	{
		mLabelControl->SetTextFromPlug(mLabelString.Get());
		SetDirty(false);
	}
}

void KnobLineCoronaControl::SetLabelControl(ITextControl* control, bool bShared)
{
	mLabelControl = control;
	mSharedLabel = bShared;
	if (mLabelControl != nullptr)
	{
		mLabelString.Set(mLabelControl->GetTextForPlug());
	}
	else
	{
		mLabelString.Set("");
		mSharedLabel = false;
	}

	// if our label control is shared, extend our rect to include where we will place the label when showing it.
	// this ensures that the area the label draws in will be cleared when the labels is moved elsewhere.
	if (mSharedLabel)
	{
		mRECT.L -= 15;
		mRECT.R += 15;
		mRECT.B += 15;
	}
}

#pragma  endregion KnobLineCoronaControl


#pragma  region EnumControl
EnumControl::EnumControl(IPlugBase* pPlug, IRECT rect, int paramIdx, IText* textStyle)
	: IControl(pPlug, rect, paramIdx)
{
	SetText(textStyle);
	mDblAsSingleClick = true;
	mDisablePrompt = false;
	// our contents need to fit into a 2px padded rect so we can draw the outline using mRECT
	// and have 1px background color between that and the contents
	IRECT contentsRect = mRECT.GetPadded(-5);
	mDecrementRect = IRECT(contentsRect.L, contentsRect.T, contentsRect.L + 5, contentsRect.B);
	mIncrementRect = IRECT(contentsRect.R - 5, contentsRect.T, contentsRect.R, contentsRect.B);

	mTextRect = contentsRect.GetHPadded(-7);

	// rect for displaying the popup should be full height, but only as wide as the text rect
	mPopupRect = mRECT.GetHPadded(-12);

	if (paramIdx < kNumParams)
	{
		mMin = GetParam()->GetMin();
		mMax = GetParam()->GetMax();
	}
	else
	{
		mMin = 0;
		mMax = mPlug->NPresets() - 1;
	}
}

bool EnumControl::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mText.mTextEntryBGColor, &mRECT);
	pGraphics->DrawRect(&mText.mTextEntryFGColor, &mRECT);

	// buttons
	IColor buttonColor = mText.mTextEntryFGColor;
	const int value = mParamIdx < kNumParams ? GetParam()->Int() : mPlug->GetCurrentPresetIdx();
	if (value == mMin || IsGrayed())
	{
		buttonColor.R *= 0.5f; buttonColor.G *= 0.5f; buttonColor.B *= 0.5f;
	}
	pGraphics->FillTriangle(&buttonColor, mDecrementRect.L, mDecrementRect.MH(), mDecrementRect.R, mDecrementRect.T, mDecrementRect.R, mDecrementRect.B, 0);

	buttonColor = mText.mTextEntryFGColor;
	if (value == mMax || IsGrayed())
	{
		buttonColor.R *= 0.5f; buttonColor.G *= 0.5f; buttonColor.B *= 0.5f;
	}
	pGraphics->FillTriangle(&buttonColor, mIncrementRect.L, mIncrementRect.T, mIncrementRect.R, mIncrementRect.MH(), mIncrementRect.L, mIncrementRect.B, 0);

	char* label = 0;
	if (mParamIdx < kNumParams)
	{
		static char display[16];
		GetParam()->GetDisplayForHost(display);
		label = display;
	}
	else
	{
		label = const_cast<char*>(mPlug->GetPresetName(mPlug->GetCurrentPresetIdx()));
	}

	IRECT textRect = mTextRect;
	// vertically center the text
	pGraphics->MeasureIText(&mText, label, &textRect);
#ifdef OS_OSX
	textRect.B -= 4;
#endif
	int offset = (mTextRect.H() - textRect.H()) / 2;
	textRect.T += offset;
	textRect.B += offset;
	mText.mColor.A = IsGrayed() ? 128 : 255;
	pGraphics->DrawIText(&mText, label, &textRect);

	return true;
}

void EnumControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (pMod->L)
	{
		if (mPopupRect.Contains(x, y))
		{
			if (mParamIdx < kNumParams)
			{
				PromptUserInput();
			}
			else // need to build the popup for presets by hand
			{
				IPopupMenu menu;
				const int currentPresetIdx = mPlug->GetCurrentPresetIdx();
				for (int i = 0; i < mPlug->NPresets(); ++i)
				{
					const char * presetName = mPlug->GetPresetName(i);
					if (i == currentPresetIdx)
					{
						menu.AddItem(new IPopupMenuItem(presetName, IPopupMenuItem::kChecked), -1);
					}
					else
					{
						menu.AddItem(new IPopupMenuItem(presetName), -1);
					}
				}

				if (GetGUI()->CreateIPopupMenu(&menu, &mRECT))
				{
					mPlug->RestorePreset(menu.GetChosenItemIdx());
					SetDirty(false);
				}
			}
		}
		else if (mIncrementRect.Contains(x, y))
		{
			StepValue(1);
		}
		else if (mDecrementRect.Contains(x, y))
		{
			StepValue(-1);
		}
	}
	else if (pMod->R && mParamIdx < kNumParams)
	{
		Interface::BeginMIDILearn(mPlug, mParamIdx, -1, x, y);
	}
}

void EnumControl::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
	StepValue(d);
}

void EnumControl::StepValue(int amount)
{
	const bool isParam = mParamIdx < kNumParams;
	if (isParam)
	{
		int count = GetParam()->GetNDisplayTexts();
		if (count > 1)
		{
			mValue += 1.0 / (double)(count - 1) * amount;
		}
		else
		{
			mValue += 1.0;
		}
	}
	else
	{
		const int presetIdx = BOUNDED(mPlug->GetCurrentPresetIdx() + amount, mMin, mMax);
		if (presetIdx != mPlug->GetCurrentPresetIdx())
		{
			mPlug->RestorePreset(presetIdx);
		}
	}

	SetDirty(isParam);
	Redraw();
}
#pragma  endregion EnumControl

#pragma region TextBox
TextBox::TextBox(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, IGraphics* pGraphics, const char * maxText, bool showParamUnits, float scrollSpeed)
	: ICaptionControl(pPlug, pR, paramIdx, pText, showParamUnits)
	, mTextRect(pR)
	, mScrollSpeed(scrollSpeed)
{
	mTextRect.GetPadded(-1);
	pGraphics->MeasureIText(pText, const_cast<char*>(maxText), &mTextRect);
#ifdef OS_OSX
	mTextRect.B -= 4;
#endif
	const int offset = (mRECT.H() - mTextRect.H()) / 2;
	mTextRect.T += offset;
	mTextRect.B += offset;

	SetTextEntryLength(strlen(maxText) - 1);
}

bool TextBox::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mText.mTextEntryBGColor, &mRECT);
	pGraphics->DrawRect(&mText.mTextEntryFGColor, &mRECT);

	IRECT ourRect = mRECT;
	mRECT = mTextRect;
	if (IsGrayed())
	{
		char display[32];
		GetParam()->GetDisplayForHost(mValue, true, display, false);
		mStr.Set(display);
		ITextControl::Draw(pGraphics);
	}
	else
	{
		ICaptionControl::Draw(pGraphics);
	}
	mRECT = ourRect;

	return true;
}

void TextBox::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (pMod->L)
	{
		IText ourText = mText;
		IRECT promptRect = mTextRect;
#if defined(OS_OSX)
		mText.mSize -= 2;
		promptRect.T -= 1;
#endif
		mPlug->GetGUI()->PromptUserInput(this, mPlug->GetParam(mParamIdx), &promptRect);
		mText = ourText;
		Redraw();
	}
	else if (pMod->R)
	{
		Interface::BeginMIDILearn(mPlug, mParamIdx, -1, x, y);
	}
}

void TextBox::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
#ifdef PROTOOLS
	if (pMod->C)
	{
		mValue += GetParam()->GetStep() * mScrollSpeed / 10 * d;
	}
#else
	if (pMod->C || pMod->S)
	{
		mValue += GetParam()->GetStep() * mScrollSpeed / 10 * d;
	}
#endif
	else
	{
		mValue += GetParam()->GetStep() * mScrollSpeed * d;
	}

	SetDirty();
}

void TextBox::GrayOut(bool gray)
{
	ICaptionControl::GrayOut(gray);

	mText.mColor.A = gray ? 128 : 255;
}
#pragma endregion TextBox

#pragma  region BangControl

BangControl::BangControl(IPlugBase* pPlug, IRECT iRect, Action action, IColor onColor, IColor offColor, IText* textStyle /*= nullptr*/, const char * label /*= nullptr*/, int paramIdx /*= -1*/, const char * fileTypes /*= "fxp"*/)
	: IControl(pPlug, iRect, paramIdx)
	, mAction(action)
	, mOnColor(onColor)
	, mOffColor(offColor)
	, mLabel(label)
	, mFileTypes(fileTypes)
{
	if (textStyle != nullptr)
	{
		SetText(textStyle);
	}
	mDblAsSingleClick = true;
}

bool BangControl::Draw(IGraphics* pGraphics)
{
	if (mValue > 0.5)
	{
		pGraphics->FillIRect(&mOnColor, &mRECT);
	}
	else
	{
		pGraphics->FillIRect(&mOffColor, &mRECT);
	}

	pGraphics->DrawRect(&mOnColor, &mRECT);

	if (mLabel != nullptr)
	{
		char * label = const_cast<char*>(mLabel);
		IRECT textRect = mRECT;
		// vertically center the text
		pGraphics->MeasureIText(&mText, label, &textRect);
#ifdef OS_OSX
		textRect.B -= 4;
#endif
		int offset = (mRECT.H() - textRect.H()) / 2;
		textRect.T += offset;
		textRect.B += offset;
		pGraphics->MeasureIText(&mText, label, &textRect);
		pGraphics->DrawIText(&mText, label, &textRect);
	}

	return true;
}

void BangControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (mAction == ActionBangParam)
	{
		mValue = 1;
		SetDirty();
	}
	else // not an actual parameter, direct-cast this bang on click
	{
		switch (mAction)
		{
		case ActionSave:
		{
			WDL_String fileName("");
			WDL_String directory("");
			GetGUI()->PromptForFile(&fileName, kFileSave, &directory, const_cast<char*>(mFileTypes));
			if (fileName.GetLength() > 0)
			{
				PLUG_CLASS_NAME * plug = static_cast<PLUG_CLASS_NAME*>(mPlug);
				if (plug != nullptr)
				{
					plug->HandleSave(&fileName, &directory);
				}
			}
		}
		break;

		case ActionLoad:
		{
			WDL_String fileName("");
			WDL_String directory("");
			GetGUI()->PromptForFile(&fileName, kFileOpen, &directory, const_cast<char*>(mFileTypes));
			if (fileName.GetLength() > 0)
			{
				PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(mPlug);
				if (plug != nullptr)
				{
					plug->HandleLoad(&fileName, &directory);
				}
			}
		}
		break;

		case ActionDumpPreset:
		{
			PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(mPlug);
			if (plug != nullptr)
			{
				plug->DumpPresetSrc();
			}
		}
		break;

		}
	}
}

void BangControl::OnMouseUp(int x, int y, IMouseMod* pMod)
{
	if (mParamIdx < kNumParams)
	{
		mValue = 0;
		SetDirty();
	}
}

#pragma  endregion 

#pragma  region PeaksControl
PeaksControl::PeaksControl(IPlugBase* pPlug, IRECT rect, IColor backColor, IColor peaksColor)
	: IPanelControl(pPlug, rect, &backColor)
	, mPeaksColor(peaksColor)
	, mPeaksSize(rect.W())
{
	mPeaks = new float[mPeaksSize];
	memset(mPeaks, 0, sizeof(float)*mPeaksSize);
}

PeaksControl::~PeaksControl()
{
	delete[] mPeaks;
}

bool PeaksControl::Draw(IGraphics* pGraphics)
{
	IPanelControl::Draw(pGraphics);

	for (int i = 0; i < mPeaksSize; ++i)
	{
		int x = mRECT.L + i;
		int ph = mRECT.H()*mPeaks[i];
		int y1 = mRECT.MH() + ph / 2;
		int y2 = mRECT.MH() - ph / 2;
		pGraphics->DrawLine(&mPeaksColor, x, y1, x, y2);
	}

	return true;
}

void PeaksControl::UpdatePeaks(const Minim::MultiChannelBuffer& withSamples)
{
	// calculate peaks
	const int chunkSize = withSamples.getBufferSize() / mPeaksSize;
	const bool bStereo = withSamples.getChannelCount() == 2;
	for (int i = 0; i < mPeaksSize; ++i)
	{
		float peak = 0;
		int s = 0;
		for (; s < chunkSize; ++s)
		{
			int frame = i*chunkSize + s;
			if (frame >= withSamples.getBufferSize())
			{
				break;
			}
			float val = 0;
			if (bStereo)
			{
				val = (withSamples.getChannel(0)[frame] + withSamples.getChannel(1)[frame]) / 2.f;
			}
			else
			{
				val = withSamples.getChannel(0)[frame];
			}
			peak += val*val;
		}
		peak /= s + 1;
		mPeaks[i] = sqrtf(peak);
	}

	SetDirty(false);
	Redraw();
}
#pragma  endregion PeaksControl

#pragma  region ShaperVizControl
ShaperVizControl::ShaperVizControl(IPlugBase* pPlug, IRECT rect, IColor bracketColor, IColor lineColor)
	: IControl(pPlug, rect)
	, mBracketColor(bracketColor)
	, mLineColor(lineColor)
{

}

bool ShaperVizControl::Draw(IGraphics* pGraphics)
{
	WaveShaper* shaper = dynamic_cast<WaveShaper*>(mPlug);
	if (shaper != nullptr)
	{
		const float center = Map(shaper->GetNoiseOffset(), -1, 1, mRECT.L, mRECT.R);
		const float widthPct = shaper->GetShape();
		const float waveWidth = mRECT.W();
		const float shaperSize = shaper->GetShaperSize();
		const float sampleWidth = (shaperSize * widthPct);
		const float chunkSize = shaperSize / mRECT.W();
		const float halfWidth = sampleWidth / chunkSize * 0.5f;

		float x = center - halfWidth;
		int y1 = mRECT.T, y2 = mRECT.B - 1;
		if (x < mRECT.L)
		{
			x += waveWidth;
		}
		pGraphics->DrawLine(&mBracketColor, x, y1, x, y2);
		pGraphics->DrawLine(&mBracketColor, x, y1, x + 4, y1);
		pGraphics->DrawLine(&mBracketColor, x, y2, x + 4, y2);

		x = center + halfWidth;
		if (x > mRECT.R)
		{
			x -= waveWidth;
		}
		pGraphics->DrawLine(&mBracketColor, x, y1, x, y2);
		pGraphics->DrawLine(&mBracketColor, x, y1, x - 4, y1);
		pGraphics->DrawLine(&mBracketColor, x, y2, x - 4, y2);

		// this will be [0, 1]
		const float mapLookup = shaper->GetShaperMapValue();
		x = Lerp(mRECT.L, mRECT.R, mapLookup);
		pGraphics->DrawLine(&mLineColor, x, y1, x, y2);
		
		Redraw();
		return true;
	}

	return false;
}


void ShaperVizControl::OnMouseUp(int x, int y, IMouseMod* pMod)
{
	TRACE;
}

void ShaperVizControl::OnMouseOver(int x, int y, IMouseMod* pMod)
{
	TRACE;
}

#pragma  endregion ShaperVizControl

#pragma  region XYControl
XYControl::XYControl(IPlugBase* pPlug, IRECT rect, const int paramX, const int paramY, const int pointRadius, IColor pointColor)
	: IControl(pPlug, rect)
	, mPointRadius(pointRadius)
	, mPointColor(pointColor)
	, mGribbed(false)
	, mPointX(0)
	, mPointY(0)
{
	AddAuxParam(paramX);
	AddAuxParam(paramY);
}

bool XYControl::Draw(IGraphics* pGraphics)
{	
	return pGraphics->FillCircle(&mPointColor, mPointX, mPointY, mPointRadius, 0, true);
}

void XYControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	mGribbed = mRECT.Contains(x, y);
}

void XYControl::OnMouseUp(int x, int y, IMouseMod* pMod)
{
	mGribbed = false;
}

void XYControl::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
	if (mGribbed)
	{
		mPointX = BOUNDED(mPointX + dX, mRECT.L, mRECT.R);
		mPointY = BOUNDED(mPointY + dY, mRECT.T, mRECT.B);

		mTargetRECT = IRECT(mPointX - mPointRadius, mPointY - mPointRadius, mPointX + mPointRadius, mPointY + mPointRadius);

		GetAuxParam(0)->mValue = Map(mPointX, mRECT.L, mRECT.R, 0, 1);
		GetAuxParam(1)->mValue = Map(mPointY, mRECT.B, mRECT.T, 0, 1);
		SetAllAuxParamsFromGUI();
		SetDirty(false);
	}
}

void XYControl::SetAuxParamValueFromPlug(int auxParamIdx, double value)
{
	IControl::SetAuxParamValueFromPlug(auxParamIdx, value);

	if (auxParamIdx == 0)
	{
		mPointX = Map(value, 0, 1, mRECT.L, mRECT.R);
	}
	else if (auxParamIdx == 1)
	{
		mPointY = Map(value, 0, 1, mRECT.B, mRECT.T);
	}

	mTargetRECT = IRECT(mPointX - mPointRadius, mPointY - mPointRadius, mPointX + mPointRadius, mPointY + mPointRadius);

	SetDirty(false);
}

#pragma  endregion XYControl

#pragma  region SnapshotControl

SnapshotControl::SnapshotControl(IPlugBase* pPlug, IRECT rect, const int snapshotParam, const int snapshotIdx, const int pointRadius, IColor backgroundColor, IColor pointColorA, IColor pointColorB)
	: IControl(pPlug, rect, snapshotParam)
	, mSnapshotIdx(snapshotIdx)
	, mPointRadius(pointRadius)
	, mBackgroundColor(backgroundColor)
	, mPointColorA(pointColorA)
	, mPointColorB(pointColorB)
	, mHighlight(0)
{

}

bool SnapshotControl::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mBackgroundColor, &mRECT);

	WaveShaper* shaper = dynamic_cast<WaveShaper*>(mPlug);
	if (shaper != nullptr)
	{
		WaveShaper::NoiseSnapshot snapshot = shaper->GetNoiseSnapshotNormalized(mSnapshotIdx);
		
		float x = ::Lerp(mRECT.L, mRECT.R, snapshot.AmpMod);
		float y = ::Lerp(mRECT.B, mRECT.T, snapshot.Rate);
		pGraphics->FillCircle(&mPointColorA, x, y, mPointRadius, 0, true);
		
		x = ::Lerp(mRECT.L, mRECT.R, snapshot.Range);
		y = ::Lerp(mRECT.B, mRECT.T, snapshot.Shape);
		pGraphics->FillCircle(&mPointColorB, x, y, mPointRadius, 0, true);
	}

	double weight = abs(GetParam()->GetNonNormalized(mValue) - mSnapshotIdx);
	if (weight < 1)
	{
		IChannelBlend blend(IChannelBlend::kBlendNone, 1 - weight);
		IColor border(255, 200, 200, 200);
		pGraphics->DrawLine(&border, mRECT.L, mRECT.T, mRECT.R, mRECT.T, &blend);
		pGraphics->DrawLine(&border, mRECT.R, mRECT.T+1, mRECT.R, mRECT.B-1, &blend);
		pGraphics->DrawLine(&border, mRECT.L, mRECT.B, mRECT.R, mRECT.B, &blend);
		pGraphics->DrawLine(&border, mRECT.L, mRECT.T+1, mRECT.L, mRECT.B-1, &blend);
	}

	if (mHighlight > 0)
	{
		IChannelBlend blend(IChannelBlend::kBlendNone, mHighlight);
		pGraphics->FillIRect(&COLOR_WHITE, &mRECT, &blend);
		mHighlight -= 0.1f;
		Redraw();
	}

	return true;
}

void SnapshotControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (pMod->R)
	{
		WaveShaper* shaper = dynamic_cast<WaveShaper*>(mPlug);
		if (shaper != nullptr)
		{
			shaper->UpdateNoiseSnapshot(mSnapshotIdx);
		}
	}

	mValue = GetParam()->GetNormalized(mSnapshotIdx);
	mHighlight = 1;
	SetDirty();
	GetGUI()->SetParameterFromGUI(mParamIdx, mValue);
}

#pragma  endregion

#pragma  region SnapshotSlider

SnapshotSlider::SnapshotSlider(IPlugBase* pPlug, int x, int y, int len, int handleRadius, int paramIdx, IColor lineColor, IColor handleColor)
	: IFaderControl(pPlug, x, y, len, paramIdx, &IBitmap(0, handleRadius*2, handleRadius*2))
	, mLineColor(lineColor)
	, mHandleColor(handleColor)
{
	mBlend.mMethod = IChannelBlend::kBlendNone;
	mBlend.mWeight = 0.75f;
}

bool SnapshotSlider::Draw(IGraphics* pGraphics)
{
	IRECT handle = GetHandleRECT();
	int handleRadius = handle.W() / 2;
	int handleCX = handle.MW();
	int handleCY = handle.MH();
	pGraphics->DrawLine(&mLineColor, handleCX - 2, mRECT.T + handleRadius, handleCX + 2, mRECT.T + handleRadius, &mBlend);
	pGraphics->DrawLine(&mLineColor, handleCX, mRECT.T + handleRadius, handleCX, mRECT.B - handleRadius, &mBlend);
	pGraphics->DrawLine(&mLineColor, handleCX - 2, mRECT.B - handleRadius, handleCX + 2, mRECT.B - handleRadius, &mBlend);
	
	pGraphics->FillCircle(&mHandleColor, handleCX, handleCY, handleRadius-4, &mBlend, true);
	//pGraphics->FillTriangle(&mHandleColor, handle.L + 4, handleCY, handle.R - 6, handle.T, handle.R - 6, handle.B, &mBlend);

	return true;
}

void SnapshotSlider::SetDirty(bool pushParamToPlug /*= true*/)
{
	IControl::SetDirty(pushParamToPlug);

	if (pushParamToPlug)
	{
		GetGUI()->SetParameterFromGUI(mParamIdx, mValue);
	}
}

#pragma  endregion


