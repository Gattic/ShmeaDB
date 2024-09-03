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
/*
LodePNG Examples

Copyright (c) 2005-2012 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/
#include "png-helper.h"
#include "image.h"
#include "lodepng.h"

using namespace shmea;

void PNGHelper::createTestPNG(const char* filename)
{
	const unsigned width = 1200;
	const unsigned height = 800;
	Image image;
	image.Allocate(width, height);

	applyRainbowFilter(image, 32); // Alpha value of 128 for transparency
	
	SavePNG(image, filename);
}

void PNGHelper::createPNGFromData(const GTable& data, const char* outputPath)
{

	GList bt_ts = data.getCol(static_cast<unsigned>(0));
	GList raw_open = data.getCol(static_cast<unsigned>(2));
	GList raw_close = data.getCol(static_cast<unsigned>(3));
	GList raw_high = data.getCol(static_cast<unsigned>(4));
	GList raw_low = data.getCol(static_cast<unsigned>(5));

	//bt_ts
	//sizes
	printf("bt_ts size = %d\n", bt_ts.size());
	printf("raw_open size = %d\n", raw_open.size());
	printf("raw_close size = %d\n", raw_close.size());
	printf("raw_high size = %d\n", raw_high.size());
	printf("raw_low size = %d\n", raw_low.size());

	/*const unsigned width = 1200;
	const unsigned height = 800;

	Image image = new Image();
	image.Allocate(width, height);

	double min_time = bt_ts[0].getLong();
	double max_time = bt_ts[bt_ts.size() - 1].getLong();

	double min_price = *std::min_element(raw_low.begin(), raw_low.end());
	double max_price = *std::max_element(raw_high.begin(), raw_high.end());*/



	for(unsigned int  i = 0; i < bt_ts.size(); ++i)
	{
//		printf("bt_ts[%d] = %ld\n", i, bt_ts[i].getLong());
	}


	printf("Create PNG from data\n");

}

void PNGHelper::applyRainbowFilter(Image& image, unsigned int alpha)
{
    //Plot the pixels of the PNG file
    for(unsigned j = 0; j < image.height; ++j)
    {
	for(unsigned i = 0; i < image.width; ++i)
	{
	    double hue = static_cast<double>(i) / static_cast<double>(image.width - 1) * 360.0; // 0.0 - 360.0
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
	    unsigned char originalR = image.GetPixel(i, j).r; //red
	    unsigned char originalG = image.GetPixel(i, j).g; //green
	    unsigned char originalB = image.GetPixel(i, j).b; //blue
	    unsigned char originalA = image.GetPixel(i, j).a; //alpha

	    // Apply the filter
	    unsigned char filteredR = static_cast<unsigned char>((r + m) * 255.0);
	    unsigned char filteredG = static_cast<unsigned char>((g + m) * 255.0);
	    unsigned char filteredB = static_cast<unsigned char>((b + m) * 255.0);

	    // Set RGBA components
	    image.SetPixel(i, j, shmea::RGBA(filteredR, filteredG, filteredB, originalA));
	}
    }
}

// TODO: FIX THIS TO INCLUDE NEW COLOR TYPE AND BIT DEPTH PARAMS (work off of rep invarient then write)
// TODO: Also check writeImage to make sure it's writing the correct color type and bit depth
void PNGHelper::pngTest(const char* inputPath, const char* outputPath)
{
    Image image;
    LoadPNG(image, inputPath);

    // Apply the rainbow filter to each pixel row
    applyRainbowFilter(image, 32); // Alpha value of 128 for transparency

    //CALL LODEPNG WRITE FUNC

}

void PNGHelper::SavePNG(const Image& image, const char* outputPath)
{
    std::vector<unsigned char> buffer;
    unsigned int error = lodepng::encode(buffer, image.getPixels(), image.getWidth(), image.getHeight());

    if(error)
    {
	std::cout << "[LODEPNG] Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	return;
    }

    std::string fullPath = "datasets/images/";
    fullPath += outputPath;

    lodepng::save_file(buffer, fullPath.c_str());
}

void PNGHelper::LoadPNG(Image& image, const char* inputPath)
{
    std::vector<unsigned char> buffer, lodeImage;
    lodepng::load_file(buffer, inputPath);
    unsigned int width, height;
    unsigned int error = lodepng::decode(lodeImage, width, height, buffer);

    if(error)
    {
	std::cout << "[LODEPNG] Decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	return;
    }


    LoadPNG(image,  &lodeImage[0], width, height);
}

