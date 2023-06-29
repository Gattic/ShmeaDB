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
#include "image.h"
#include "GString.h"
#include "GList.h"
#include "png-helper.h"

using namespace shmea;

bool Image::LoadPPM(const GString& fname)
{
	// valid file name?
	if (fname.length() == 0)
		return false;

	int len = fname.length();
	if (!(len > 4 && fname.substr(len - 4) == GString(".ppm")))
	{
		printf("ERROR: This is not a PPM fname: %s\n", fname.c_str());
		return false;
	}

	FILE* file = fopen(fname.c_str(), "rb");
	if (file == NULL)
	{
		printf("Unable to open %s for reading\n", fname.c_str());
		return false;
	}

	// misc header information
	char tmp[100];
	fgets(tmp, 100, file);
	if (!(strstr(tmp, "P6")))
		return false;

	fgets(tmp, 100, file);
	while (tmp[0] == '#')
	{
		fgets(tmp, 100, file);
	}

	sscanf(tmp, "%d %d", &width, &height);
	fgets(tmp, 100, file);
	if (!(strstr(tmp, "255")))
		return false;

	// the data
	delete[] data;
	data = new RGBA[height * width];

	// flip y so that (0,0) is bottom left corner
	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < width; x++)
		{
			RGBA c;
			c.r = fgetc(file);
			c.g = fgetc(file);
			c.b = fgetc(file);
			c.a = 0xFF;
			SetPixel(x, y, c);
		}
	}

	fclose(file);
	return true;
}

bool Image::LoadPBM(const GString& fname)
{
	// valid file name?
	if (fname.length() == 0)
		return false;

	int len = fname.length();
	if (!(len > 4 && fname.substr(len - 4) == GString(".pbm")))
	{
		printf("ERROR: This is not a PBM fname: %s\n", fname.c_str());
		return false;
	}

	FILE* file = fopen(fname.c_str(), "rb");
	if (file == NULL)
	{
		printf("Unable to open %s for reading\n", fname.c_str());
		return false;
	}

	// read file identifier (magic number)
	char buffer[100];
	fgets(buffer, sizeof(buffer), file);
	if ((buffer[0] != 'P') || (buffer[1] != '4'))
	{
		printf("Not a simple pbm file\n");
		return false;
	}

	// read image size
	do
	{
		fgets(buffer, sizeof(buffer), file);
		// Skip all comments "#"
		// Also skip any newlines that might (but shouldn't) be there
	} while (buffer[0] == '#' || buffer[0] == 10);

	// Read the width and height of the  image
	sscanf(buffer, "%d %d", &width, &height);

	// Allocate the buffer
	if (data)
		delete[] data; // don't leak!!

	data = new RGBA[width * height]; // 1 RGBA per pixel

	// Read in the pixel array row-by-row
	// each row is width bits, packed 8 to a byte
	int rowsize = (width + 7) / 8;							// the size of each row in bytes
	unsigned char* packedData = new unsigned char[rowsize]; // array of row bits to unpack

	// for each line of the image
	for (int i = 0; i < height; ++i)
	{
		// read a row from the file of packed data
		fread(packedData, sizeof(char), rowsize, file);
		for (int k = 0; k < rowsize; ++k)
		{											// for each byte in the row
			unsigned char packed_d = packedData[k]; // temporary char of packed bits
			for (int j = 0; j < 8; ++j)
			{

				// special case last byte, might not be enough bits to fill
				if ((k * 8 + j) == width)
					break;

				// an ugly one linear that extracts j'th bit as a new byte
				unsigned char temp = (packed_d >> (7 - j)) & 1;

				// in a .pbm file, 1 == true == black
				RGBA c;
				c.r = (temp == 1) ? 0x00 : 0xFF;
				c.g = (temp == 1) ? 0x00 : 0xFF;
				c.b = (temp == 1) ? 0x00 : 0xFF;
				c.a = 0xFF;
				SetPixel(k * 8 + j, height - 1 - i, c);
			}
		}
	}

	// close the file
	fclose(file);
	delete[] packedData;
	return true;
}

bool Image::SavePPM(const GString& filename) const
{
	int len = filename.length();
	if (!(len > 4 && filename.substr(len - 4) == GString(".ppm")))
	{
		printf("ERROR: This is not a PPM filename: %s\n", filename.c_str());
		return false;
	}

	FILE* file = fopen(filename.c_str(), "wb");
	if (file == NULL)
	{
		printf("Unable to open %s for writing\n", filename.c_str());
		return false;
	}

	// misc header information
	fprintf(file, "P6\n");
	fprintf(file, "%d %d\n", width, height);
	fprintf(file, "255\n");

	// the data
	// flip y so that (0,0) is bottom left corner
	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < width; x++)
		{
			RGBA v = GetPixel(x, y);
			fputc((unsigned char)(v.r), file);
			fputc((unsigned char)(v.g), file);
			fputc((unsigned char)(v.b), file);
		}
	}

	fclose(file);
	return true;
}

