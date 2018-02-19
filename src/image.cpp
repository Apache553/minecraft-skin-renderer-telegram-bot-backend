
#include <png.h>

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <memory>

struct ImageData {
	unsigned char *data;
	unsigned int width;
	unsigned int height;
	unsigned int bytePerPixel;
};

ImageData GetImageDataFromPNG(std::string filename, unsigned int signatureLength, bool flip = false) {
	std::FILE *filePtr = std::fopen(filename.c_str(), "rb");
	if (filePtr == nullptr) {
		std::cerr << "ERROR: Unable to open input file \'" << filename << '\'' << std::endl;
		exit(-1);
	}
	std::unique_ptr<unsigned char[]> signature(new unsigned char[signatureLength]);
	if (std::fread(signature.get(), 1, signatureLength, filePtr) < signatureLength) {
		std::cerr << "ERROR: Input file is not a valid png file!(Signature not "
			"long enough)"
			<< std::endl;
		exit(-1);
	}
	if (png_sig_cmp(signature.get(), 0, signatureLength)) {
		std::cerr << "ERROR: Input file is not a valid png file!(Signature not "
			"matches)"
			<< std::endl;
		exit(-1);
	}
	auto pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pngPtr == nullptr) {
		std::cerr << "ERROR: \'png_create_read_struct\' failed!" << std::endl;
		exit(-1);
	}
	auto pngInfoPtr = png_create_info_struct(pngPtr);
	if (pngInfoPtr == nullptr) {
		std::cerr << "ERROR: \'png_create_info_struct\' failed!" << std::endl;
		png_destroy_read_struct(&pngPtr, nullptr, nullptr);
		exit(-1);
	}

	if (setjmp(png_jmpbuf(pngPtr))) {
		png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
		std::cerr << "ERROR: Unhandled exception!" << std::endl;
		exit(-1);
	}
	png_init_io(pngPtr, filePtr);
	png_set_sig_bytes(pngPtr, signatureLength);
	png_read_info(pngPtr, pngInfoPtr);

	std::cout << "INFO: reading png file \'" << filename << "\'\n";

	unsigned int imageWidth, imageHeight;
	int imageBitDepth, imageColorType, imageInterlaceMethod, imageCompressionMethod, imageFilterMethod;
	png_get_IHDR(pngPtr, pngInfoPtr, &imageWidth, &imageHeight, &imageBitDepth, &imageColorType, &imageInterlaceMethod,
		&imageCompressionMethod,
		&imageFilterMethod);  // get image infomation

	std::cout << "INFO: PNG file is " << imageWidth << " * " << imageHeight << std::endl;

	if (imageColorType == PNG_COLOR_TYPE_RGB || imageColorType == PNG_COLOR_TYPE_RGBA) {
		std::cout << "INFO: PNG file is " << (imageColorType == PNG_COLOR_TYPE_RGB ? "RGB" : "RGBA") << " format."
			<< std::endl;
	}

	if (imageColorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(pngPtr);  // force indexed-color image to rgb image
	}
	if (imageColorType == PNG_COLOR_TYPE_GRAY && imageBitDepth < 8) {
		png_set_expand_gray_1_2_4_to_8(pngPtr);  // force bit depth to 8 for gray124 image
	}
	if (imageBitDepth == 16) {
		png_set_strip_16(pngPtr);  // force bit depth to 16bit
	}
	if (png_get_valid(pngPtr, pngInfoPtr, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(pngPtr);  // transform indexed-color image's
										// transparent color to alpha channel
	}
	if (imageColorType == PNG_COLOR_TYPE_GRAY || imageColorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(pngPtr);  // force gray image to rgb image
	}
	if (imageInterlaceMethod != PNG_INTERLACE_NONE) {
		png_set_interlace_handling(pngPtr);
	}
	if (!(imageColorType & PNG_COLOR_MASK_ALPHA)) {
		png_set_add_alpha(pngPtr, 0x00000000, PNG_FILLER_AFTER);
	}
	png_read_update_info(pngPtr, pngInfoPtr);

	// generate a texture buffer for further usage

	// read & store data
	unsigned char *imageData = new unsigned char[imageWidth * imageHeight * 4];
	unsigned char **imageDataArray = new unsigned char *[imageHeight];
	if (flip) {
		for (size_t i = 0; i < imageHeight; ++i) {
			imageDataArray[i] = &imageData[(imageHeight - 1 - i) * imageWidth * 4];
		}
	}
	else {
		for (size_t i = 0; i < imageHeight; ++i) {
			imageDataArray[i] = &imageData[i * imageWidth * 4];
		}
	}
	png_read_image(pngPtr, imageDataArray);
	/* Debug output */
	// for (size_t i = 0; i < imageHeight; ++i)
	//{
	//	for (size_t j = 0; j < imageWidth; ++j)
	//	{
	//		for (size_t k = 0; k < 4; ++k)
	//		{
	//			std::cout << std::setw(2) << std::setfill('0') <<
	// std::hex
	//<< (static_cast<unsigned int>(imageData[i*imageWidth * 4 + j * 4 + k]) &
	// 0xFF);
	//		}
	//		std::cout << ' ';
	//	}
	//	std::cout << std::endl;
	//}
	/* Debug ouput */
	png_read_end(pngPtr, pngInfoPtr);
	png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
	std::fclose(filePtr);

	delete[] imageDataArray;

	ImageData ret;
	ret.data = imageData;
	ret.width = imageWidth;
	ret.height = imageHeight;
	ret.bytePerPixel = 4;
	return ret;
}

