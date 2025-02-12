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

void Image::drawVerticalGradient(int x, int y, RGBA color1, RGBA color2, int cornerRadius)
{
    int cHeight = static_cast<int>(height);
    int cWidth = static_cast<int>(width);

    int startY = y;
    int endY = y + cHeight;

    // Calculate color deltas with type casting to float for precision
    float deltaR = (static_cast<float>(color2.r) - static_cast<float>(color1.r)) / cHeight;
    float deltaG = (static_cast<float>(color2.g) - static_cast<float>(color1.g)) / cHeight;
    float deltaB = (static_cast<float>(color2.b) - static_cast<float>(color1.b)) / cHeight;
    float deltaA = (static_cast<float>(color2.a) - static_cast<float>(color1.a)) / cHeight;

    int rectXW = x + cWidth - 1;
    int rectYH = y + cHeight - 1;

    // Draw the vertical gradient with adjustable rounded corners
    for (int currentY = startY; currentY < endY; ++currentY) {
        // Calculate current color and cast to unsigned char
        unsigned char r = static_cast<unsigned char>(color1.r + deltaR * (currentY - startY));
        unsigned char g = static_cast<unsigned char>(color1.g + deltaG * (currentY - startY));
        unsigned char b = static_cast<unsigned char>(color1.b + deltaB * (currentY - startY));
        unsigned char a = static_cast<unsigned char>(color1.a + deltaA * (currentY - startY));

        // Iterate over the width of the rectangle
        for (int currentX = x; currentX <= rectXW; ++currentX) {
            // Check if the pixel is within the rounded corner area
            bool drawPixel = true;
            int dx = 0, dy = 0;

            if (currentX < x + cornerRadius) {
                dx = currentX - (x + cornerRadius);
                if (currentY < y + cornerRadius)
                    dy = currentY - (y + cornerRadius);
                else if (currentY >= rectYH - cornerRadius)
                    dy = currentY - (rectYH - cornerRadius);
            } else if (currentX >= rectXW - cornerRadius) {
                dx = currentX - (rectXW - cornerRadius);
                if (currentY < y + cornerRadius)
                    dy = currentY - (y + cornerRadius);
                else if (currentY >= rectYH - cornerRadius)
                    dy = currentY - (rectYH - cornerRadius);
            } else if (currentY < y + cornerRadius || currentY >= rectYH - cornerRadius) {
                if (currentX < x || currentX >= rectXW)
                    drawPixel = false;
            }

            // Draw the pixel if it's not excluded by the corner radius
            if (drawPixel) {
                int radiusSquared = cornerRadius * cornerRadius;
                if ((dx * dx + dy * dy) <= radiusSquared) {
                    SetPixel(currentX, currentY, RGBA(r, g, b, a));
                }
            }
        }
    }
}

RGBA Image::averageColor(int startX, int startY, int blockWidth, int blockHeight)
{
    int totalR = 0, totalG = 0, totalB = 0, totalA = 0;
    int count = 0;

    for (int y = 0; y < blockHeight; ++y) {
        for (int x = 0; x < blockWidth; ++x) {
            // Retrieve the pixel color from the high-resolution image
            RGBA pixel = GetPixel(startX + x, startY + y);

            // Sum the color values
            totalR += pixel.r;
            totalG += pixel.g;
            totalB += pixel.b;
            totalA += pixel.a;
            count++;
        }
    }

    // Calculate the average values and return the resulting color
    return RGBA(
        static_cast<unsigned char>(totalR / count),
        static_cast<unsigned char>(totalG / count),
        static_cast<unsigned char>(totalB / count),
        static_cast<unsigned char>(totalA / count)
    );


}

