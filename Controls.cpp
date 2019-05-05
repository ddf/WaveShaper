#include "Controls.h"
#include "Interface.h"
#include "WaveShaper.h"
#include "Params.h"
#include "MultiChannelBuffer.h"
#include "Interp.h"


#pragma  region EnumControl
EnumControl::EnumControl(IRECT rect, int paramIdx, const IText& textStyle)
	: IControl(rect, paramIdx)
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
    // #TODO get the number of presets
    mMax = 1; // GetDelegate()->NPresets() - 1;
	}
}

void EnumControl::Draw(IGraphics& g)
{
	g.FillRect(mText.mTextEntryBGColor, mRECT);
	g.DrawRect(mText.mTextEntryFGColor, mRECT);

	// buttons
	IColor buttonColor = mText.mTextEntryFGColor;
  // #TODO get current preset index
  const int value = mParamIdx < kNumParams ? GetParam()->Int() : 0; // mPlug->GetCurrentPresetIdx();
	if (value == mMin || IsGrayed())
	{
		buttonColor.R *= 0.5f; buttonColor.G *= 0.5f; buttonColor.B *= 0.5f;
	}
	g.FillTriangle(buttonColor, mDecrementRect.L, mDecrementRect.MH(), mDecrementRect.R, mDecrementRect.T, mDecrementRect.R, mDecrementRect.B, 0);

	buttonColor = mText.mTextEntryFGColor;
	if (value == mMax || IsGrayed())
	{
		buttonColor.R *= 0.5f; buttonColor.G *= 0.5f; buttonColor.B *= 0.5f;
	}
	g.FillTriangle(buttonColor, mIncrementRect.L, mIncrementRect.T, mIncrementRect.R, mIncrementRect.MH(), mIncrementRect.L, mIncrementRect.B, 0);

	char* label = 0;
	if (mParamIdx < kNumParams)
	{
    WDL_String display;
		GetParam()->GetDisplayForHost(display);
		label = display.Get();
	}
	else
	{
    // #TODO get preset name
    label = ""; // const_cast<char*>(mPlug->GetPresetName(mPlug->GetCurrentPresetIdx()));
	}

	IRECT textRect = mTextRect;
	// vertically center the text
	g.MeasureText(mText, label, textRect);
#ifdef OS_OSX
	textRect.B -= 4;
#endif
	int offset = (mTextRect.H() - textRect.H()) / 2;
	textRect.T += offset;
	textRect.B += offset;
	mText.mFGColor.A = IsGrayed() ? 128 : 255;
	g.DrawText(mText, label, textRect);
}

void EnumControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
{
	if (pMod.L)
	{
		if (mPopupRect.Contains(x, y))
		{
			if (mParamIdx < kNumParams)
			{
				PromptUserInput();
			}
			else // need to build the popup for presets by hand
			{
        // #TODO preset popup
				//IPopupMenu menu;
				//const int currentPresetIdx = mPlug->GetCurrentPresetIdx();
				//for (int i = 0; i < mPlug->NPresets(); ++i)
				//{
				//	const char * presetName = mPlug->GetPresetName(i);
				//	if (i == currentPresetIdx)
				//	{
				//		menu.AddItem(new IPopupMenuItem(presetName, IPopupMenuItem::kChecked), -1);
				//	}
				//	else
				//	{
				//		menu.AddItem(new IPopupMenuItem(presetName), -1);
				//	}
				//}

				//if (GetGUI()->CreateIPopupMenu(&menu, &mRECT))
				//{
				//	mPlug->RestorePreset(menu.GetChosenItemIdx());
				//	SetDirty(false);
				//}
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
	else if (pMod.R && mParamIdx < kNumParams)
	{
		Interface::BeginMIDILearn(GetDelegate(), mParamIdx, -1, x, y);
	}
}

void EnumControl::OnMouseWheel(float x, float y, const IMouseMod& pMod, float d)
{
	StepValue(d);
}

void EnumControl::StepValue(int amount)
{
	const bool isParam = mParamIdx < kNumParams;
	if (isParam)
	{
		int count = GetParam()->NDisplayTexts();
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
    // #TODO restore preset correctly
		//const int presetIdx = BOUNDED(mPlug->GetCurrentPresetIdx() + amount, mMin, mMax);
		//if (presetIdx != mPlug->GetCurrentPresetIdx())
		//{
		//	mPlug->RestorePreset(presetIdx);
		//}
	}

	SetDirty(isParam);
}
#pragma  endregion EnumControl


#pragma  region BangControl

BangControl::BangControl(IRECT iRect, Action action, IColor onColor, IColor offColor, IText* textStyle /*= nullptr*/, const char * label /*= nullptr*/, int paramIdx /*= -1*/, const char * fileTypes /*= "fxp"*/)
	: IControl(iRect, paramIdx)
	, mAction(action)
	, mOnColor(onColor)
	, mOffColor(offColor)
	, mLabel(label)
	, mFileTypes(fileTypes)
{
	if (textStyle != nullptr)
	{
		SetText(*textStyle);
	}
	mDblAsSingleClick = true;
}

void BangControl::Draw(IGraphics& g)
{
	if (mValue > 0.5)
	{
		g.FillRect(mOnColor, mRECT);
	}
	else
	{
		g.FillRect(mOffColor, mRECT);
	}

	//pGraphics->DrawRect(&mOnColor, &mRECT);

	if (mLabel != nullptr)
	{
		char * label = const_cast<char*>(mLabel);
		IRECT textRect = mRECT;
		// vertically center the text
		g.MeasureText(mText, label, textRect);
#ifdef OS_OSX
		textRect.B -= 4;
#endif
		int offset = (mRECT.H() - textRect.H()) / 2;
		textRect.T += offset;
		textRect.B += offset;
		g.MeasureText(mText, label, textRect);
		g.DrawText(mText, label, textRect);
	}
}

void BangControl::OnMouseDown(float x, float y, const IMouseMod& pMod)
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
			GetUI()->PromptForFile(fileName, directory, kFileSave, mFileTypes);
			if (fileName.GetLength() > 0)
			{
				PLUG_CLASS_NAME * plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
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
			GetUI()->PromptForFile(fileName, directory, kFileOpen, mFileTypes);
			if (fileName.GetLength() > 0)
			{
				PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
				if (plug != nullptr)
				{
					plug->HandleLoad(&fileName, &directory);
				}
			}
		}
		break;

		case ActionDumpPreset:
		{
			PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
			if (plug != nullptr)
			{
				plug->DumpPresetSrc();
			}
		}
		break;

		case ActionCustom:
		default:
		{
			PLUG_CLASS_NAME* plug = static_cast<PLUG_CLASS_NAME*>(GetDelegate());
			if (plug != nullptr)
			{
				plug->HandleAction(mAction);
			}
			mValue = 1;
			SetDirty(false);
		}
    break;

		}
	}
}

void BangControl::OnMouseUp(float x, float y, const IMouseMod& pMod)
{
	if (mParamIdx < kNumParams)
	{
		mValue = 0;
		SetDirty();
	}
}

#pragma  endregion 

#pragma  region PeaksControl
PeaksControl::PeaksControl(IRECT rect, IColor backColor, IColor peaksColor)
	: IPanelControl(rect, &backColor)
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

void PeaksControl::Draw(IGraphics& g)
{
	IPanelControl::Draw(g);

	for (int i = 0; i < mPeaksSize; ++i)
	{
		int x = mRECT.L + i;
		int ph = mRECT.H()*mPeaks[i];
		int y1 = mRECT.MH() + ph / 2;
		int y2 = mRECT.MH() - ph / 2;
		g.DrawLine(mPeaksColor, x, y1, x, y2);
	}
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
}
#pragma  endregion PeaksControl