void WriteImageDataToPNG(ImageData &image, const std::string &filename, bool flip = false) {
	FILE *outputPtr = std::fopen(filename.c_str(), "wb");
	if (outputPtr == nullptr) {
		std::cerr << "ERROR: Unable to open output file \'" << filename << "\'" << std::endl;
		exit(-1);
	}

	auto pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pngPtr == nullptr) {
		std::cerr << "ERROR: \'png_create_write_struct\' failed!" << std::endl;
		std::fclose(outputPtr);
		exit(-1);
	}
	auto pngInfoPtr = png_create_info_struct(pngPtr);
	if (pngInfoPtr == nullptr) {
		std::cerr << "ERROR: \'png_create_info_struct\' failed!" << std::endl;
		std::fclose(outputPtr);
		exit(-1);
	}

	if (setjmp(png_jmpbuf(pngPtr))) {
		std::cerr << "ERROR: Unhandled unknown libpng error" << std::endl;
		fclose(outputPtr);
		png_destroy_write_struct(&pngPtr, &pngInfoPtr);
		exit(-1);
	}

	png_init_io(pngPtr, outputPtr);

	png_set_IHDR(pngPtr, pngInfoPtr, image.width, image.height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_color_8 bitSig;
	bitSig.red = 8;
	bitSig.green = 8;
	bitSig.blue = 8;
	png_set_sBIT(pngPtr, pngInfoPtr, &bitSig);

	png_write_info(pngPtr, pngInfoPtr);
	unsigned char **rowPtr = new unsigned char *[image.height];
	if (flip) {
		for (unsigned int rowId = 0; rowId < image.height; ++rowId) {
			rowPtr[rowId] = &image.data[(image.height - 1 - rowId) * image.width * image.bytePerPixel];
		}
	}
	else {
		for (unsigned int rowId = 0; rowId < image.height; ++rowId) {
			rowPtr[rowId] = &image.data[rowId * image.width * image.bytePerPixel];
		}
	}
	png_write_image(pngPtr, rowPtr);
	delete[] rowPtr;
	png_write_end(pngPtr, pngInfoPtr);
	png_destroy_write_struct(&pngPtr, &pngInfoPtr);
	fclose(outputPtr);
}

void CopyPixels(ImageData &image, unsigned int srcX, unsigned int srcY, unsigned int sizeX, unsigned int sizeY,
	unsigned int targetX, unsigned int targetY) {
	for (unsigned int deltaY = 0; deltaY < sizeY; ++deltaY) {
		for (unsigned int deltaX = 0; deltaX < sizeX; ++deltaX) {
			for (unsigned int byteIndex = 0; byteIndex < image.bytePerPixel; ++byteIndex) {
				image.data[(targetY + deltaY) * image.width * image.bytePerPixel +
					(targetX + deltaX) * image.bytePerPixel + byteIndex] =
					image.data[(srcY + deltaY) * image.width * image.bytePerPixel +
					(srcX + deltaX) * image.bytePerPixel + byteIndex];
			}
		}
	}
}

