/******************************************************************************\
# C - ocr                                        #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/18 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/21 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE PICTURE (SEE PICTURE.C) IS MADE BIG ENOUGH BUT NOT TOO BIG FOR THE
# OCR, ITS TABLE BORDERS ARE ERASED (SEE LINES.C), AND WINDOWS.MEDIA.OCR
# READS IT. THE WINRT FUNCTIONS ARE TAKEN
# FROM COMBASE.DLL AT RUN TIME, SO AN OLD WINDOWS GETS A CLEAR "NO OCR".
*/

#ifndef COBJMACROS
#define COBJMACROS /* OBJECT_METHOD(THIS, ...) STYLE COM CALLS IN C */
#endif /* !COBJMACROS */

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define CP_UTF8
 * #define FAILED(hr)
 * #define REFIID
 * #define SUCCEEDED(hr)
 * #define WINAPI
 * typedef	DWORD;
 * typedef	HMODULE;
 * typedef	HRESULT;
 * typedef	IID;
 * typedef	LPCWSTR;
 * typedef	UINT32;
 * typedef	UINT;
 * typedef	WCHAR;
 * FARPROC	GetProcAddress(HMODULE, LPCSTR);
 * DWORD	GetTickCount(void);
 * HMODULE	LoadLibraryW(LPCWSTR);
 * void		Sleep(DWORD);
 * int		WideCharToMultiByte(
 * 	UINT,
 * 	DWORD,
 * 	LPCWCH,
 * 	int,
 * 	LPSTR,
 * 	int,
 * 	LPCCH,
 * 	LPBOOL
 * );
 */
#include <stdlib.h>
/******* """"""""""
 * void	free(void *);
 * void	*realloc(void *, size_t);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memset(void *, int, size_t);
 */
#include "piccell.h"
/******* """""""""
 * #define PICCELL_DONE
 * #define PICCELL_NO_OCR
 * #define PICCELL_NO_PICTURE
 * #define PICCELL_NO_TEXT
 * #define PICCELL_WORD_MAX
 * struct S_WORD;
 */
#include "winrt.h"
/******* """""""""
 * #define ASYNC_COMPLETED
 * #define ASYNC_STARTED
 * #define PIXEL_BGRA8
 * #define WINRT_CALL(TYPE, OBJECT)
 * struct S_BITMAP_STATICS_VTBL;
 * struct S_BUFFER_STATICS_VTBL;
 * struct S_LANGUAGE_FACTORY_VTBL;
 * struct S_OCR_STATICS_VTBL;
 * struct S_RECT;
 * struct S_VECTOR_VIEW_VTBL;
 * typedef	WINRT_STRING;
 */
#include "lines.h"
/******* """""""""
 * int	LINES_ERASE(unsigned char *, int, int);
 */
#include "picture.h"
/******* """""""""""
 * struct S_PICTURE;
 * unsigned char	*PICTURE_RESIZE(const struct S_PICTURE *, int, int);
 */
#include "ocr.h"
/******* """""""
 * int	OCR_READ(
 * 	const struct S_PICTURE *,
 * 	const WCHAR *,
 * 	struct S_WORD **,
 * 	int *
 * );
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define SMALL_PICTURE 1600 /* A PICTURE SMALLER THAN THIS IS DOUBLED */
#define WAIT_LIMIT_MS 120000 /* THE OCR GETS TWO MINUTES */
#define RO_INIT_MTA 1 /* RO_INIT_MULTITHREADED */

typedef HRESULT	(WINAPI *T_RO_INITIALIZE)(int);
typedef void	(WINAPI *T_RO_UNINITIALIZE)(void);
typedef HRESULT	(WINAPI *T_RO_FACTORY)(WINRT_STRING, REFIID, void **);
typedef HRESULT	(WINAPI *T_MAKE_STRING)(LPCWSTR, UINT32, WINRT_STRING *);
typedef HRESULT	(WINAPI *T_DELETE_STRING)(WINRT_STRING);
typedef LPCWSTR	(WINAPI *T_STRING_TEXT)(WINRT_STRING, UINT32 *);

