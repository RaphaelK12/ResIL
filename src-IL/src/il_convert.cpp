//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_convert.c
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_manip.h"
#include <limits.h>
//#include "IL/il.h"


ILimage *iConvertPalette(ILimage *Image, ILenum DestFormat)
{
	static const ILfloat LumFactor[3] = { 0.212671f, 0.715160f, 0.072169f };  // http://www.inforamp.net/~poynton/ and libpng's libpng.txt - Used for conversion to luminance.
	ILimage		*NewImage = NULL, *CurImage = NULL;
	ILuint		i, j, k, c, Size, LumBpp = 1;
	ILfloat		Resultf;
	ILubyte		*Temp = NULL;
	ILboolean	Converted;
	ILboolean	HasAlpha;

	NewImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
	if (NewImage == NULL) {
		return IL_FALSE;
	}

	il2CopyImageAttr(NewImage, Image);

	if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE || Image->Bpp != 1) {
		ilCloseImage(NewImage);
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	if (DestFormat == IL_LUMINANCE || DestFormat == IL_LUMINANCE_ALPHA) {
		if (NewImage->Pal.Palette)
			ifree(NewImage->Pal.Palette);
		if (DestFormat == IL_LUMINANCE_ALPHA)
			LumBpp = 2;

		switch (Image->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				Temp = (ILubyte*)ialloc(LumBpp * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
					Resultf = 0.0f;
					for (c = 0; c < Size; c++) {
						Resultf += Image->Pal.Palette[i + c] * LumFactor[c];
					}
					Temp[k] = (ILubyte)Resultf;
					if (LumBpp == 2) {
						if (Image->Pal.PalType == IL_PAL_RGBA32)
							Temp[k+1] = Image->Pal.Palette[i + 3];
						else
							Temp[k+1] = 0xff;
					}
				}

				break;

			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				Temp = (ILubyte*)ialloc(LumBpp * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
					Resultf = 0.0f;  j = 2;
					for (c = 0; c < Size; c++, j--) {
						Resultf += Image->Pal.Palette[i + c] * LumFactor[j];
					}
					Temp[k] = (ILubyte)Resultf;
					if (LumBpp == 2) {
						if (Image->Pal.PalType == IL_PAL_BGRA32)
							Temp[k+1] = Image->Pal.Palette[i + 3];
						else
							Temp[k+1] = 0xff;
					}
				}

				break;
		}

		NewImage->Pal.Palette = NULL;
		NewImage->Pal.PalSize = 0;
		NewImage->Pal.PalType = IL_PAL_NONE;
		NewImage->Format = DestFormat;
		NewImage->Bpp = LumBpp;
		NewImage->Bps = NewImage->Width * LumBpp;
		NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->Data = (ILubyte*)ialloc(NewImage->SizeOfData);
		if (NewImage->Data == NULL)
			goto alloc_error;

		if (LumBpp == 2) {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
				NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
			}
		}
		else {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i] = Temp[Image->Data[i]];
			}
		}

		ifree(Temp);

		return NewImage;
	}
	else if (DestFormat == IL_ALPHA) {
		if (NewImage->Pal.Palette)
			ifree(NewImage->Pal.Palette);

		switch (Image->Pal.PalType)
		{
			// Opaque, so all the values are 0xFF.
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
				HasAlpha = IL_FALSE;
				break;

			case IL_PAL_BGRA32:
			case IL_PAL_RGBA32:
				HasAlpha = IL_TRUE;
				Temp = (ILubyte*)ialloc(1 * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += 1) {
					Temp[k] = Image->Pal.Palette[i + 3];
				}

				break;
		}

		NewImage->Pal.Palette = NULL;
		NewImage->Pal.PalSize = 0;
		NewImage->Pal.PalType = IL_PAL_NONE;
		NewImage->Format = DestFormat;
		NewImage->Bpp = LumBpp;
		NewImage->Bps = NewImage->Width * 1;  // Alpha is only one byte.
		NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->Data = (ILubyte*)ialloc(NewImage->SizeOfData);
		if (NewImage->Data == NULL)
			goto alloc_error;

		if (HasAlpha) {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
				NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
			}
		}
		else {  // No alpha, opaque.
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i] = 0xFF;
			}
		}

		ifree(Temp);

		return NewImage;
	}

	//NewImage->Format = ilGetPalBaseType(Image->Pal.PalType);
	NewImage->Format = DestFormat;

	if (ilGetBppFormat(NewImage->Format) == 0) {
		ilCloseImage(NewImage);
		il2SetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	CurImage = Image;
	//ilSetCurImage(NewImage);

	switch (DestFormat)
	{
		case IL_RGB:
			Converted = il2ConvertPal(NewImage, IL_PAL_RGB24);
			break;

		case IL_BGR:
			Converted = il2ConvertPal(NewImage, IL_PAL_BGR24);
			break;

		case IL_RGBA:
			Converted = il2ConvertPal(NewImage, IL_PAL_RGB32);
			break;

		case IL_BGRA:
			Converted = il2ConvertPal(NewImage, IL_PAL_BGR32);
			break;

		case IL_COLOUR_INDEX:
			// Just copy the original image over.
			NewImage->Data = (ILubyte*)ialloc(CurImage->SizeOfData);
			if (NewImage->Data == NULL)
				goto alloc_error;
			NewImage->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
			if (NewImage->Pal.Palette == NULL)
				goto alloc_error;
			memcpy(NewImage->Data, CurImage->Data, CurImage->SizeOfData);
			memcpy(NewImage->Pal.Palette, Image->Pal.Palette, Image->Pal.PalSize);
			NewImage->Pal.PalSize = Image->Pal.PalSize;
			NewImage->Pal.PalType = Image->Pal.PalType;
			//ilSetCurImage(CurImage);
			return NewImage;

		default:
			ilCloseImage(NewImage);
			il2SetError(IL_INVALID_CONVERSION);
			return NULL;
	}

	// Resize to new bpp
	il2ResizeImage(NewImage, NewImage->Width, NewImage->Height, NewImage->Depth, ilGetBppFormat(DestFormat), /*ilGetBpcType(DestType)*/1);

	// il2ConvertPal already sets the error message - no need to confuse the user.
	if (!Converted) {
		//ilSetCurImage(CurImage);
		ilCloseImage(NewImage);
		return NULL;
	}

	Size = ilGetBppPal(NewImage->Pal.PalType);
	for (i = 0; i < Image->SizeOfData; i++) {
		for (c = 0; c < NewImage->Bpp; c++) {
			NewImage->Data[i * NewImage->Bpp + c] = NewImage->Pal.Palette[Image->Data[i] * Size + c];
		}
	}

	ifree(NewImage->Pal.Palette);

	NewImage->Pal.Palette = NULL;
	NewImage->Pal.PalSize = 0;
	NewImage->Pal.PalType = IL_PAL_NONE;
	//ilSetCurImage(CurImage);

	return NewImage;

alloc_error:
	ifree(Temp);
	if (NewImage)
		ilCloseImage(NewImage);
	return NULL;
}


