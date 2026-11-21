/******************************************************************************\
# C - picture                                    #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/21 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

#ifndef COBJMACROS
#define COBJMACROS /* IWICBITMAPSOURCE_RELEASE() STYLE COM CALLS IN C */
#endif /* !COBJMACROS */

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define CF_BITMAP
 * #define CF_HDROP
 * #define FAILED(hr)
 * #define GENERIC_READ
 * #define IStream_Release(This)
 * #define MAX_PATH
 * #define PropVariantInit(pvar)
 * #define SUCCEEDED(hr)
 * #define TRUE
 * enum tagCOINIT;
 * enum VARENUM;
 * typedef	HANDLE;
 * typedef	HGLOBAL;
 * typedef	HRESULT;
 * typedef	IStream;
 * typedef	PROPVARIANT;
 * typedef	UINT;
 * typedef	WCHAR;
 * BOOL		CloseClipboard(void);
 * HRESULT	CoInitializeEx(LPVOID, DWORD);
 * void		CoUninitialize(void);
 * HANDLE	GetClipboardData(UINT);
 * LPVOID	GlobalLock(HGLOBAL);
 * SIZE_T	GlobalSize(HGLOBAL);
 * BOOL		GlobalUnlock(HGLOBAL);
 * BOOL		OpenClipboard(HWND);
 * HRESULT	PropVariantClear(PROPVARIANT *);
 * void		Sleep(DWORD);
 */
#include <wincodec.h>
/******* """"""""""""
 * #define IWICBitmap_Release(This)
 * #define IWICBitmapDecoder_GetFrame(This, index, ppIBitmapFrame)
 * #define IWICBitmapDecoder_Release(This)
 * #define IWICBitmapEncoder_Commit(This)
 * #define IWICBitmapEncoder_CreateNewFrame(
 * 	This,
 * 	ppIFrameEncode,
 * 	ppIEncoderOptions
 * )
 * #define IWICBitmapEncoder_Initialize(This, pIStream, cacheOption)
 * #define IWICBitmapEncoder_Release(This)
 * #define IWICBitmapFlipRotator_Initialize(This, pISource, options)
 * #define IWICBitmapFlipRotator_Release(This)
 * #define IWICBitmapFrameDecode_GetMetadataQueryReader(
 * 	This,
 * 	ppIMetadataQueryReader
 * )
 * #define IWICBitmapFrameDecode_Release(This)
 * #define IWICBitmapFrameEncode_Commit(This)
 * #define IWICBitmapFrameEncode_Initialize(This, pIEncoderOptions)
 * #define IWICBitmapFrameEncode_Release(This)
 * #define IWICBitmapFrameEncode_SetSize(This, uiWidth, uiHeight)
 * #define IWICBitmapFrameEncode_WriteSource(This, pIBitmapSource, prc)
 * #define IWICBitmapScaler_Initialize(This, pISource, uiWidth, uiHeight, mode)
 * #define IWICBitmapScaler_Release(This)
 * #define IWICBitmapSource_GetSize(This, puiWidth, puiHeight)
 * #define IWICBitmapSource_Release(This)
 * #define IWICFormatConverter_CopyPixels(
 * 	This,
 * 	prc,
 * 	cbStride,
 * 	cbBufferSize,
 * 	pbBuffer
 * )
 * #define IWICFormatConverter_Initialize(
 * 	This,
 * 	pISource,
 * 	dstFormat,
 * 	dither,
 * 	pIPalette,
 * 	alphaThresholdPercent,
 * 	paletteTranslate
 * )
 * #define IWICFormatConverter_Release(This)
 * #define IWICImagingFactory_CreateBitmapFlipRotator(
 * 	This,
 * 	ppIBitmapFlipRotator
 * )
 * #define IWICImagingFactory_CreateBitmapFromHBITMAP(
 * 	This,
 * 	hBitmap,
 * 	hPalette,
 * 	options,
 * 	ppIBitmap
 * )
 * #define IWICImagingFactory_CreateBitmapFromMemory(
 * 	This,
 * 	uiWidth,
 * 	uiHeight,
 * 	pixelFormat,
 * 	cbStride,
 * 	cbBufferSize,
 * 	pbBuffer,
 * 	ppIBitmap
 * )
 * #define IWICImagingFactory_CreateBitmapScaler(This, ppIBitmapScaler)
 * #define IWICImagingFactory_CreateDecoderFromFilename(
 * 	This,
 * 	wzFilename,
 * 	pguidVendor,
 * 	dwDesiredAccess,
 * 	metadataOptions,
 * 	ppIDecoder
 * )
 * #define IWICImagingFactory_CreateDecoderFromStream(
 * 	This,
 * 	pIStream,
 * 	pguidVendor,
 * 	metadataOptions,
 * 	ppIDecoder
 * )
 * #define IWICImagingFactory_CreateEncoder(
 * 	This,
 * 	guidContainerFormat,
 * 	pguidVendor,
 * 	ppIEncoder
 * )
 * #define IWICImagingFactory_CreateFormatConverter(This, ppIFormatConverter)
 * #define IWICImagingFactory_CreateStream(This, ppIWICStream)
 * #define IWICImagingFactory_Release(This)
 * #define IWICMetadataQueryReader_GetMetadataByName(This, wzName, pvarValue)
 * #define IWICMetadataQueryReader_Release(This)
 * #define IWICStream_InitializeFromMemory(This, pbBuffer, cbBufferSize)
 * #define IWICStream_Release(This)
 * enum WICBitmapTransformOptions;
 * typedef	IWICBitmap;
 * typedef	IWICBitmapDecoder;
 * typedef	IWICBitmapEncoder;
 * typedef	IWICBitmapFlipRotator;
 * typedef	IWICBitmapFrameDecode;
 * typedef	IWICBitmapFrameEncode;
 * typedef	IWICBitmapScaler;
 * typedef	IWICBitmapSource;
 * typedef	IWICFormatConverter;
 * typedef	IWICImagingFactory;
 * typedef	IWICMetadataQueryReader;
 * typedef	IWICStream;
 * typedef	WICBitmapTransformOptions;
 */