#pragma  region ShaperVizControl
const float kVizTriangleSize = 5;
ShaperVizControl::ShaperVizControl(IRECT rect, IColor bracketColor, IColor lineColor)
	: IControl(rect)
	, mBracketColor(bracketColor)
	, mLineColor(lineColor)
{

}

void ShaperVizControl::Draw(IGraphics& pGraphics)
{
	WaveShaper* shaper = dynamic_cast<WaveShaper*>(GetDelegate());
	if (shaper != nullptr)
	{
		const float center = Map(shaper->GetNoiseOffset(), -1, 1, mRECT.L, mRECT.R);
		const float widthPct = shaper->GetShape();
		const float waveWidth = mRECT.W();
		const float shaperSize = shaper->GetShaperSize();
		const float sampleWidth = (shaperSize * widthPct);
		const float chunkSize = shaperSize / mRECT.W();
		const float halfWidth = sampleWidth / chunkSize * 0.5f;

		float x1 = center - halfWidth, x2 = center + halfWidth;
		int y1 = mRECT.T, y2 = mRECT.B - 1;

		// this will be [0, 1]
		const float mapLookup = shaper->GetShaperMapValue();
		const float lx = Lerp(mRECT.L, mRECT.R, mapLookup);
		pGraphics->DrawLine(&mLineColor, lx, y1, lx, y2-1);

		IChannelBlend blend(IChannelBlend::kBlendNone, 0.4f);
		if (x1 < mRECT.L)
		{
			x1 += waveWidth;
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1, y2);
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1 + 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x1, y2, mRECT.R, y2);
			//pGraphics->DrawLine(&mBracketColor, mRECT.L, y2, center, y2);

			IRECT rect(x1, y1, mRECT.R, y2);
			pGraphics->FillIRect(&mBracketColor, &rect, &blend);
			rect = IRECT(mRECT.L, y1, center, y2);
			pGraphics->FillIRect(&mBracketColor, &rect, &blend);
		}
		else
		{
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1, y2);
			//pGraphics->DrawLine(&mBracketColor, x1, y1, x1 + 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x1, y2, center, y2);
			IRECT rect(x1, y1, center, y2);
			pGraphics->FillIRect(&mBracketColor, &rect, &blend);
		}		

		if (x2 > mRECT.R)
		{
			x2 -= waveWidth;
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2, y2);
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2 - 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x2, y2, mRECT.L, y2);
			//pGraphics->DrawLine(&mBracketColor, center, y2, mRECT.R, y2);

			IRECT rect(mRECT.L, y1, x2, y2);
			pGraphics->FillIRect(&mBracketColor, &rect, &blend);
			rect = IRECT(center, y1, mRECT.R, y2);
			pGraphics->FillIRect(&mBracketColor, &rect, &blend);
		}
		else
		{
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2, y2);
			//pGraphics->DrawLine(&mBracketColor, x2, y1, x2 - 4, y1);
			//pGraphics->DrawLine(&mBracketColor, x2, y2, center, y2);
			IRECT rect(center, y1, x2, y2);
			pGraphics->FillIRect(&mBracketColor, &rect, &blend);
		}

		if (center > mRECT.L + kVizTriangleSize && center < mRECT.R - kVizTriangleSize)
		{
			pGraphics->FillTriangle(&mBracketColor, center, y2, center - kVizTriangleSize, y2, center, y2 - kVizTriangleSize, 0);
			pGraphics->FillTriangle(&mBracketColor, center, y2, center + kVizTriangleSize, y2, center, y2 - kVizTriangleSize, 0);
		}
		else
		{
			x1 = mRECT.L;
			x2 = mRECT.R;
			pGraphics->FillTriangle(&mBracketColor, x1, y2, x1 + kVizTriangleSize, y2, x1, y2 - kVizTriangleSize, 0);
			pGraphics->FillTriangle(&mBracketColor, x2, y2, x2 - kVizTriangleSize, y2, x2, y2 - kVizTriangleSize, 0);
		}
		
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
	mPointRect = mRECT.GetPadded(-pointRadius - 1);
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
		mPointX = BOUNDED(mPointX + dX, mPointRect.L, mPointRect.R);
		mPointY = BOUNDED(mPointY + dY, mPointRect.T, mPointRect.B);

		mTargetRECT = IRECT(mPointX - mPointRadius, mPointY - mPointRadius, mPointX + mPointRadius, mPointY + mPointRadius);

		GetAuxParam(0)->mValue = Map(mPointX, mPointRect.L, mPointRect.R, 0, 1);
		GetAuxParam(1)->mValue = Map(mPointY, mPointRect.B, mPointRect.T, 0, 1);
		SetAllAuxParamsFromGUI();
		SetDirty(false);
	}
}