// In il_quantizer.c
ILimage *iQuantizeImage(ILimage *Image, ILuint NumCols);
// In il_neuquant.c
ILimage *iNeuQuant(ILimage *Image, ILuint NumCols);

// Converts an image from one format to another
ILAPI ILimage* ILAPIENTRY iConvertImage(ILimage *Image, ILenum DestFormat, ILenum DestType)
{
	ILimage	*NewImage, *CurImage;
	ILuint	i;
	ILubyte	*NewData;

	CurImage = Image;
	if (Image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// We don't support 16-bit color indices (or higher).
	if (DestFormat == IL_COLOUR_INDEX && DestType >= IL_SHORT) {
		il2SetError(IL_INVALID_CONVERSION);
		return NULL;
	}

	if (Image->Format == IL_COLOUR_INDEX) {
		NewImage = iConvertPalette(Image, DestFormat);

		//added test 2003-09-01
		if (NewImage == NULL)
			return NULL;

		if (DestType == NewImage->Type)
			return NewImage;

		NewData = (ILubyte*)ilConvertBuffer(NewImage->SizeOfData, NewImage->Format, DestFormat, NewImage->Type, DestType, NULL, NewImage->Data);
		if (NewData == NULL) {
			ifree(NewImage);  // ilCloseImage not needed.
			return NULL;
		}
		ifree(NewImage->Data);
		NewImage->Data = NewData;

		il2CopyImageAttr(NewImage, Image);
		NewImage->Format = DestFormat;
		NewImage->Type = DestType;
		NewImage->Bpc = ilGetBpcType(DestType);
		NewImage->Bpp = ilGetBppFormat(DestFormat);
		NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
		NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;
	}
	else if (DestFormat == IL_COLOUR_INDEX && Image->Format != IL_LUMINANCE) {
		if (iGetInt(IL_QUANTIZATION_MODE) == IL_NEU_QUANT)
			return iNeuQuant(Image, iGetInt(IL_MAX_QUANT_INDICES));
		else // Assume IL_WU_QUANT otherwise.
			return iQuantizeImage(Image, iGetInt(IL_MAX_QUANT_INDICES));
	}
	else {
		NewImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
		if (NewImage == NULL) {
			return NULL;
		}

		if (ilGetBppFormat(DestFormat) == 0) {
			il2SetError(IL_INVALID_PARAM);
			ifree(NewImage);
			return NULL;
		}

		il2CopyImageAttr(NewImage, Image);
		NewImage->Format = DestFormat;
		NewImage->Type = DestType;
		NewImage->Bpc = ilGetBpcType(DestType);
		NewImage->Bpp = ilGetBppFormat(DestFormat);
		NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
		NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;

		if (DestFormat == IL_COLOUR_INDEX && Image->Format == IL_LUMINANCE) {
			NewImage->Pal.PalSize = 768;
			NewImage->Pal.PalType = IL_PAL_RGB24;
			NewImage->Pal.Palette = (ILubyte*)ialloc(768);
			for (i = 0; i < 256; i++) {
				NewImage->Pal.Palette[i * 3] = i;
				NewImage->Pal.Palette[i * 3 + 1] = i;
				NewImage->Pal.Palette[i * 3 + 2] = i;
			}
			NewImage->Data = (ILubyte*)ialloc(Image->SizeOfData);
			if (NewImage->Data == NULL) {
				ilCloseImage(NewImage);
				return NULL;
			}
			memcpy(NewImage->Data, Image->Data, Image->SizeOfData);
		}
		else {
			NewImage->Data = (ILubyte*)ilConvertBuffer(Image->SizeOfData, Image->Format, DestFormat, Image->Type, DestType, NULL, Image->Data);
			if (NewImage->Data == NULL) {
				ifree(NewImage);  // ilCloseImage not needed.
				return NULL;
			}
		}
	}

	return NewImage;
}


//! Converts the current image to the DestFormat format.
/*! \param DestFormat An enum of the desired output format.  Any format values are accepted.
    \param DestType An enum of the desired output type.  Any type values are accepted.
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\exception IL_INVALID_CONVERSION DestFormat or DestType was an invalid identifier.
	\exception IL_OUT_OF_MEMORY Could not allocate enough memory.
	\return Boolean value of failure or success*/
ILboolean ILAPIENTRY il2ConvertImage(ILimage* image, ILenum DestFormat, ILenum DestType)
{
	ILimage *Image, *pCurImage;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (DestFormat == image->Format && DestType == image->Type)
		return IL_TRUE;  // No conversion needed.

	if (DestType == image->Type) {
		if (iFastConvert(image, DestFormat)) {
			image->Format = DestFormat;
			return IL_TRUE;
		}
	}

	if (il2IsEnabled(IL_USE_KEY_COLOUR)) {
		ilAddAlphaKey(image);
	}

	pCurImage = image;
	while (pCurImage != NULL)
	{
		Image = iConvertImage(pCurImage, DestFormat, DestType);
		if (Image == NULL)
			return IL_FALSE;

		//ilCopyImageAttr(pCurImage, Image);  // Destroys subimages.

		// We don't copy the colour profile here, since it stays the same.
		//	Same with the DXTC data.
		pCurImage->Format = DestFormat;
		pCurImage->Type = DestType;
		pCurImage->Bpc = ilGetBpcType(DestType);
		pCurImage->Bpp = ilGetBppFormat(DestFormat);
		pCurImage->Bps = pCurImage->Width * pCurImage->Bpc * pCurImage->Bpp;
		pCurImage->SizeOfPlane = pCurImage->Bps * pCurImage->Height;
		pCurImage->SizeOfData = pCurImage->Depth * pCurImage->SizeOfPlane;
		if (pCurImage->Pal.Palette && pCurImage->Pal.PalSize && pCurImage->Pal.PalType != IL_PAL_NONE)
			ifree(pCurImage->Pal.Palette);
		pCurImage->Pal.Palette = Image->Pal.Palette;
		pCurImage->Pal.PalSize = Image->Pal.PalSize;
		pCurImage->Pal.PalType = Image->Pal.PalType;
		Image->Pal.Palette = NULL;
		ifree(pCurImage->Data);
		pCurImage->Data = Image->Data;
		Image->Data = NULL;
		ilCloseImage(Image);

		pCurImage = pCurImage->Next;
	}

	return IL_TRUE;
}


// Swaps the colour order of the current image (rgb(a)->bgr(a) or vice-versa).
//	Must be either an 8, 24 or 32-bit (coloured) image (or palette).
ILboolean il2SwapColours(ILimage* image)
{
	ILuint		i = 0, Size = image->Bpp * image->Width * image->Height;
	ILbyte		PalBpp = ilGetBppPal(image->Pal.PalType);
	ILushort	*ShortPtr;
	ILuint		*IntPtr, Temp;
	ILdouble	*DoublePtr, DoubleTemp;

	if ((image->Bpp != 1 && image->Bpp != 3 && image->Bpp != 4)) {
		il2SetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	// Just check before we change the format.
	if (image->Format == IL_COLOUR_INDEX) {
		if (PalBpp == 0 || image->Format != IL_COLOUR_INDEX) {
			il2SetError(IL_ILLEGAL_OPERATION);
			return IL_FALSE;
		}
	}

	switch (image->Format)
	{
		case IL_RGB:
			image->Format = IL_BGR;
			break;
		case IL_RGBA:
			image->Format = IL_BGRA;
			break;
		case IL_BGR:
			image->Format = IL_RGB;
			break;
		case IL_BGRA:
			image->Format = IL_RGBA;
			break;
		case IL_ALPHA:
		case IL_LUMINANCE:
		case IL_LUMINANCE_ALPHA:
			return IL_TRUE;  // No need to do anything to luminance or alpha images.
		case IL_COLOUR_INDEX:
			switch (image->Pal.PalType)
			{
				case IL_PAL_RGB24:
					image->Pal.PalType = IL_PAL_BGR24;
					break;
				case IL_PAL_RGB32:
					image->Pal.PalType = IL_PAL_BGR32;
					break;
				case IL_PAL_RGBA32:
					image->Pal.PalType = IL_PAL_BGRA32;
					break;
				case IL_PAL_BGR24:
					image->Pal.PalType = IL_PAL_RGB24;
					break;
				case IL_PAL_BGR32:
					image->Pal.PalType = IL_PAL_RGB32;
					break;
				case IL_PAL_BGRA32:
					image->Pal.PalType = IL_PAL_RGBA32;
					break;
				default:
					il2SetError(IL_ILLEGAL_OPERATION);
					return IL_FALSE;
			}
			break;
		default:
			il2SetError(IL_ILLEGAL_OPERATION);
			return IL_FALSE;
	}

	if (image->Format == IL_COLOUR_INDEX) {
		for (; i < image->Pal.PalSize; i += PalBpp) {
				Temp = image->Pal.Palette[i];
				image->Pal.Palette[i] = image->Pal.Palette[i+2];
				image->Pal.Palette[i+2] = Temp;
		}
	}
	else {
		ShortPtr = (ILushort*)image->Data;
		IntPtr = (ILuint*)image->Data;
		DoublePtr = (ILdouble*)image->Data;
		switch (image->Bpc)
		{
			case 1:
				for (; i < Size; i += image->Bpp) {
					Temp = image->Data[i];
					image->Data[i] = image->Data[i+2];
					image->Data[i+2] = Temp;
				}
				break;
			case 2:
				for (; i < Size; i += image->Bpp) {
					Temp = ShortPtr[i];
					ShortPtr[i] = ShortPtr[i+2];
					ShortPtr[i+2] = Temp;
				}
				break;
			case 4:  // Works fine with ILint, ILuint and ILfloat.
				for (; i < Size; i += image->Bpp) {
					Temp = IntPtr[i];
					IntPtr[i] = IntPtr[i+2];
					IntPtr[i+2] = Temp;
				}
				break;
			case 8:
				for (; i < Size; i += image->Bpp) {
					DoubleTemp = DoublePtr[i];
					DoublePtr[i] = DoublePtr[i+2];
					DoublePtr[i+2] = DoubleTemp;
				}
				break;
		}
	}

	return IL_TRUE;
}


// Adds an opaque alpha channel to a 24-bit image
ILboolean ilAddAlpha(ILimage* image)
{
	ILubyte		*NewData, NewBpp;
	ILuint		i = 0, j = 0, Size;

	if (image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (image->Bpp != 3) {
		il2SetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	Size = image->Bps * image->Height / image->Bpc;
	NewBpp = (ILubyte)(image->Bpp + 1);
	
	NewData = (ILubyte*)ialloc(NewBpp * image->Bpc * image->Width * image->Height);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	switch (image->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				NewData[j]   = image->Data[i];
				NewData[j+1] = image->Data[i+1];
				NewData[j+2] = image->Data[i+2];
				NewData[j+3] = UCHAR_MAX;  // Max opaqueness
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILushort*)NewData)[j]   = ((ILushort*)image->Data)[i];
				((ILushort*)NewData)[j+1] = ((ILushort*)image->Data)[i+1];
				((ILushort*)NewData)[j+2] = ((ILushort*)image->Data)[i+2];
				((ILushort*)NewData)[j+3] = USHRT_MAX;
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILuint*)NewData)[j]   = ((ILuint*)image->Data)[i];
				((ILuint*)NewData)[j+1] = ((ILuint*)image->Data)[i+1];
				((ILuint*)NewData)[j+2] = ((ILuint*)image->Data)[i+2];
				((ILuint*)NewData)[j+3] = UINT_MAX;
			}
			break;

		case IL_FLOAT:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILfloat*)NewData)[j]   = ((ILfloat*)image->Data)[i];
				((ILfloat*)NewData)[j+1] = ((ILfloat*)image->Data)[i+1];
				((ILfloat*)NewData)[j+2] = ((ILfloat*)image->Data)[i+2];
				((ILfloat*)NewData)[j+3] = 1.0f;
			}
			break;

		case IL_DOUBLE:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILdouble*)NewData)[j]   = ((ILdouble*)image->Data)[i];
				((ILdouble*)NewData)[j+1] = ((ILdouble*)image->Data)[i+1];
				((ILdouble*)NewData)[j+2] = ((ILdouble*)image->Data)[i+2];
				((ILdouble*)NewData)[j+3] = 1.0;
			}
			break;

		default:
			ifree(NewData);
			il2SetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}


	image->Bpp = NewBpp;
	image->Bps = image->Width * image->Bpc * NewBpp;
	image->SizeOfPlane = image->Bps * image->Height;
	image->SizeOfData = image->SizeOfPlane * image->Depth;
	ifree(image->Data);
	image->Data = NewData;

	switch (image->Format)
	{
		case IL_RGB:
			image->Format = IL_RGBA;
			break;
		case IL_BGR:
			image->Format = IL_BGRA;
			break;
	}

	return IL_TRUE;
}


//ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;

void ILAPIENTRY ilKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha)
{
	ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;
        KeyRed		= Red;
	KeyGreen	= Green;
	KeyBlue		= Blue;
	KeyAlpha	= Alpha;
	return;
}


// Adds an alpha channel to an 8 or 24-bit image,
//	making the image transparent where Key is equal to the pixel.
ILboolean ilAddAlphaKey(ILimage *Image)
{
	ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;
        ILubyte		*NewData, NewBpp;
	ILfloat		KeyColour[3];
	ILuint		i = 0, j = 0, c, Size;
	ILboolean	Same;

	if (Image == NULL) {
		il2SetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->Format != IL_COLOUR_INDEX) {
		if (Image->Bpp != 3) {
			il2SetError(IL_INVALID_VALUE);
			return IL_FALSE;
		}

		if (Image->Format == IL_BGR || Image->Format == IL_BGRA) {
			KeyColour[0] = KeyBlue;
			KeyColour[1] = KeyGreen;
			KeyColour[2] = KeyRed;
		}
		else {
			KeyColour[0] = KeyRed;
			KeyColour[1] = KeyGreen;
			KeyColour[2] = KeyBlue;
		}

		Size = Image->Bps * Image->Height / Image->Bpc;

		NewBpp = (ILubyte)(Image->Bpp + 1);
		
		NewData = (ILubyte*)ialloc(NewBpp * Image->Bpc * Image->Width * Image->Height);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		switch (Image->Type)
		{
			case IL_BYTE:
			case IL_UNSIGNED_BYTE:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					NewData[j]   = Image->Data[i];
					NewData[j+1] = Image->Data[i+1];
					NewData[j+2] = Image->Data[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (Image->Data[i+c] != KeyColour[c] * UCHAR_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						NewData[j+3] = 0;  // Transparent - matches key colour
					else
						NewData[j+3] = UCHAR_MAX;
				}
				break;

			case IL_SHORT:
			case IL_UNSIGNED_SHORT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILushort*)NewData)[j]   = ((ILushort*)Image->Data)[i];
					((ILushort*)NewData)[j+1] = ((ILushort*)Image->Data)[i+1];
					((ILushort*)NewData)[j+2] = ((ILushort*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILushort*)Image->Data)[i+c] != KeyColour[c] * USHRT_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						((ILushort*)NewData)[j+3] = 0;
					else
						((ILushort*)NewData)[j+3] = USHRT_MAX;
				}
				break;

			case IL_INT:
			case IL_UNSIGNED_INT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILuint*)NewData)[j]   = ((ILuint*)Image->Data)[i];
					((ILuint*)NewData)[j+1] = ((ILuint*)Image->Data)[i+1];
					((ILuint*)NewData)[j+2] = ((ILuint*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILuint*)Image->Data)[i+c] != KeyColour[c] * UINT_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						((ILuint*)NewData)[j+3] = 0;
					else
						((ILuint*)NewData)[j+3] = UINT_MAX;
				}
				break;

			case IL_FLOAT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILfloat*)NewData)[j]   = ((ILfloat*)Image->Data)[i];
					((ILfloat*)NewData)[j+1] = ((ILfloat*)Image->Data)[i+1];
					((ILfloat*)NewData)[j+2] = ((ILfloat*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILfloat*)Image->Data)[i+c] != KeyColour[c])
							Same = IL_FALSE;
					}

					if (Same)
						((ILfloat*)NewData)[j+3] = 0.0f;
					else
						((ILfloat*)NewData)[j+3] = 1.0f;
				}
				break;

			case IL_DOUBLE:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILdouble*)NewData)[j]   = ((ILdouble*)Image->Data)[i];
					((ILdouble*)NewData)[j+1] = ((ILdouble*)Image->Data)[i+1];
					((ILdouble*)NewData)[j+2] = ((ILdouble*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILdouble*)Image->Data)[i+c] != KeyColour[c])
							Same = IL_FALSE;
					}

					if (Same)
						((ILdouble*)NewData)[j+3] = 0.0;
					else
						((ILdouble*)NewData)[j+3] = 1.0;
				}
				break;

			default:
				ifree(NewData);
				il2SetError(IL_INTERNAL_ERROR);
				return IL_FALSE;
		}


		Image->Bpp = NewBpp;
		Image->Bps = Image->Width * Image->Bpc * NewBpp;
		Image->SizeOfPlane = Image->Bps * Image->Height;
		Image->SizeOfData = Image->SizeOfPlane * Image->Depth;
		ifree(Image->Data);
		Image->Data = NewData;

		switch (Image->Format)
		{
			case IL_RGB:
				Image->Format = IL_RGBA;
				break;
			case IL_BGR:
				Image->Format = IL_BGRA;
				break;
		}
	}
	else {  // IL_COLOUR_INDEX
		if (Image->Bpp != 1) {
			il2SetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
		}

		Size = il2GetInteger(IL_PALETTE_NUM_COLS);
		if (Size == 0) {
			il2SetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
		}

		if ((ILuint)(KeyAlpha * UCHAR_MAX) > Size) {
			il2SetError(IL_INVALID_VALUE);
			return IL_FALSE;
		}

		switch (Image->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				if (!il2ConvertPal(Image, IL_PAL_RGBA32))
					return IL_FALSE;
				break;
			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				if (!il2ConvertPal(Image, IL_PAL_BGRA32))
					return IL_FALSE;
				break;
			default:
				il2SetError(IL_INTERNAL_ERROR);
				return IL_FALSE;
		}

		// Set the colour index to be transparent.
		Image->Pal.Palette[(ILuint)(KeyAlpha * UCHAR_MAX) * 4 + 3] = 0;

		// @TODO: Check if this is the required behaviour.

		if (Image->Pal.PalType == IL_PAL_RGBA32)
			il2ConvertImage(Image, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			il2ConvertImage(Image, IL_BGRA, IL_UNSIGNED_BYTE);
	}

	return IL_TRUE;
}