#include <shellapi.h>
/******* """"""""""""
 * typedef	HDROP;
 * UINT		DragQueryFileW(HDROP, UINT, LPWSTR, UINT);
 */
#include <stdlib.h>
/******* """"""""""
 * void	free(void *);
 * void	*malloc(size_t);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memcpy(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 */
#include "piccell.h"
/******* """""""""
 * #define PICCELL_DONE
 * #define PICCELL_NO_CLIP
 * #define PICCELL_NO_PICTURE
 */
#include "picture.h"
/******* """""""""""
 * #define PICTURE_SIDE_MAX
 * struct S_PICTURE;
 */
/* **************************** [^] INCLUDES [^] **************************** */

/* THE EXIF ORIENTATION OF A PHOTO (1 TO 8), 1 WHEN IT HAS NONE */
static int
	EXIF_TURN(IWICBitmapFrameDecode *FRAME)
{
	IWICMetadataQueryReader	*READER;
	PROPVARIANT				VALUE;
	int						TURN;

	TURN = 1;
	READER = NULL;

	if (FAILED(IWICBitmapFrameDecode_GetMetadataQueryReader(FRAME, &READER)))
		return (1);

	PropVariantInit(&VALUE);

	if (
		SUCCEEDED(
			IWICMetadataQueryReader_GetMetadataByName(
				READER,
				L"/app1/ifd/{ushort=274}",
				&VALUE
			)
		) ||
		SUCCEEDED(
			IWICMetadataQueryReader_GetMetadataByName(
				READER,
				L"/ifd/{ushort=274}",
				&VALUE
			)
		)
	)
		if (VALUE.vt == VT_UI2)
			TURN = (int)VALUE.uiVal;

	PropVariantClear(&VALUE);
	IWICMetadataQueryReader_Release(READER);

	if (TURN < 1 || TURN > 8)
		return (1);

	return (TURN);
}