void XYControl::SetAuxParamValueFromPlug(int auxParamIdx, double value)
{
	IControl::SetAuxParamValueFromPlug(auxParamIdx, value);

	if (auxParamIdx == 0)
	{
		mPointX = Map(value, 0, 1, mPointRect.L, mPointRect.R);
	}
	else if (auxParamIdx == 1)
	{
		mPointY = Map(value, 0, 1, mPointRect.B, mPointRect.T);
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
	mPointRect = mRECT.GetPadded(-pointRadius - 1);
}

bool SnapshotControl::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mBackgroundColor, &mRECT);

	WaveShaper* shaper = dynamic_cast<WaveShaper*>(mPlug);
	if (shaper != nullptr)
	{
		WaveShaper::NoiseSnapshot snapshot = shaper->GetNoiseSnapshotNormalized(mSnapshotIdx);
		
		float x = ::Lerp(mPointRect.L, mPointRect.R, snapshot.AmpMod);
		float y = ::Lerp(mPointRect.B, mPointRect.T, snapshot.Rate);
		pGraphics->FillCircle(&mPointColorA, x, y, mPointRadius, 0, true);
		
		x = ::Lerp(mPointRect.L, mPointRect.R, snapshot.Range);
		y = ::Lerp(mPointRect.B, mPointRect.T, snapshot.Shape);
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
	if (pMod->L)
	{
		mValue = GetParam()->GetNormalized(mSnapshotIdx);
		mHighlight = 1;
		SetDirty();
		GetGUI()->SetParameterFromGUI(mParamIdx, mValue);
	}
}

void SnapshotControl::Update()
{
	WaveShaper* shaper = dynamic_cast<WaveShaper*>(mPlug);
	if (shaper != nullptr)
	{
		shaper->UpdateNoiseSnapshot(mSnapshotIdx);
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

#pragma  region PlayStopControl
PlayStopControl::PlayStopControl(IPlugBase* pPlug, IRECT rect, IColor backgroundColor, IColor foregroundColor)
	: IPanelControl(pPlug, rect, &backgroundColor)
	, mForeground(foregroundColor)
{
	mIconRect = mRECT.GetPadded(-8);
	mDblAsSingleClick = true;
}

void PlayStopControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	IMidiMsg msg;
	if (mValue)
	{
		msg.MakeNoteOffMsg(0, 0);
		mValue = 0;
	}
	else
	{
		msg.MakeNoteOnMsg(0, 127, 0);
		mValue = 1;
	}
	mPlug->ProcessMidiMsg(&msg);
	SetDirty(false);
}

bool PlayStopControl::Draw(IGraphics* pGraphics)
{
	IPanelControl::Draw(pGraphics);

	if (mValue)
	{
		pGraphics->FillIRect(&mForeground, &mIconRect);
	}
	else
	{
		pGraphics->FillTriangle(&mForeground, mIconRect.L, mIconRect.T, mIconRect.R, (int)mIconRect.MH(), mIconRect.L, mIconRect.B, 0);
	}

	return true;
}

#pragma endregion
