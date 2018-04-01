#pragma once

float Map(float value, float istart, float istop, float ostart, float ostop)
{
	return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

float Lerp(float from, float to, float t)
{
	return (1.0f - t)*from + t*to;
}