/* THE FIRST FRAME OF A DECODER, TURNED THE WAY THE PHONE MEANT IT, OR NULL */
static IWICBitmapSource
	*TURNED_FRAME(IWICImagingFactory *WIC, IWICBitmapDecoder *DECODER)
{
	static const WICBitmapTransformOptions	TURNS[9] = {
		WICBitmapTransformRotate0,
		WICBitmapTransformRotate0,
		WICBitmapTransformFlipHorizontal,
		WICBitmapTransformRotate180,
		WICBitmapTransformFlipVertical,
		WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal,
		WICBitmapTransformRotate90,
		WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal,
		WICBitmapTransformRotate270
	};
	IWICBitmapFrameDecode					*FRAME;
	IWICBitmapFlipRotator					*TURNER;
	IWICBitmapSource						*RESULT;
	int										TURN;

	FRAME = NULL;
	TURNER = NULL;

	if (FAILED(IWICBitmapDecoder_GetFrame(DECODER, 0, &FRAME)))
		return (NULL);

	RESULT = (IWICBitmapSource *)FRAME;
	TURN = EXIF_TURN(FRAME);

	if (
		TURN != 1 &&
		SUCCEEDED(IWICImagingFactory_CreateBitmapFlipRotator(WIC, &TURNER)) &&
		SUCCEEDED(
			IWICBitmapFlipRotator_Initialize(
				TURNER,
				(IWICBitmapSource *)FRAME,
				TURNS[TURN]
			)
		)
	)
	{
		RESULT = (IWICBitmapSource *)TURNER;
		IWICBitmapFrameDecode_Release(FRAME);
	}
	else if (TURNER)
		IWICBitmapFlipRotator_Release(TURNER);

	return (RESULT);
}

/* A PICTURE FILE, OR NULL */
static IWICBitmapSource
	*LOAD_FILE(IWICImagingFactory *WIC, const WCHAR *PATH)
{
	IWICBitmapDecoder	*DECODER;
	IWICBitmapSource	*RESULT;

	DECODER = NULL;

	if (
		FAILED(
			IWICImagingFactory_CreateDecoderFromFilename(
				WIC,
				PATH,
				NULL,
				GENERIC_READ,
				WICDecodeMetadataCacheOnDemand,
				&DECODER
			)
		)
	)
		return (NULL);

	RESULT = TURNED_FRAME(WIC, DECODER);
	IWICBitmapDecoder_Release(DECODER);
	return (RESULT);
}

/*
# THE PICTURE ON THE CLIPBOARD: A PICTURE FILE COPIED IN EXPLORER, OR A
# COPIED PICTURE (A SCREENSHOT, A PICTURE FROM WHATSAPP). NULL WHEN NONE.
*/
static IWICBitmapSource
	*LOAD_CLIPBOARD(IWICImagingFactory *WIC)
{
	IWICBitmapSource	*RESULT;
	WCHAR				PATH[MAX_PATH];
	HANDLE				DATA;
	int					TRY;

	RESULT = NULL;

	for (TRY = 0; TRY < 20 && !OpenClipboard(NULL); ++TRY)
		Sleep(50); /* ANOTHER PROGRAM IS USING THE CLIPBOARD */

	if (TRY == 20)
		return (NULL);

	DATA = GetClipboardData(CF_HDROP);

	if (DATA && DragQueryFileW((HDROP)DATA, 0, PATH, MAX_PATH))
		RESULT = LOAD_FILE(WIC, PATH);

	if (!RESULT)
	{
		IWICBitmap	*BITMAP;

		DATA = GetClipboardData(CF_BITMAP);
		BITMAP = NULL;

		if (
			DATA &&
			SUCCEEDED(
				IWICImagingFactory_CreateBitmapFromHBITMAP(
					WIC,
					(HBITMAP)DATA,
					NULL,
					WICBitmapIgnoreAlpha,
					&BITMAP
				)
			)
		)
			RESULT = (IWICBitmapSource *)BITMAP;
	}

	CloseClipboard();
	return (RESULT);
}