bool Image::SavePBM(const GString& filename) const
{
	int len = filename.length();
	if (!(len > 4 && filename.substr(len - 4) == GString(".pbm")))
	{
		printf("ERROR: This is not a PBM filename: %s\n", filename.c_str());
		return false;
	}

	FILE* file = fopen(filename.c_str(), "wb");
	if (file == NULL)
	{
		printf("Unable to open %s for writing\n", filename.c_str());
		return false;
	}

	// write the header information
	char tmp[20];
	tmp[0] = 'P';
	tmp[1] = '4';
	tmp[2] = 10;
	fwrite(tmp, sizeof(char), 3, file);
	sprintf(tmp, "%d %d", width, height);
	fwrite(tmp, strlen(tmp), 1, file);
	tmp[0] = 10;
	fwrite(tmp, sizeof(char), 1, file);

	// size of row in bytes
	int rowsize = (width + 7) / 8;							// the size of each row
	unsigned char* packedData = new unsigned char[rowsize]; // row of packed bytes to write

	// Write the image row by row
	for (int i = 0; i < height; ++i)
	{
		// pack a row of pixels
		for (int k = 0; k < rowsize; ++k)
		{								// for each byte in line to write,
			unsigned char packed_d = 0; // initialize a packed byte to 0
			for (int j = 0; j < 8; ++j)
			{
				// special case when not enough bits to fill last byte
				if ((k * 8 + j) == width)
				{
					packed_d <<= (8 - j);
					break;
				}
				packed_d <<= 1; // shift left one
				RGBA cPixel = GetPixel(k * 8 + j, height - 1 - i);
				if (cPixel.r && cPixel.g && cPixel.b && cPixel.a)
					packed_d = (packed_d | 1);
				else
					packed_d = (packed_d | 0);
			}
			packedData[k] = packed_d;
		}
		fwrite((void*)packedData, sizeof(unsigned char), rowsize, file);
	}

	fclose(file);
	delete[] packedData;
	return true;
}

void Image::LoadBMP(const GString& filename)
{
	int len = filename.length();
	if (!(len > 4 && filename.substr(len - 4) == GString(".bmp")))
	{
		printf("ERROR: This is not a BMP filename: %s\n", filename.c_str());
		return;
	}

	FILE* file = fopen(filename.c_str(), "rb");
	if (file == NULL)
	{
		printf("Unable to open %s for reading\n", filename.c_str());
		return;
	}

	// misc header information
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, file); // read the 54-byte header

	// extract image height and width from header
	int fileSize = *(int*)&info[2];
	int dataOffset = *(int*)&info[10];
	width = *(int*)&info[18];
	height = *(int*)&info[22];
	int depth = *(int*)&info[28];

	// flip y so that (0,0) is bottom left corner
	data = new RGBA[height * width];
	for (int y = height - 1; y >= 0; --y)
	{
		for (int x = 0; x < width; ++x)
		{
			RGBA c;
			c.a = fgetc(file);
			c.g = fgetc(file);
			c.r = fgetc(file);
			c.b = fgetc(file);
			SetPixel(x, y, c);
		}
	}

	fclose(file);

	printf("[IMG] Loaded BMP: %s(%d,%d)\n", filename.c_str(), width, height);
}

void Image::LoadPNG(const GString& filename)
{
	int len = filename.length();
	if (!(len > 4 && filename.substr(len - 4) == GString(".png")))
	{
		printf("ERROR: This is not a PNG filename: %s\n", filename.c_str());
		return;
	}

	// Load the image
	PNGHelper::LoadPNG(*this, filename.c_str());

	printf("[IMG] Loaded PNG: %s(%d,%d)\n", filename.c_str(), width, height);
}

shmea::GList Image::flatten() const
{
    shmea::GList retList;

    for (int y = 0; y < height; ++y)
    {
	for (int x = 0; x < width; ++x)
	{
		RGBA c = GetPixel(x, y);
		retList.addInt(c.r);
		retList.addInt(c.g);
		retList.addInt(c.b);
		retList.addInt(c.a);
	}
    }

    return retList;
}
