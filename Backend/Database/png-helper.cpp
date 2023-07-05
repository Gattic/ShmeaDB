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
#include "png-helper.h"
#include "image.h"

using namespace shmea;

void PNGHelper::applyRainbowFilter(png_bytep row, png_bytep originalRow, png_uint_32 width, png_byte alpha)
{
    for (png_uint_32 i = 0; i < width; i++)
    {
        double hue = static_cast<double>(i) / static_cast<double>(width - 1) * 360.0; // 0.0 - 360.0
        double saturation = 1.0; // 0.0 - 1.0
        double value = 1.0; // Brightness: 0.0 - 1.0

        double c = value * saturation; // Chroma: 0.0 - 1.0
        double x = c * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
        double m = value - c; // Match value to brightness

        double r, g, b;

        if (hue >= 0 && hue < 60)
	{
            r = c;
            g = x;
            b = 0;
        }
	else if (hue >= 60 && hue < 120)
	{
            r = x;
            g = c;
            b = 0;
        }
	else if (hue >= 120 && hue < 180)
	{
            r = 0;
            g = c;
            b = x;
        }
	else if (hue >= 180 && hue < 240)
	{
            r = 0;
            g = x;
            b = c;
        }
	else if (hue >= 240 && hue < 300)
	{
            r = x;
            g = 0;
            b = c;
        }
	else
	{
            r = c;
            g = 0;
            b = x;
        }

	// Get the original pixel
        png_byte originalR = originalRow[i * 4];
        png_byte originalG = originalRow[i * 4 + 1];
        png_byte originalB = originalRow[i * 4 + 2];
        png_byte originalA = originalRow[i * 4 + 3];

	// Apply the filter
        png_byte filteredR = static_cast<png_byte>((r + m) * 255.0);
        png_byte filteredG = static_cast<png_byte>((g + m) * 255.0);
        png_byte filteredB = static_cast<png_byte>((b + m) * 255.0);

        originalRow[i * 4] = static_cast<png_byte>((filteredR * alpha / 255.0) + (originalR * (255 - alpha) / 255.0));     // Red
        originalRow[i * 4 + 1] = static_cast<png_byte>((filteredG * alpha / 255.0) + (originalG * (255 - alpha) / 255.0)); // Green
        originalRow[i * 4 + 2] = static_cast<png_byte>((filteredB * alpha / 255.0) + (originalB * (255 - alpha) / 255.0)); // Blue
        originalRow[i * 4 + 3] = originalA; // Alpha
    }
}

void PNGHelper::readImage(const char* inputPath, png_structp& png, png_infop& info, png_bytep*& rowPointers, png_uint_32& width, png_uint_32& height, png_byte& bitDepth, png_byte& colorType)
{
    // Open the input file
    FILE* file = fopen(inputPath, "rb");
    if (!file)
    {
        printf("Error opening input file: %s\n", inputPath);
        return;
    }

    // Read the PNG header
    png_byte header[8];
    fread(header, 1, 8, file);
    if (png_sig_cmp(header, 0, 8))
    {
        printf("Error reading input PNG file: %s\n", inputPath);
        fclose(file);
        return;
    }

    // Create PNG read struct
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        printf("Error creating PNG read struct!\n");
        fclose(file);
        return;
    }

    // Create PNG info struct
    info = png_create_info_struct(png);
    if (!info)
    {
        printf("Error creating PNG info struct!\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(file);
        return;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        printf("Error during PNG read!\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(file);
        return;
    }

    png_init_io(png, file);
    png_set_sig_bytes(png, 8);

    // Read the image info
    png_read_info(png, info);

    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    bitDepth = png_get_bit_depth(png, info);
    colorType = png_get_color_type(png, info);

    // Transform palette images to RGB
    if (colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png);
    }

    // Transform grayscale images to RGB
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
    {
        bitDepth = 8;
        png_set_expand_gray_1_2_4_to_8(png);
    }

    // Expand bit depths less than 8 to 8
    if (bitDepth < 8)
    {
        png_set_expand(png);
    }

    // Add alpha channel if image doesn't have one
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY ||
        colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    }

    // Update the info structure
    png_read_update_info(png, info);

    // Allocate memory for image data
    rowPointers = new png_bytep[height];
    for (png_uint_32 y = 0; y < height; y++)
    {
        rowPointers[y] = new png_byte[png_get_rowbytes(png, info)];
    }

    // Read the image data
    png_read_image(png, rowPointers);

    // Close the input file
    fclose(file);
}