/* A PICTURE IN MEMORY, OR NULL */
static IWICBitmapSource
	*LOAD_MEMORY(IWICImagingFactory *WIC, const void *DATA, size_t SIZE)
{
	IWICBitmapDecoder	*DECODER;
	IWICBitmapSource	*RESULT;
	IWICStream			*STREAM;

	DECODER = NULL;
	STREAM = NULL;
	RESULT = NULL;

	if (
		SIZE &&
		SIZE < 0X7FFFFFFF &&
		SUCCEEDED(IWICImagingFactory_CreateStream(WIC, &STREAM)) &&
		SUCCEEDED(
			IWICStream_InitializeFromMemory(
				STREAM,
				(BYTE *)DATA,
				(DWORD)SIZE
			)
		) &&
		SUCCEEDED(
			IWICImagingFactory_CreateDecoderFromStream(
				WIC,
				(IStream *)STREAM,
				NULL,
				WICDecodeMetadataCacheOnDemand,
				&DECODER
			)
		)
	)
		RESULT = TURNED_FRAME(WIC, DECODER);

	if (DECODER)
		IWICBitmapDecoder_Release(DECODER);

	if (STREAM)
		IWICStream_Release(STREAM);

	return (RESULT);
}

/* THE PIXELS OF A SOURCE (B, G, R, A), AT MOST PICTURE_SIDE_MAX WIDE. 1 = OK */
static int
	GET_PIXELS(
		IWICImagingFactory *WIC,
		IWICBitmapSource *SOURCE,
		struct S_PICTURE *P
	)
{
	IWICBitmapScaler	*SCALER;
	IWICFormatConverter	*CONVERTER;
	IWICBitmapSource	*SIZED;
	UINT				W;
	UINT				H;

	SCALER = NULL;
	CONVERTER = NULL;

	if (FAILED(IWICBitmapSource_GetSize(SOURCE, &W, &H)) || !W || !H)
		return (0);

	SIZED = SOURCE;

	if (
		(W > PICTURE_SIDE_MAX || H > PICTURE_SIDE_MAX) &&
		SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(WIC, &SCALER))
	)
	{
		double	F;

		F = (double)PICTURE_SIDE_MAX / (double)W;

		if (H > W)
			F = (double)PICTURE_SIDE_MAX / (double)H;

		if (
			SUCCEEDED(
				IWICBitmapScaler_Initialize(
					SCALER,
					SOURCE,
					(UINT)(W * F),
					(UINT)(H * F),
					WICBitmapInterpolationModeFant
				)
			)
		)
		{
			SIZED = (IWICBitmapSource *)SCALER;
			W = (UINT)(W * F);
			H = (UINT)(H * F);
		}
	}

	if (
		SUCCEEDED(IWICImagingFactory_CreateFormatConverter(WIC, &CONVERTER)) &&
		SUCCEEDED(
			IWICFormatConverter_Initialize(
				CONVERTER,
				SIZED,
				&GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.0,
				WICBitmapPaletteTypeCustom
			)
		)
	)
	{
		P->BGRA = (unsigned char *)malloc((size_t)W * H * 4);

		if (
			P->BGRA &&
			FAILED(
				IWICFormatConverter_CopyPixels(
					CONVERTER,
					NULL,
					W * 4,
					W * H * 4,
					P->BGRA
				)
			)
		)
		{
			free(P->BGRA);
			P->BGRA = NULL;
		}
	}

	if (CONVERTER)
		IWICFormatConverter_Release(CONVERTER);

	if (SCALER)
		IWICBitmapScaler_Release(SCALER);

	P->W = (int)W;
	P->H = (int)H;
	return (P->BGRA != NULL);
}

