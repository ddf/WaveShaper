#include "Controls.h"
#include "Interface.h"
#include "Params.h"

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