struct	S_COMBASE /* THE WINRT FUNCTIONS OF COMBASE.DLL */
{
	HMODULE				DLL;
	T_RO_INITIALIZE		RO_INITIALIZE;
	T_RO_UNINITIALIZE	RO_UNINITIALIZE;
	T_RO_FACTORY		RO_FACTORY;
	T_MAKE_STRING		MAKE_STRING;
	T_DELETE_STRING		DELETE_STRING;
	T_STRING_TEXT		STRING_TEXT;
};

struct	S_OCR /* EVERYTHING ONE READ NEEDS */
{
	void	*OCR_STATICS;
	void	*BITMAP_STATICS;
	void	*BUFFER_STATICS;
	void	*ENGINE;
	UINT32	LIMIT; /* LARGEST SIDE THE OCR TAKES */
};

static const IID	IID_OCR_STATICS = {
	0X5BFFA85A, 0X3384, 0X3540,
	{0X99, 0X40, 0X69, 0X91, 0X20, 0XD4, 0X28, 0XA8}
};
static const IID	IID_BITMAP_STATICS = {
	0XDF0385DB, 0X672F, 0X4A9D,
	{0X80, 0X6E, 0XC2, 0X44, 0X2F, 0X34, 0X3E, 0X86}
};
static const IID	IID_BUFFER_STATICS = {
	0X320B7E22, 0X3CB0, 0X4CDF,
	{0X86, 0X63, 0X1D, 0X28, 0X91, 0X00, 0X65, 0XEB}
};
static const IID	IID_LANGUAGE_FACTORY = {
	0X9B0252AC, 0X0C27, 0X44F8,
	{0XB7, 0X92, 0X97, 0X93, 0XFB, 0X66, 0XC6, 0X3E}
};
static const IID	IID_ASYNC_INFO = {
	0X00000036, 0X0000, 0X0000,
	{0XC0, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X46}
};

static struct S_COMBASE	G__RT__;

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          WINRT                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* 1 = THE WINRT FUNCTIONS WERE FOUND (WINDOWS 8 AND NEWER) */
static int
	LOAD_COMBASE(void)
{
	G__RT__.DLL = LoadLibraryW(L"combase.dll");

	if (!G__RT__.DLL)
		return (0);

	G__RT__.RO_INITIALIZE = (T_RO_INITIALIZE)(void *)GetProcAddress(
		G__RT__.DLL,
		"RoInitialize"
	);
	G__RT__.RO_UNINITIALIZE = (T_RO_UNINITIALIZE)(void *)GetProcAddress(
		G__RT__.DLL,
		"RoUninitialize"
	);
	G__RT__.RO_FACTORY = (T_RO_FACTORY)(void *)GetProcAddress(
		G__RT__.DLL,
		"RoGetActivationFactory"
	);
	G__RT__.MAKE_STRING = (T_MAKE_STRING)(void *)GetProcAddress(
		G__RT__.DLL,
		"WindowsCreateString"
	);
	G__RT__.DELETE_STRING = (T_DELETE_STRING)(void *)GetProcAddress(
		G__RT__.DLL,
		"WindowsDeleteString"
	);
	G__RT__.STRING_TEXT = (T_STRING_TEXT)(void *)GetProcAddress(
		G__RT__.DLL,
		"WindowsGetStringRawBuffer"
	);
	return (
		G__RT__.RO_INITIALIZE &&
		G__RT__.RO_UNINITIALIZE &&
		G__RT__.RO_FACTORY &&
		G__RT__.MAKE_STRING &&
		G__RT__.DELETE_STRING &&
		G__RT__.STRING_TEXT
	);
}