/* FROM A FILE (PATH), THE CLIPBOARD (NEITHER) OR MEMORY (DATA): A CODE */
static int
	LOAD(
		const WCHAR *PATH,
		const void *DATA,
		size_t SIZE,
		struct S_PICTURE *P
	)
{
	IWICImagingFactory	*WIC;
	IWICBitmapSource	*SOURCE;
	HRESULT				COM;
	int					CODE;

	memset(P, 0, sizeof(*P));
	WIC = NULL;
	COM = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (
		FAILED(
			CoCreateInstance(
				&CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				&IID_IWICImagingFactory,
				(void **)&WIC
			)
		)
	)
		WIC = NULL;

	SOURCE = NULL;

	if (WIC && DATA)
		SOURCE = LOAD_MEMORY(WIC, DATA, SIZE);
	else if (WIC && PATH)
		SOURCE = LOAD_FILE(WIC, PATH);
	else if (WIC)
		SOURCE = LOAD_CLIPBOARD(WIC);

	CODE = PICCELL_NO_PICTURE;

	if (!SOURCE && !DATA && !PATH)
		CODE = PICCELL_NO_CLIP;

	if (SOURCE && GET_PIXELS(WIC, SOURCE, P))
		CODE = PICCELL_DONE;

	if (SOURCE)
		IWICBitmapSource_Release(SOURCE);

	if (WIC)
		IWICImagingFactory_Release(WIC);

	/* (ANOTHER KIND OF COM ON THIS THREAD ALREADY: NOTHING TO UNDO) */
	if (SUCCEEDED(COM))
		CoUninitialize();

	return (CODE);
}

int
	PICTURE_LOAD(const WCHAR *PATH, struct S_PICTURE *P)
{
	return (LOAD(PATH, NULL, 0, P));
}

int
	PICTURE_FROM_MEMORY(const void *DATA, size_t SIZE, struct S_PICTURE *P)
{
	if (!DATA || !SIZE)
	{
		memset(P, 0, sizeof(*P));
		return (PICCELL_NO_PICTURE);
	}

	return (LOAD(NULL, DATA, SIZE, P));
}

/*
# EVERY NEW PIXEL IS THE AVERAGE OF THE OLD ONES IT COVERS (WEIGHED BY HOW
# MUCH OF THEM IT COVERS); WHEN IT IS BIGGER, THE OLD PIXELS AROUND ITS
# MIDDLE ARE MIXED (BILINEAR). ONE DIRECTION AT A TIME.
*/
static void
	RESIZE_ROW(
		const unsigned char *SRC,
		int SRC_N,
		int SRC_STEP,
		unsigned char *DST,
		int DST_N,
		int DST_STEP
	)
{
	const double	F = (double)SRC_N / (double)DST_N;
	int				I;

	for (I = 0; I < DST_N; ++I)
	{
		double	SUM[4];
		double	WEIGHT;
		double	A;
		double	B;
		int		K;
		int		X;

		SUM[0] = 0.0;
		SUM[1] = 0.0;
		SUM[2] = 0.0;
		SUM[3] = 0.0;
		WEIGHT = 0.0;
		A = I * F;
		B = A + F;

		if (F < 1.0)
		{
			/* BIGGER: THE TWO OLD PIXELS AROUND THE MIDDLE */
			A = (I + 0.5) * F - 0.5;

			if (A < 0.0)
				A = 0.0;

			X = (int)A;
			B = A - X;

			for (K = 0; K < 4; ++K)
			{
				const int		X1 = X + (X + 1 < SRC_N);
				const double	V0 = SRC[X * SRC_STEP + K];
				const double	V1 = SRC[X1 * SRC_STEP + K];

				DST[I * DST_STEP + K] = (unsigned char)(
					V0 + (V1 - V0) * B + 0.5
				);
			}

			continue ;
		}

		for (X = (int)A; X < SRC_N && X < B; ++X)
		{
			double	W;

			W = 1.0;

			if (X < A)
				W -= A - X;

			if (X + 1 > B)
				W -= X + 1 - B;

			for (K = 0; K < 4; ++K)
				SUM[K] += W * SRC[X * SRC_STEP + K];

			WEIGHT += W;
		}

		for (K = 0; K < 4 && WEIGHT > 0.0; ++K)
			DST[I * DST_STEP + K] = (unsigned char)(SUM[K] / WEIGHT + 0.5);
	}
}