// Removes alpha from a 32-bit image
//	Should we maybe add an option that changes the image based on the alpha?
ILboolean il2RemoveAlpha(ILimage* image)
{
        ILubyte *NewData, NewBpp;
	ILuint i = 0, j = 0, Size;

	if (image == NULL) {
		il2SetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (image->Bpp != 4) {
		il2SetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	Size = image->Bps * image->Height;
	NewBpp = (ILubyte)(image->Bpp - 1);
	
	NewData = (ILubyte*)ialloc(NewBpp * image->Bpc * image->Width * image->Height);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	switch (image->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				NewData[j]   = image->Data[i];
				NewData[j+1] = image->Data[i+1];
				NewData[j+2] = image->Data[i+2];
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILushort*)NewData)[j]   = ((ILushort*)image->Data)[i];
				((ILushort*)NewData)[j+1] = ((ILushort*)image->Data)[i+1];
				((ILushort*)NewData)[j+2] = ((ILushort*)image->Data)[i+2];
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILuint*)NewData)[j]   = ((ILuint*)image->Data)[i];
				((ILuint*)NewData)[j+1] = ((ILuint*)image->Data)[i+1];
				((ILuint*)NewData)[j+2] = ((ILuint*)image->Data)[i+2];
			}
			break;

		case IL_FLOAT:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILfloat*)NewData)[j]   = ((ILfloat*)image->Data)[i];
				((ILfloat*)NewData)[j+1] = ((ILfloat*)image->Data)[i+1];
				((ILfloat*)NewData)[j+2] = ((ILfloat*)image->Data)[i+2];
			}
			break;

		case IL_DOUBLE:
			for (; i < Size; i += image->Bpp, j += NewBpp) {
				((ILdouble*)NewData)[j]   = ((ILdouble*)image->Data)[i];
				((ILdouble*)NewData)[j+1] = ((ILdouble*)image->Data)[i+1];
				((ILdouble*)NewData)[j+2] = ((ILdouble*)image->Data)[i+2];
			}
			break;

		default:
			ifree(NewData);
			il2SetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}


	image->Bpp = NewBpp;
	image->Bps = image->Width * image->Bpc * NewBpp;
	image->SizeOfPlane = image->Bps * image->Height;
	image->SizeOfData = image->SizeOfPlane * image->Depth;
	ifree(image->Data);
	image->Data = NewData;

	switch (image->Format)
	{
		case IL_RGBA:
			image->Format = IL_RGB;
			break;
		case IL_BGRA:
			image->Format = IL_BGR;
			break;
	}

	return IL_TRUE;
}


