#include "trFunctions.hpp"
#include "trFileException.hpp"

#include "../EasyBMP/EasyBMP.h"

tr::ColorBuffer tr::LoadTexture(const std::string& filename)
{
	BMP* image   = new BMP();
	bool success = image->ReadFromFile(filename.c_str());

	if (success)
	{
		tr::ColorBuffer colorBuffer(image->TellWidth(), image->TellHeight());

		for (int x = 0; x < image->TellWidth(); ++x)
		{
			for (int y = 0; y < image->TellHeight(); ++y)
			{
				colorBuffer.At(x, y) =  ((tr::Color) image->GetPixel(x, y).Red)   << 16;
				colorBuffer.At(x, y) += ((tr::Color) image->GetPixel(x, y).Green) << 8;
				colorBuffer.At(x, y) += ((tr::Color) image->GetPixel(x, y).Blue)  << 0;
			}
		}

		delete image;

		return colorBuffer;
	}
	else
	{
		delete image;

		throw tr::FileException("Failed to load texture " + filename);
	}
}