unsigned char
	*PICTURE_RESIZE(const struct S_PICTURE *P, int W, int H)
{
	unsigned char	*ACROSS;
	unsigned char	*RESULT;
	int				Y;
	int				X;

	if (W < 1 || H < 1)
		return (NULL);

	ACROSS = (unsigned char *)malloc((size_t)W * P->H * 4);
	RESULT = (unsigned char *)malloc((size_t)W * H * 4);

	if (!ACROSS || !RESULT)
	{
		free(ACROSS);
		free(RESULT);
		return (NULL);
	}

	for (Y = 0; Y < P->H; ++Y)
		RESIZE_ROW(
			P->BGRA + (size_t)Y * P->W * 4,
			P->W,
			4,
			ACROSS + (size_t)Y * W * 4,
			W,
			4
		);

	for (X = 0; X < W; ++X)
		RESIZE_ROW(
			ACROSS + (size_t)X * 4,
			P->H,
			W * 4,
			RESULT + X * 4,
			H,
			W * 4
		);

	free(ACROSS);
	return (RESULT);
}

unsigned char
	*PICTURE_GRAY(const struct S_PICTURE *P)
{
	const size_t	COUNT = (size_t)P->W * P->H;
	unsigned char	*RESULT;
	size_t			I;

	RESULT = (unsigned char *)malloc(COUNT);

	if (!RESULT)
		return (NULL);

	/* (B, G, R): 0.114, 0.587, 0.299, IN 1/1024 */
	for (I = 0; I < COUNT; ++I)
	{
		const unsigned char	*PX = P->BGRA + I * 4;

		RESULT[I] = (unsigned char)(
			(117 * PX[0] + 601 * PX[1] + 306 * PX[2]) >> 10
		);
	}

	return (RESULT);
}