ILboolean il2FixCur(ILimage* image)
{
	if (il2IsEnabled(IL_ORIGIN_SET)) {
		if ((ILenum)il2GetInteger(IL_ORIGIN_MODE) != image->Origin) {
			if (!ilFlipImage(image)) {
				return IL_FALSE;
			}
		}
	}

	if (il2IsEnabled(IL_TYPE_SET)) {
		if ((ILenum)il2GetInteger(IL_TYPE_MODE) != image->Type) {
			if (!il2ConvertImage(image, image->Format, il2GetInteger(IL_TYPE_MODE))) {
				return IL_FALSE;
			}
		}
	}
	if (il2IsEnabled(IL_FORMAT_SET)) {
		if ((ILenum)il2GetInteger(IL_FORMAT_MODE) != image->Format) {
			if (!il2ConvertImage(image, il2GetInteger(IL_FORMAT_MODE), image->Type)) {
				return IL_FALSE;
			}
		}
	}

	if (image->Format == IL_COLOUR_INDEX) {
		if (il2GetBoolean(IL_CONV_PAL) == IL_TRUE) {
			if (!il2ConvertImage(image, IL_BGR, IL_UNSIGNED_BYTE)) {
				return IL_FALSE;
			}
		}
	}
/*	Swap Colors on Big Endian !!!!!
#ifdef __BIG_ENDIAN__
	// Swap endian
	EndianSwapData(image);
#endif 
*/
	return IL_TRUE;
}

