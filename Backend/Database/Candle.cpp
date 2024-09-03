// Copyright 2020 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "Candle.h"


Candle::Candle()
{
	setX(0.0f);
	setOpen(0.0f);
	setClose(0.0f);
	setHigh(0.0f);
	setLow(0.0f);
}

Candle::Candle(float newX, float newOpen, float newClose, float newHigh, float newLow)
{
	setX(newX);
	setOpen(newOpen);
	setClose(newClose);
	setHigh(newHigh);
	setLow(newLow);
}

Candle::Candle(const Candle& c2)
{
	setX(c2.x);
	setOpen(c2.open);
	setClose(c2.close);
	setHigh(c2.high);
	setLow(c2.low);
}

Candle::~Candle()
{
	setX(0.0f);
	setOpen(0.0f);
	setClose(0.0f);
	setHigh(0.0f);
	setLow(0.0f);
}

float Candle::getX() const
{
	return x;
}

float Candle::getOpen() const
{
	return open;
}

float Candle::getClose() const
{
	return close;
}

float Candle::getHigh() const
{
	return high;
}

float Candle::getLow() const
{
	return low;
}

void Candle::set(float newX, float newOpen, float newClose, float newHigh, float newLow)
{
	setX(newX);
	setOpen(newOpen);
	setClose(newClose);
	setHigh(newHigh);
	setLow(newLow);
}

void Candle::setX(float newX)
{
	x = newX;
}

void Candle::setOpen(float newOpen)
{
	open = newOpen;
}

void Candle::setClose(float newClose)
{
	close = newClose;
}

void Candle::setHigh(float newHigh)
{
	high = newHigh;
}

void Candle::setLow(float newLow)
{
	low = newLow;
}