void PNGHelper::writeImage(const char* outputPath, png_structp png, png_infop info, png_bytep* rowPointers, png_uint_32 width, png_uint_32 height, png_byte bitDepth, png_byte colorType)
{
    // Open the output file
    FILE* file = fopen(outputPath, "wb");
    if (!file)
    {
        printf("Error opening output file: %s\n", outputPath);
        png_destroy_read_struct(&png, &info, NULL);
        delete[] rowPointers;
        return;
    }

    // Create PNG write struct
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        printf("Error creating PNG write struct!\n");
        fclose(file);
        png_destroy_read_struct(&png, &info, NULL);
        delete[] rowPointers;
        return;
    }

    info = png_create_info_struct(png);
    if (!info)
    {
        printf("Error creating PNG info struct!\n");
        fclose(file);
        png_destroy_write_struct(&png, NULL);
        delete[] rowPointers;
        return;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        printf("Error during PNG write!\n");
        fclose(file);
        png_destroy_write_struct(&png, &info);
        delete[] rowPointers;
        return;
    }

    png_init_io(png, file);

    // Set the image info
    png_set_IHDR(png, info, width, height, bitDepth, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // Write the image data
    png_write_image(png, rowPointers);

    // End writing
    png_write_end(png, NULL);

    // Clean up
    fclose(file);
    png_destroy_write_struct(&png, &info);
    delete[] rowPointers;

    printf("Image processed and saved to: %s\n", outputPath);
}

void PNGHelper::pngTest(const char* inputPath, const char* outputPath)
{
    png_structp png = NULL;
    png_infop info = NULL;
    png_bytep* rowPointers = NULL;
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    png_byte bitDepth = 0;
    png_byte colorType = 0;

    readImage(inputPath, png, info, rowPointers, width, height, bitDepth, colorType);

    // Apply the rainbow filter to each pixel row
    for (png_uint_32 y = 0; y < height; y++)
    {
        png_bytep row = rowPointers[y];
        png_bytep originalRow = row;
        if (y != 0)
        {
            originalRow = rowPointers[y - 1]; // Use the row above as the original image for overlay
        }
        applyRainbowFilter(row, originalRow, width, 32); // Alpha value of 128 for transparency
    }

    writeImage(outputPath, png, info, rowPointers, width, height, bitDepth, colorType);

    // Clean up
    png_destroy_read_struct(&png, &info, NULL);
    delete[] rowPointers;
}

void PNGHelper::LoadPNG(Image& image, const char* inputPath)
{
    png_structp png = NULL;
    png_infop info = NULL;
    png_bytep* rowPointers = NULL;
    png_uint_32 width = 0;
    png_uint_32 height = 0;
    png_byte bitDepth = 0;
    png_byte colorType = 0;

    readImage(inputPath, png, info, rowPointers, width, height, bitDepth, colorType);
    image.Allocate(width, height);

    // Copy image data to the Image class
    for (png_uint_32 y = 0; y < height; ++y)
    {
        png_bytep row = rowPointers[y];
        for (png_uint_32 x = 0; x < width; ++x)
        {
            png_bytep pixel = &(row[x * 4]);
            shmea::RGBA rgba(pixel[0], pixel[1], pixel[2], pixel[3]);
            image.SetPixel(x, y, rgba);
        }
    }

    // Clean up
    png_destroy_read_struct(&png, &info, NULL);
    delete[] rowPointers;
}