/*
ILboolean ilFixImage()
{
	ILuint NumImages, i;

	NumImages = ilGetInteger(IL_NUM_IMAGES);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName());  // Set to parent image first.
		if (!ilActiveImage(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	NumImages = ilGetInteger(IL_NUM_MIPMAPS);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName());  // Set to parent image first.
		if (!ilActiveMipmap(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	NumImages = ilGetInteger(IL_NUM_LAYERS);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName());  // Set to parent image first.
		if (!ilActiveLayer(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	ilBindImage(ilGetCurName());
	ilFixCur();

	return IL_TRUE;
}
*/

// Fix all frames, faces, layers and mipmaps of an image
/*
This function was replaced 20050304, because the previous version
didn't fix the mipmaps of the subimages etc. This version is not
completely correct either, because the subimages of the subimages
etc. are not fixed, but at the moment no images of this type can
be loaded anyway. Thanks to Chris Lux for pointing this out.
*/
ILboolean il2FixImage(ILimage* image)
{
	ILint numImages = 0;
	il2GetImageInteger(image, IL_NUM_IMAGES, &numImages);
	for (ILint i = 0; i <= numImages; i++) {
		ILimage* frame = il2GetFrame(image, i);
		ILint numFaces = 0;
		il2GetImageInteger(frame, IL_NUM_FACES, &numFaces);

		for (ILint f = 0; f <= numFaces; f++) {
			ILimage* face = il2GetFace(frame, f);
			ILint numLayers = 0;
			il2GetImageInteger(face, IL_NUM_LAYERS, &numLayers);

			for (ILint k = 0; k <= numLayers; k++) {
				ILimage* layer = il2GetLayer(face, k);
				ILint numMipmaps = 0;
				il2GetImageInteger(layer, IL_NUM_MIPMAPS, &numMipmaps);

				for (ILint j = 0; j <= numMipmaps; j++) {

					ILimage* mipmap = il2GetMipmap(layer, j);

					if (!il2FixCur(mipmap))
						return IL_FALSE;
				}
			}
		}
	}

	return IL_TRUE;
}