static void
	RELEASE(void *OBJECT)
{
	if (OBJECT)
		WINRT_CALL(S_INSPECTABLE_VTBL, OBJECT)->Release(OBJECT);
}

/* THE STATIC METHODS (FACTORY) OF A WINRT CLASS, OR NULL */
static void
	*FACTORY(const WCHAR *CLASS, const IID *WANTED)
{
	WINRT_STRING	NAME;
	void			*RESULT;

	RESULT = NULL;

	if (FAILED(G__RT__.MAKE_STRING(CLASS, (UINT32)wcslen(CLASS), &NAME)))
		return (NULL);

	if (FAILED(G__RT__.RO_FACTORY(NAME, WANTED, &RESULT)))
		RESULT = NULL;

	G__RT__.DELETE_STRING(NAME);
	return (RESULT);
}

/* WAITS FOR AN IASYNCOPERATION<T>; ITS RESULT, OR NULL. RELEASES OPERATION */
static void
	*WAIT(void *OPERATION)
{
	void	*INFO;
	void	*RESULT;
	DWORD	START;
	int		STATUS;

	if (!OPERATION)
		return (NULL);

	RESULT = NULL;
	INFO = NULL;
	WINRT_CALL(S_INSPECTABLE_VTBL, OPERATION)->QueryInterface(
		OPERATION,
		&IID_ASYNC_INFO,
		&INFO
	);
	STATUS = ASYNC_STARTED;
	START = GetTickCount();

	while (
		INFO &&
		SUCCEEDED(
			WINRT_CALL(S_ASYNC_INFO_VTBL, INFO)->get_Status(INFO, &STATUS)
		) &&
		STATUS == ASYNC_STARTED &&
		GetTickCount() - START < WAIT_LIMIT_MS
	)
		Sleep(5);

	if (
		STATUS == ASYNC_COMPLETED &&
		FAILED(
			WINRT_CALL(S_ASYNC_OP_VTBL, OPERATION)->GetResults(
				OPERATION,
				&RESULT
			)
		)
	)
		RESULT = NULL;

	RELEASE(INFO);
	RELEASE(OPERATION);
	return (RESULT);
}

