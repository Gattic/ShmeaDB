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
#include "crypt.h"

using namespace GNet;

/*!
 * @brief encrypt a string
 * @details encrypt a cstring using a given implicit key and two random keys
 * @param src the string to encrypt
 * @param key the implicit key with which to encrypt the string
 * @param tSize the size of the block to encrypt
 */
void Crypt::encrypt(const char* src, int64_t key, unsigned int tSize)
{
	// info
	sizeClaimed = tSize;
	dText = shmea::GString(src, sizeClaimed);

	++sizeClaimed; // plus the key
	sizeCurrent = sizeClaimed;

	// I can make this assumption because this time already happened lol
	cTime = time(NULL);
	if (cTime < LEN_OFFSET / 10)
	{
		error = 1;
		return;
	}

	// random keys
	int lastDigit = cTime % 10;
	int secondLastDigit = (cTime % 100) / 10;
	shmea = lastDigit - 0x30;
	shmea = (shmea == 0) ? 7 : shmea; // lucky number 7
	brej = secondLastDigit - 0x30;
	brej = (brej == 0) ? 4 : brej; // i also like 4

	// encrypt
	int64_t len = ((int64_t)sizeClaimed) * LEN_OFFSET;
	int64_t newKeyRow = (cTime + len) * key;
	eText = shmea::GString((const char*)&newKeyRow, sizeof(int64_t));
	for (unsigned int i = 1; i < sizeClaimed; ++i)
	{
		int64_t newERow = dText[i - 1];
		newERow *= (int64_t)shmea;
		newERow += (int64_t)brej;
		newERow *= key;
		eText += shmea::GString((const char*)&newERow, sizeof(int64_t));
	}
}

/*!
 * @brief decrypt an encrypted block
 * @details decrypt an encrypted block using a given implicit key and two random keys
 * @param src a pointer to the first block to decrypt
 * @param key the implicit key with which to decrypt the string
 * @param srcLen the length of src, the first parameter
 */
void Crypt::decrypt(const int64_t* src, int64_t key, unsigned int srcLen)
{
	// info
	eText = shmea::GString((const char*)src, sizeof(int64_t) * srcLen);

	int64_t y = *(int64_t*)eText.substr(0, sizeof(int64_t)).c_str(); // switch to little endian
	int64_t firstLine = y / key;
	sizeClaimed = (int)(firstLine / LEN_OFFSET);
	unsigned int linesToRead = (srcLen > sizeClaimed) ? sizeClaimed : srcLen;
	if (linesToRead <= 0)
	{
		error = 2;
		return;
	}

	// I can make this assumption because this time already happened lol
	cTime = firstLine - (((int64_t)sizeClaimed) * LEN_OFFSET);
	if (cTime < LEN_OFFSET / 10)
	{
		error = 3;
		return;
	}

	// random keys
	int lastDigit = cTime % 10;
	int secondLastDigit = (cTime % 100) / 10;
	shmea = lastDigit - 0x30;
	shmea = (shmea == 0) ? 7 : shmea; // lucky number 7
	brej = secondLastDigit - 0x30;
	brej = (brej == 0) ? 4 : brej; // i also like 4

	// decrypt
	dText = "";
	unsigned int i = 1;
	for (; i < linesToRead; ++i)
	{
		y = *(int64_t*)eText.substr(i*sizeof(int64_t), sizeof(int64_t)).c_str(); // switch to little endian
		y /= key;
		y -= (int64_t)brej;
		y /= (int64_t)shmea;
		dText+=y;
	}

	// how many lines did we decrypt
	sizeCurrent = i;
}

void Crypt::decryptHeader(const shmea::GString& newText, int64_t key)
{
	// info
	eText = newText;

	int64_t y = *(int64_t*)eText.substr(0, sizeof(int64_t)).c_str(); // switch to little endian
	int64_t firstLine = y / key;
	sizeClaimed = (int)(firstLine / LEN_OFFSET);
	unsigned int linesToRead = (eText.length() > sizeClaimed) ? sizeClaimed : eText.length();
	if (linesToRead <= 0)
	{
		error = 2;
		return;
	}

	// I can make this assumption because this time already happened lol
	cTime = firstLine - (((int64_t)sizeClaimed) * LEN_OFFSET);
	if (cTime < LEN_OFFSET / 10)
	{
		error = 3;
		return;
	}

	sizeCurrent = 0;
}

int64_t Crypt::getTimesent() const
{
	return cTime;
}