void FlipImageRegionHorizontally(ImageData &image, unsigned int baseX, unsigned int baseY, unsigned sizeX,
	unsigned int sizeY) {
	unsigned char *tmp = new unsigned char[sizeX * sizeY * image.bytePerPixel];
	// read
	for (unsigned int Y = 0; Y < sizeY; ++Y) {
		for (unsigned int X = 0; X < sizeX; ++X) {
			for (unsigned int byteIndex = 0; byteIndex < image.bytePerPixel; ++byteIndex) {
				tmp[Y * sizeX * image.bytePerPixel + (sizeX - 1 - X) * image.bytePerPixel + byteIndex] =
					image.data[(baseY + Y) * image.width * image.bytePerPixel + (baseX + X) * image.bytePerPixel +
					byteIndex];
			}
		}
	}
	// write back
	for (unsigned int Y = 0; Y < sizeY; ++Y) {
		for (unsigned int X = 0; X < sizeX; ++X) {
			for (unsigned int byteIndex = 0; byteIndex < image.bytePerPixel; ++byteIndex) {
				image.data[(baseY + Y) * image.width * image.bytePerPixel + (baseX + X) * image.bytePerPixel +
					byteIndex] = tmp[Y * sizeX * image.bytePerPixel + X * image.bytePerPixel + byteIndex];
			}
		}
	}
	delete[] tmp;
}

void SwapImageRegion(ImageData &image, unsigned int srcX, unsigned int srcY, unsigned int sizeX, unsigned int sizeY,
	unsigned int targetX, unsigned int targetY) {
	unsigned char *tmp = new unsigned char[sizeX * image.bytePerPixel];
	for (unsigned int Y = 0; Y < sizeY; ++Y) {
		std::memcpy(tmp, &image.data[(srcY + Y) * image.width * image.bytePerPixel + srcX * image.bytePerPixel],
			sizeX * image.bytePerPixel);
		std::memcpy(&image.data[(srcY + Y) * image.width * image.bytePerPixel + srcX * image.bytePerPixel],
			&image.data[(targetY + Y) * image.width * image.bytePerPixel + targetX * image.bytePerPixel],
			sizeX * image.bytePerPixel);
		std::memcpy(&image.data[(targetY + Y) * image.width * image.bytePerPixel + targetX * image.bytePerPixel], tmp,
			sizeX * image.bytePerPixel);
	}
	delete[] tmp;
}

void FilpImageVertically(ImageData &image) {
	unsigned char *tmpImage = new unsigned char[image.bytePerPixel * image.height * image.width];
	for (unsigned int Y = 0; Y < image.height; ++Y) {
		std::memcpy(&tmpImage[Y * image.width * image.bytePerPixel],
			&image.data[(image.height - Y - 1) * image.width * image.bytePerPixel],
			image.bytePerPixel * image.width);
	}
	delete[] image.data;
	image.data = tmpImage;
}

ImageData ExtendSkin32x(ImageData &image, bool thinArm) {
	if (image.height != 32 || image.width != 64) {
		std::cerr << "ERROR: Unable to extend skin image to 64x" << std::endl;
		exit(-1);
	}
	ImageData newImage;
	newImage.data = new unsigned char[64 * 64 * 4];
	newImage.height = 64;
	newImage.width = 64;
	newImage.bytePerPixel = 4;

	std::memset(newImage.data, 0, sizeof(unsigned char) * 64 * 64 * 4);
	std::memcpy(newImage.data, image.data, sizeof(unsigned char) * 4 * image.height * image.width);

	CopyPixels(newImage, 0, 16, 16, 16, 16, 48);
	FlipImageRegionHorizontally(newImage, 16, 52, 4, 12);
	FlipImageRegionHorizontally(newImage, 20, 48, 4, 16);
	FlipImageRegionHorizontally(newImage, 24, 48, 4, 16);
	FlipImageRegionHorizontally(newImage, 28, 52, 4, 12);
	SwapImageRegion(newImage, 16, 52, 4, 12, 24, 52);
	if (thinArm) {
		std::cerr << "ERROR: Not a valid skin! Conversion failed!" << std::endl;
		exit(-1);
	}
	else {
		CopyPixels(newImage, 40, 16, 16, 16, 32, 48);
		FlipImageRegionHorizontally(newImage, 32, 52, 4, 12);
		FlipImageRegionHorizontally(newImage, 36, 48, 4, 16);
		FlipImageRegionHorizontally(newImage, 40, 48, 4, 16);
		FlipImageRegionHorizontally(newImage, 44, 52, 4, 12);
		SwapImageRegion(newImage, 32, 52, 4, 12, 40, 52);
	}
	return newImage;
}