void PNGHelper::LoadPNG(Image& image, const unsigned char* rgba, unsigned int w, unsigned int h)
{
	image.Allocate(w, h);

	// Copy image data to the Image class
	for (unsigned y = 0; y < h; ++y)
	{
		for (unsigned x = 0; x < w; ++x)
		{
			const int channels = 4;
			const unsigned char* pixel = &(rgba[(y * w + x) * channels]);

			shmea::RGBA rgba(pixel[0], pixel[1], pixel[2], pixel[3]);
			image.SetPixel(x, y, rgba);
		}
	}
}




//Compile command for Linux:
//g++ lodepng.cpp example_sdl.cpp -lSDL2 -O3 -o showpng

/*
LodePNG SDL example
This example displays a PNG with a checkerboard pattern to show tranparency.
It requires the SDL2 library to compile and run.
If multiple filenames are given to the command line, it shows all of them.
Press any key to see next image, or esc to quit.
*/


/*int show(const std::string& caption, const unsigned char* rgba, unsigned w, unsigned h)
{
  //avoid too large window size by downscaling large image
  unsigned jump = 1;
  if(w / 1024 >= jump) jump = w / 1024 + 1;
  if(h / 1024 >= jump) jump = h / 1024 + 1;

  size_t screenw = w / jump;
  size_t screenh = h / jump;
  size_t pitch = screenw * sizeof(Uint32);
  //init SDL
  SDL_Window* sdl_window;
  SDL_Renderer* sdl_renderer;
  SDL_CreateWindowAndRenderer(screenw, screenh, SDL_WINDOW_OPENGL, &sdl_window, &sdl_renderer);
  SDL_SetWindowTitle(sdl_window, caption.c_str());
  if(!sdl_window)
  {
    std::cout << "error, no SDL screen" << std::endl;
    return 0;
  }
  SDL_Texture* sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888,
                                               SDL_TEXTUREACCESS_STREAMING, screenw, screenh);
  std::vector<Uint32> sdl_pixels(screenw * screenh * sizeof(Uint32));

  //plot the pixels of the PNG file
  for(unsigned y = 0; y + jump - 1 < h; y += jump)
  for(unsigned x = 0; x + jump - 1 < w; x += jump)
  {
    //get RGBA components
    Uint32 r = rgba[4 * y * w + 4 * x + 0]; //red
    Uint32 g = rgba[4 * y * w + 4 * x + 1]; //green
    Uint32 b = rgba[4 * y * w + 4 * x + 2]; //blue
    Uint32 a = rgba[4 * y * w + 4 * x + 3]; //alpha

    //make translucency visible by placing checkerboard pattern behind image
    int checkerColor = 191 + 64 * (((x / 16) % 2) == ((y / 16) % 2));
    r = (a * r + (255 - a) * checkerColor) / 255;
    g = (a * g + (255 - a) * checkerColor) / 255;
    b = (a * b + (255 - a) * checkerColor) / 255;

    //give the color value to the pixel of the screenbuffer
    sdl_pixels[(y * screenw + x) / jump] = 65536 * r + 256 * g + b;
  }

  // render the pixels to the screen
  SDL_UpdateTexture(sdl_texture, NULL, sdl_pixels.data(), pitch);
  SDL_RenderClear(sdl_renderer);
  SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
  SDL_RenderPresent(sdl_renderer);

  //pause until you press escape and meanwhile redraw screen
  SDL_Event event;
  int done = 0;
  while(done == 0)
  {
    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_QUIT) done = 2;
      else if(SDL_GetKeyboardState(NULL)[SDLK_ESCAPE]) done = 2;
      else if(event.type == SDL_KEYDOWN) done = 1; //press any other key for next image
    }
    SDL_Delay(5); //pause 5 ms so it consumes less processing power
  }

  SDL_Quit();
  return done == 2 ? 1 : 0;
}

//shows image with SDL. Returns 1 if user wants to fully quit, 0 if user wants to see next image.
int showfile(const char* filename)
{
  std::cout << "showing " << filename << std::endl;

  std::vector<unsigned char> buffer, image;
  lodepng::load_file(buffer, filename); //load the image file with given filename
  unsigned w, h;
  unsigned error = lodepng::decode(image, w, h, buffer); //decode the png

  //stop if there is an error
  if(error)
  {
    std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    return 0;
  }

  return show(filename, &image[0], w, h);
}

void lodeTest(std::vector<std::string> > fNames)
{
  for(unsigned int i = 1; i < fNames.size(); ++i)
  {
    if(showfile(fNames[i]))
	return;
  }
}*/