std::vector<unsigned char> Image::getPixels() const 
{
	std::vector<unsigned char> pixels; //Store RGBA Data
	pixels.reserve(getPixelCount() * 4);

	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			RGBA c = GetPixel(x, y); //pixel at x, y
			pixels.push_back(c.r); //Red
			pixels.push_back(c.g); //Green
			pixels.push_back(c.b); //Blue
			pixels.push_back(c.a); //Alpha
		}
	}

	return pixels; // Returns the vector of raw pixel data
}

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
	//for (unsigned int y = height - 1; y >= 0; y--)
	for(unsigned int y = 0; y < height; ++y)//TODO CHECK THIS
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			RGBA c;
			c.r = fgetc(file);
			c.g = fgetc(file);
			c.b = fgetc(file);
			c.a = 0xFF;
			//SetPixel(x, y, c);
			SetPixel(x, height - 1 - y, c);
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
	for (unsigned int i = 0; i < height; ++i)
	{
		// read a row from the file of packed data
		fread(packedData, sizeof(char), rowsize, file);
		for (int k = 0; k < rowsize; ++k)
		{											// for each byte in the row
			unsigned char packed_d = packedData[k]; // temporary char of packed bits
			for (unsigned int j = 0; j < 8; ++j)
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
	//for (unsigned int y = height - 1; y >= 0; y--)
	for(unsigned int y = 0; y < height; ++y)//TODO CHECK THIS
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			//RGBA v = GetPixel(x, y);
			RGBA v = GetPixel(x, height - 1 - y);
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
	for (unsigned int i = 0; i < height; ++i)
	{
		// pack a row of pixels
		for (int k = 0; k < rowsize; ++k)
		{								// for each byte in line to write,
			unsigned char packed_d = 0; // initialize a packed byte to 0
			for (unsigned int j = 0; j < 8; ++j)
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
	//for (unsigned int y = height - 1; y >= 0; --y)
	for (unsigned int y = 0; y < height; ++y)//TODO CHECK THIS
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			RGBA c;
			c.a = fgetc(file);
			c.g = fgetc(file);
			c.r = fgetc(file);
			c.b = fgetc(file);
			//SetPixel(x, y, c);
			SetPixel(x, height - 1 - y, c);
		}
	}

	fclose(file);

	printf("[IMG] Loaded BMP: %s(%d,%d)\n", filename.c_str(), width, height);
}

void Image::SavePNG(const GString& filename) const
{
	int len = filename.length();
	if (!(len > 4 && filename.substr(len - 4) == GString(".png")))
	{
		printf("ERROR: This is not a PNG filename: %s\n", filename.c_str());
		return;
	}

	// Save the image
	PNGHelper::SavePNG(*this, filename.c_str());

	printf("[IMG] Saved PNG: %s(%d,%d)\n", filename.c_str(), width, height);

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

    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            RGBA c = GetPixel(x, y);

            // Pack RGBA into a single 32-bit integer (0xRRGGBBAA)
            unsigned int packedRGBA = (static_cast<unsigned int>(c.r) << 24) |
                                       (static_cast<unsigned int>(c.g) << 16) |
                                       (static_cast<unsigned int>(c.b) << 8)  |
                                       static_cast<unsigned int>(c.a);

            // Add the packed integer to the list
            retList.addInt(static_cast<int>(packedRGBA));
        }
    }

    return retList;
}

bool Image::unflatten(const shmea::GList& pixels)
{
    if (pixels.size() != (unsigned int)width * height * 4)
	return false;

    unsigned int i = 0;
    for (unsigned int y = 0; y < height; ++y)
    {
	for (unsigned int x = 0; x < width; ++x)
	{
	    RGBA c;
	    c.r = pixels.getInt(i++);
	    c.g = pixels.getInt(i++);
	    c.b = pixels.getInt(i++);
	    c.a = pixels.getInt(i++);
	    SetPixel(x, y, c);
	}
    }

    return true;
}

shmea::GString Image::hash() const
{
    shmea::GString ret;

    for (unsigned int y = 0; y < height; ++y)
    {
	for (unsigned int x = 0; x < width; ++x)
	{
	    RGBA c = GetPixel(x, y);
	    ret += shmea::GString::intTOstring(c.r);
	    ret += shmea::GString::intTOstring(c.g);
	    ret += shmea::GString::intTOstring(c.b);
	    ret += shmea::GString::intTOstring(c.a);
	}
    }

    return ret;
}

bool Image::operator<(const Image& sd2) const
{
	return hash() < sd2.hash();
}

bool Image::operator>(const Image& sd2) const
{
	return hash() > sd2.hash();
}