/* AN OCR ENGINE FOR LANGUAGE ("tr", "en-US"...), OR NULL WHEN NOT INSTALLED */
static void
	*ENGINE_FOR(void *STATICS, const WCHAR *LANGUAGE)
{
	void *const	LANGUAGES = FACTORY(
		L"Windows.Globalization.Language",
		&IID_LANGUAGE_FACTORY
	);
	WINRT_STRING	TAG;
	void			*LANG;
	void			*ENGINE;

	ENGINE = NULL;
	LANG = NULL;

	if (
		LANGUAGES &&
		SUCCEEDED(G__RT__.MAKE_STRING(LANGUAGE, (UINT32)wcslen(LANGUAGE), &TAG))
	)
	{
		const struct S_LANGUAGE_FACTORY_VTBL	*const MAKER = WINRT_CALL(
			S_LANGUAGE_FACTORY_VTBL,
			LANGUAGES
		);

		if (FAILED(MAKER->CreateLanguage(LANGUAGES, TAG, &LANG)))
			LANG = NULL;

		G__RT__.DELETE_STRING(TAG);
	}

	if (
		LANG &&
		FAILED(
			WINRT_CALL(S_OCR_STATICS_VTBL, STATICS)->TryCreateFromLanguage(
				STATICS,
				LANG,
				&ENGINE
			)
		)
	)
		ENGINE = NULL;

	RELEASE(LANG);
	RELEASE(LANGUAGES);
	return (ENGINE);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          WINRT                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                           OCR                            |  \\//  */
/*   \/   #==========================================================#   \/   */
/* A SOFTWAREBITMAP WITH A COPY OF THE PIXELS, OR NULL */
static void
	*MAKE_BITMAP(struct S_OCR *O, unsigned char *PIXELS, UINT W, UINT H)
{
	const struct S_BUFFER_STATICS_VTBL	*const BUFFERS = WINRT_CALL(
		S_BUFFER_STATICS_VTBL,
		O->BUFFER_STATICS
	);
	const struct S_BITMAP_STATICS_VTBL	*const BITMAPS = WINRT_CALL(
		S_BITMAP_STATICS_VTBL,
		O->BITMAP_STATICS
	);
	void								*BUFFER;
	void								*BITMAP;

	BUFFER = NULL;
	BITMAP = NULL;

	if (
		FAILED(
			BUFFERS->CreateFromByteArray(
				O->BUFFER_STATICS,
				W * H * 4,
				PIXELS,
				&BUFFER
			)
		)
	)
		return (NULL);

	if (
		FAILED(
			BITMAPS->CreateCopyFromBuffer(
				O->BITMAP_STATICS,
				BUFFER,
				PIXEL_BGRA8,
				(INT32)W,
				(INT32)H,
				&BITMAP
			)
		)
	)
		BITMAP = NULL;

	RELEASE(BUFFER);
	return (BITMAP);
}

/* ONE OCR WORD INTO THE LIST */
static int
	ADD_WORD(
		void *WORD,
		int LINE,
		struct S_WORD **WORDS,
		int *COUNT,
		int *ROOM
	)
{
	struct S_WORD	*W;
	struct S_RECT	BOX;
	WINRT_STRING	STRING;
	LPCWSTR			TEXT;
	UINT32			LENGTH;
	int				BYTES;

	if (
		FAILED(
			WINRT_CALL(S_OCR_WORD_VTBL, WORD)->get_BoundingRect(WORD, &BOX)
		) ||
		FAILED(WINRT_CALL(S_OCR_WORD_VTBL, WORD)->get_Text(WORD, &STRING))
	)
		return (1);

	if (*COUNT == *ROOM)
	{
		struct S_WORD	*MORE;

		if (*ROOM)
			*ROOM *= 2;
		else
			*ROOM = 256;

		MORE = (struct S_WORD *)realloc(
			*WORDS,
			(size_t)*ROOM * sizeof(**WORDS)
		);

		if (!MORE)
		{
			G__RT__.DELETE_STRING(STRING);
			return (0);
		}

		*WORDS = MORE;
	}

	W = &(*WORDS)[*COUNT];
	TEXT = G__RT__.STRING_TEXT(STRING, &LENGTH);
	BYTES = WideCharToMultiByte(
		CP_UTF8,
		0,
		TEXT,
		(int)LENGTH,
		W->TEXT,
		PICCELL_WORD_MAX - 1,
		NULL,
		NULL
	);
	G__RT__.DELETE_STRING(STRING);

	if (BYTES < 0)
		BYTES = 0;

	W->TEXT[BYTES] = '\0';
	W->LINE = LINE;
	W->X = BOX.X;
	W->Y = BOX.Y;
	W->W = BOX.WIDTH;
	W->H = BOX.HEIGHT;
	*COUNT += W->TEXT[0] != '\0';
	return (1);
}

/* ALL WORDS OF AN OCRRESULT, LINE BY LINE */
static int
	COLLECT_WORDS(void *RESULT, struct S_WORD **WORDS, int *COUNT)
{
	void	*LINES;
	UINT32	LINE_COUNT;
	UINT32	I;
	int		ROOM;
	int		OK;

	ROOM = 0;
	OK = 1;
	LINES = NULL;
	LINE_COUNT = 0;

	if (
		FAILED(
			WINRT_CALL(S_OCR_RESULT_VTBL, RESULT)->get_Lines(RESULT, &LINES)
		) ||
		FAILED(
			WINRT_CALL(S_VECTOR_VIEW_VTBL, LINES)->get_Size(LINES, &LINE_COUNT)
		)
	)
	{
		RELEASE(LINES);
		return (0);
	}

	for (I = 0; I < LINE_COUNT && OK; ++I)
	{
		void	*LINE;
		void	*LIST;
		UINT32	WORD_COUNT;

		LINE = NULL;
		LIST = NULL;
		WORD_COUNT = 0;

		if (
			SUCCEEDED(
				WINRT_CALL(S_VECTOR_VIEW_VTBL, LINES)->GetAt(LINES, I, &LINE)
			) &&
			SUCCEEDED(
				WINRT_CALL(S_OCR_LINE_VTBL, LINE)->get_Words(LINE, &LIST)
			) &&
			SUCCEEDED(
				WINRT_CALL(S_VECTOR_VIEW_VTBL, LIST)->get_Size(
					LIST,
					&WORD_COUNT
				)
			)
		)
		{
			const struct S_VECTOR_VIEW_VTBL	*const WORD_LIST = WINRT_CALL(
				S_VECTOR_VIEW_VTBL,
				LIST
			);
			UINT32							J;

			for (J = 0; J < WORD_COUNT && OK; ++J)
			{
				void	*WORD;

				WORD = NULL;

				if (SUCCEEDED(WORD_LIST->GetAt(LIST, J, &WORD)))
					OK = ADD_WORD(WORD, (int)I, WORDS, COUNT, &ROOM);

				RELEASE(WORD);
			}
		}

		RELEASE(LIST);
		RELEASE(LINE);
	}

	RELEASE(LINES);
	return (OK);
}

/* THE OCR CLASSES AND AN ENGINE. A PICCELL_ CODE */
static int
	OPEN_OCR(struct S_OCR *O, const WCHAR *LANGUAGE)
{
	const struct S_OCR_STATICS_VTBL	*OCR;

	O->OCR_STATICS = FACTORY(L"Windows.Media.Ocr.OcrEngine", &IID_OCR_STATICS);
	O->BITMAP_STATICS = FACTORY(
		L"Windows.Graphics.Imaging.SoftwareBitmap",
		&IID_BITMAP_STATICS
	);
	O->BUFFER_STATICS = FACTORY(
		L"Windows.Security.Cryptography.CryptographicBuffer",
		&IID_BUFFER_STATICS
	);

	if (!O->OCR_STATICS || !O->BITMAP_STATICS || !O->BUFFER_STATICS)
		return (PICCELL_NO_OCR);

	OCR = WINRT_CALL(S_OCR_STATICS_VTBL, O->OCR_STATICS);

	if (
		FAILED(OCR->get_MaxImageDimension(O->OCR_STATICS, &O->LIMIT)) ||
		O->LIMIT < 500
	)
		O->LIMIT = 2600;

	if (LANGUAGE)
		O->ENGINE = ENGINE_FOR(O->OCR_STATICS, LANGUAGE);
	else
		O->ENGINE = ENGINE_FOR(O->OCR_STATICS, L"tr");

	if (
		!O->ENGINE &&
		FAILED(
			OCR->TryCreateFromUserProfileLanguages(O->OCR_STATICS, &O->ENGINE)
		)
	)
		O->ENGINE = NULL;

	if (!O->ENGINE)
		O->ENGINE = ENGINE_FOR(O->OCR_STATICS, L"en-US");

	if (!O->ENGINE)
		return (PICCELL_NO_OCR);

	return (PICCELL_DONE);
}

static void
	CLOSE_OCR(struct S_OCR *O)
{
	RELEASE(O->ENGINE);
	RELEASE(O->OCR_STATICS);
	RELEASE(O->BITMAP_STATICS);
	RELEASE(O->BUFFER_STATICS);
}

/*
# HOW MUCH BIGGER THE OCR SHOULD SEE THE PICTURE: A SMALL PICTURE IS
# DOUBLED, ONE TOO BIG FOR THE OCR IS MADE SMALLER.
*/
static double
	OCR_SCALE(const struct S_PICTURE *P, UINT32 LIMIT)
{
	int	SIDE;

	SIDE = P->W;

	if (P->H > SIDE)
		SIDE = P->H;

	if ((UINT32)SIDE > LIMIT)
		return ((double)LIMIT / (double)SIDE);

	if (SIDE < SMALL_PICTURE && 2.0 * SIDE > LIMIT)
		return ((double)LIMIT / (double)SIDE);

	if (SIDE < SMALL_PICTURE)
		return (2.0);

	return (1.0);
}

/* THE PICTURE TO WORDS (IN PIXELS OF THE PICTURE). A PICCELL_ CODE */
static int
	READ_PICTURE(
		struct S_OCR *O,
		const struct S_PICTURE *P,
		struct S_WORD **WORDS,
		int *COUNT
	)
{
	unsigned char	*PIXELS;
	void			*BITMAP;
	void			*OPERATION;
	void			*RESULT;
	double			SCALE;
	UINT			W;
	UINT			H;
	int				CODE;
	int				I;

	SCALE = OCR_SCALE(P, O->LIMIT);
	W = (UINT)(P->W * SCALE);
	H = (UINT)(P->H * SCALE);

	if (W < 1)
		W = 1;

	if (H < 1)
		H = 1;

	PIXELS = PICTURE_RESIZE(P, (int)W, (int)H);

	if (!PIXELS)
		return (PICCELL_NO_PICTURE);

	LINES_ERASE(PIXELS, (int)W, (int)H);
	BITMAP = MAKE_BITMAP(O, PIXELS, W, H);
	free(PIXELS);
	OPERATION = NULL;

	if (
		!BITMAP ||
		FAILED(
			WINRT_CALL(S_OCR_ENGINE_VTBL, O->ENGINE)->RecognizeAsync(
				O->ENGINE,
				BITMAP,
				&OPERATION
			)
		)
	)
	{
		RELEASE(BITMAP);
		return (PICCELL_NO_OCR);
	}

	RESULT = WAIT(OPERATION);
	RELEASE(BITMAP);

	if (!RESULT)
		return (PICCELL_NO_OCR);

	CODE = PICCELL_NO_OCR;

	if (COLLECT_WORDS(RESULT, WORDS, COUNT))
		CODE = PICCELL_DONE;

	RELEASE(RESULT);

	if (CODE == PICCELL_DONE && !*COUNT)
		CODE = PICCELL_NO_TEXT;

	/* BACK TO THE PIXELS OF THE PICTURE */
	for (I = 0; CODE == PICCELL_DONE && I < *COUNT; ++I)
	{
		(*WORDS)[I].X /= (float)SCALE;
		(*WORDS)[I].Y /= (float)SCALE;
		(*WORDS)[I].W /= (float)SCALE;
		(*WORDS)[I].H /= (float)SCALE;
	}

	return (CODE);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                           OCR                            |  //\\  */
/* '""""' #==========================================================# '""""' */

int
	OCR_READ(
		const struct S_PICTURE *PICTURE,
		const WCHAR *LANGUAGE,
		struct S_WORD **WORDS,
		int *COUNT
	)
{
	struct S_OCR	O;
	int				CODE;

	*WORDS = NULL;
	*COUNT = 0;
	memset(&O, 0, sizeof(O));

	if (!LOAD_COMBASE() || FAILED(G__RT__.RO_INITIALIZE(RO_INIT_MTA)))
		return (PICCELL_NO_OCR);

	CODE = OPEN_OCR(&O, LANGUAGE);

	if (CODE == PICCELL_DONE)
		CODE = READ_PICTURE(&O, PICTURE, WORDS, COUNT);

	CLOSE_OCR(&O);
	G__RT__.RO_UNINITIALIZE();

	if (CODE != PICCELL_DONE)
	{
		free(*WORDS);
		*WORDS = NULL;
		*COUNT = 0;
	}

	return (CODE);
}