/*
# A JPEG OF THE PICTURE, AT MOST SIDE PIXELS WIDE AND HIGH, FOR THE WINDOW
# TO SHOW. NULL = NO MEMORY OR NO ENCODER.
*/
unsigned char
	*PICTURE_JPEG(const struct S_PICTURE *P, int SIDE, size_t *SIZE)
{
	IWICImagingFactory		*WIC;
	IWICBitmap				*BITMAP;
	IWICFormatConverter		*CONVERTER;
	IWICBitmapEncoder		*ENCODER;
	IWICBitmapFrameEncode	*FRAME;
	IStream					*STREAM;
	unsigned char			*SMALL;
	unsigned char			*RESULT;
	HRESULT					COM;
	double					F;
	int						W;
	int						H;

	F = 1.0;

	if (P->W > SIDE || P->H > SIDE)
		F = (double)SIDE / (double)P->W;

	if (P->H > P->W && P->H > SIDE)
		F = (double)SIDE / (double)P->H;

	W = (int)(P->W * F);
	H = (int)(P->H * F);

	if (W < 1)
		W = 1;

	if (H < 1)
		H = 1;

	SMALL = PICTURE_RESIZE(P, W, H);

	if (!SMALL)
		return (NULL);

	WIC = NULL;
	BITMAP = NULL;
	CONVERTER = NULL;
	ENCODER = NULL;
	FRAME = NULL;
	STREAM = NULL;
	RESULT = NULL;
	COM = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (
		SUCCEEDED(
			CoCreateInstance(
				&CLSID_WICImagingFactory,
				NULL,
				CLSCTX_INPROC_SERVER,
				&IID_IWICImagingFactory,
				(void **)&WIC
			)
		) &&
		SUCCEEDED(
			IWICImagingFactory_CreateBitmapFromMemory(
				WIC,
				(UINT)W,
				(UINT)H,
				&GUID_WICPixelFormat32bppPBGRA,
				(UINT)W * 4,
				(UINT)W * H * 4,
				SMALL,
				&BITMAP
			)
		) &&
		SUCCEEDED(IWICImagingFactory_CreateFormatConverter(WIC, &CONVERTER)) &&
		SUCCEEDED(
			IWICFormatConverter_Initialize(
				CONVERTER,
				(IWICBitmapSource *)BITMAP,
				&GUID_WICPixelFormat24bppBGR,
				WICBitmapDitherTypeNone,
				NULL,
				0.0,
				WICBitmapPaletteTypeCustom
			)
		) &&
		SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &STREAM)) &&
		SUCCEEDED(
			IWICImagingFactory_CreateEncoder(
				WIC,
				&GUID_ContainerFormatJpeg,
				NULL,
				&ENCODER
			)
		) &&
		SUCCEEDED(
			IWICBitmapEncoder_Initialize(
				ENCODER,
				STREAM,
				WICBitmapEncoderNoCache
			)
		) &&
		SUCCEEDED(IWICBitmapEncoder_CreateNewFrame(ENCODER, &FRAME, NULL)) &&
		SUCCEEDED(IWICBitmapFrameEncode_Initialize(FRAME, NULL)) &&
		SUCCEEDED(IWICBitmapFrameEncode_SetSize(FRAME, (UINT)W, (UINT)H)) &&
		SUCCEEDED(
			IWICBitmapFrameEncode_WriteSource(
				FRAME,
				(IWICBitmapSource *)CONVERTER,
				NULL
			)
		) &&
		SUCCEEDED(IWICBitmapFrameEncode_Commit(FRAME)) &&
		SUCCEEDED(IWICBitmapEncoder_Commit(ENCODER))
	)
	{
		HGLOBAL	MEMORY;
		void	*BYTES;

		MEMORY = NULL;
		BYTES = NULL;

		if (SUCCEEDED(GetHGlobalFromStream(STREAM, &MEMORY)) && MEMORY)
			BYTES = GlobalLock(MEMORY);

		if (BYTES)
		{
			*SIZE = GlobalSize(MEMORY);
			RESULT = (unsigned char *)malloc(*SIZE);

			if (RESULT)
				memcpy(RESULT, BYTES, *SIZE);

			GlobalUnlock(MEMORY);
		}
	}

	if (FRAME)
		IWICBitmapFrameEncode_Release(FRAME);

	if (ENCODER)
		IWICBitmapEncoder_Release(ENCODER);

	if (STREAM)
		IStream_Release(STREAM);

	if (CONVERTER)
		IWICFormatConverter_Release(CONVERTER);

	if (BITMAP)
		IWICBitmap_Release(BITMAP);

	if (WIC)
		IWICImagingFactory_Release(WIC);

	if (SUCCEEDED(COM))
		CoUninitialize();

	free(SMALL);
	return (RESULT);
}

int
	PICTURE_CROP(
		const struct S_PICTURE *P,
		int X,
		int Y,
		int W,
		int H,
		struct S_PICTURE *PART
	)
{
	int	ROW;

	memset(PART, 0, sizeof(*PART));

	if (X < 0 || Y < 0 || W < 1 || H < 1 || X + W > P->W || Y + H > P->H)
		return (0);

	PART->BGRA = (unsigned char *)malloc((size_t)W * (size_t)H * 4);

	if (!PART->BGRA)
		return (0);

	PART->W = W;
	PART->H = H;

	for (ROW = 0; ROW < H; ++ROW)
		memcpy(
			PART->BGRA + (size_t)ROW * (size_t)W * 4,
			P->BGRA + ((size_t)(Y + ROW) * (size_t)P->W + (size_t)X) * 4,
			(size_t)W * 4
		);

	return (1);
}

void
	PICTURE_FREE(struct S_PICTURE *P)
{
	free(P->BGRA);
	memset(P, 0, sizeof(*P));
}
