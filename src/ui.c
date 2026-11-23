/******************************************************************************\
# C - ui                                         #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/23 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE PAGE CALLS THESE (EVERY ONE GIVES BACK A PROMISE, SEE WEBVIEW_BIND):
#   piccell.info()             {lang, ocr, reader, version, updated}
#   piccell.open()             A PHOTO FROM A FILE: {code, w, h, preview}
#   piccell.paste()            THE SAME, FROM THE CLIPBOARD
#   piccell.drop(base64)       THE SAME, FROM THE BYTES OF A DROPPED FILE
#   piccell.crop(x0,y0,x1,y1)  ONLY THAT PART OF THE PHOTO (0 TO 1): THE SAME
#   piccell.whole()            THE PHOTO AS IT WAS BEFORE IT WAS CROPPED
#   piccell.read(mode)         {code, used, rows, check}, IN ANOTHER THREAD
#   piccell.save(kind, rows)   {code, path}, AFTER A SAVE DIALOG
#   piccell.copy(rows)         TAB SEPARATED TEXT ON THE CLIPBOARD
#   piccell.show(path)         THE SAVED FILE OPENS (IN EXCEL...)
#   piccell.learn(texts)       THE CELLS THE USER FIXED BECOME KNOWN WORDS
#   piccell.language(lang)     "tr" OR "en"
#   piccell.update()           {ready, tag}, WHEN THE UPDATE CHECK ENDS
#   piccell.restart()          THE NEW EXE THAT IS READY OPENS INSTEAD, NOW
# A CANCELLED DIALOG ANSWERS {cancel: 1}.
*/

/* **************************** [v] INCLUDES [v] **************************** */
#ifdef __GNUC__
/* (FROM THE WEBVIEW LIBRARY: A FALSE ONE, AND A NOTE ABOUT ITS SIZE) */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif /* __GNUC__ */

/* -isystem webview: THE WARNINGS OF THE WEBVIEW LIBRARY ARE NOT OURS */
#include <webview.h>
/******* """""""""""
 * struct webview;
 * int	webview_bind(struct webview *, const char *, webview_bind_fn, void *);
 * void	webview_exit(struct webview *);
 * int	webview_init(struct webview *);
 * int	webview_loop(struct webview *, int);
 * int	webview_return(struct webview *, const char *, int, const char *);
 * void	webview_set_app_name(const char *);
 * int	webview_set_html(struct webview *, const char *);
 * void	webview_terminate(struct webview *);
 */
#include <wv_value.h>
/******* """"""""""""
 * typedef		wv_value;
 * wv_value		*wv_at(const wv_value *, int);
 * int			wv_count(const wv_value *);
 * void			wv_free(wv_value *);
 * double		wv_number(const wv_value *);
 * wv_value		*wv_parse(const char *);
 * const char	*wv_string(const wv_value *);
 */
#include <windows.h>
/******* """""""""""
 * #define CF_UNICODETEXT
 * #define CP_UTF8
 * #define GMEM_MOVEABLE
 * #define InterlockedExchange
 * #define LANG_TURKISH
 * #define MB_ICONERROR
 * #define PRIMARYLANGID(lgid)
 * #define RT_RCDATA
 * #define SW_SHOWNORMAL
 * #define WINAPI
 * typedef	DWORD;
 * typedef	HANDLE;
 * typedef	HGLOBAL;
 * typedef	HMODULE;
 * typedef	HRSRC;
 * typedef	LONG;
 * typedef	LPCWSTR;
 * typedef	LPVOID;
 * typedef	OSVERSIONINFOW;
 * typedef	SYSTEMTIME;
 * typedef	WCHAR;
 * BOOL		CloseClipboard(void);
 * BOOL		CloseHandle(HANDLE);
 * HANDLE	CreateThread(
 * 	LPSECURITY_ATTRIBUTES,
 * 	SIZE_T,
 * 	LPTHREAD_START_ROUTINE,
 * 	LPVOID,
 * 	DWORD,
 * 	LPDWORD
 * );
 * BOOL		EmptyClipboard(void);
 * HRSRC	FindResourceW(HMODULE, LPCWSTR, LPCWSTR);
 * void		GetLocalTime(LPSYSTEMTIME);
 * HMODULE	GetModuleHandleW(LPCWSTR);
 * FARPROC	GetProcAddress(HMODULE, LPCSTR);
 * LANGID	GetUserDefaultUILanguage(void);
 * HGLOBAL	GlobalAlloc(UINT, SIZE_T);
 * HGLOBAL	GlobalFree(HGLOBAL);
 * LPVOID	GlobalLock(HGLOBAL);
 * BOOL		GlobalUnlock(HGLOBAL);
 * HGLOBAL	LoadResource(HMODULE, HRSRC);
 * LPVOID	LockResource(HGLOBAL);
 * int		MessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
 * int		MultiByteToWideChar(UINT, DWORD, LPCCH, int, LPWSTR, int);
 * BOOL		OpenClipboard(HWND);
 * HANDLE	SetClipboardData(UINT, HANDLE);
 * DWORD	SizeofResource(HMODULE, HRSRC);
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
#include <commdlg.h>
/******* """""""""""
 * #define OFN_FILEMUSTEXIST
 * #define OFN_NOCHANGEDIR
 * #define OFN_OVERWRITEPROMPT
 * #define OFN_PATHMUSTEXIST
 * typedef	OPENFILENAMEW;
 * BOOL		GetOpenFileNameW(LPOPENFILENAMEW);
 * BOOL		GetSaveFileNameW(LPOPENFILENAMEW);
 */
#include <shellapi.h>
/******* """"""""""""
 * HINSTANCE	ShellExecuteW(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
 */
#include <stdio.h>
/******* """""""""
 * int	_snwprintf(wchar_t *, size_t, const wchar_t *, ...);
 * int	snprintf(char *, size_t, const char *, ...);
 */
#include <stdlib.h>
/******* """"""""""
 * void	*calloc(size_t, size_t);
 * void	free(void *);
 * void	*malloc(size_t);
 * void	*realloc(void *, size_t);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * typedef	size_t;
 * void		*memcpy(void *, const void *, size_t);
 * void		*memset(void *, int, size_t);
 * int		strcmp(const char *, const char *);
 * char		*strcpy(char *, const char *);
 * size_t	strlen(const char *);
 */
#include "piccell.h"
/******* """""""""
 * #define PICCELL_DONE
 * #define PICCELL_NO_PICTURE
 * #define PICCELL_USAGE
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 * void	LAYOUT_FREE(struct S_TABLE *);
 */
#include "picture.h"
/******* """""""""""
 * struct S_PICTURE;
 * int				PICTURE_CROP(
 * 	const struct S_PICTURE *,
 * 	int,
 * 	int,
 * 	int,
 * 	int,
 * 	struct S_PICTURE *
 * );
 * void				PICTURE_FREE(struct S_PICTURE *);
 * int				PICTURE_FROM_MEMORY(
 * 	const void *,
 * 	size_t,
 * 	struct S_PICTURE *
 * );
 * unsigned char	*PICTURE_JPEG(const struct S_PICTURE *, int, size_t *);
 * int				PICTURE_LOAD(const WCHAR *, struct S_PICTURE *);
 */
#include "convert.h"
/******* """""""""""
 * #define CONVERT_AUTO
 * #define CONVERT_HAND
 * #define CONVERT_PRINT
 * int	CONVERT(
 * 	const struct S_PICTURE *,
 * 	int,
 * 	const WCHAR *,
 * 	const WCHAR *,
 * 	struct S_TABLE *,
 * 	int *
 * );
 * int	CONVERT_SAVE(const struct S_TABLE *, const WCHAR *, const WCHAR *);
 * int	CONVERT_LEARN(const char *);
 * int	CONVERT_START(const WCHAR *);
 */
#include "version.h"
/******* """""""""""
 * #define PICCELL_VERSION_TEXT
 */
#include "update.h"
/******* """"""""""
 * #define UPDATE_READY
 * int	UPDATE_APPLY(int);
 * int	UPDATE_FIND(char *, size_t);
 */
#include "ui.h"
/******* """"""
 * int	UI_RUN(const WCHAR *, int, int);
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define PREVIEW_SIDE 1600 /* PIXELS OF THE PHOTO THE PAGE SHOWS */
#define PATH_MAX_W 1024

struct	S_TEXT /* A GROWING STRING */
{
	char	*DATA;
	size_t	SIZE;
	size_t	ROOM;
	int		FAILED;
};

struct	S_JOB /* ONE READ, IN ITS OWN THREAD */
{
	char	SEQ[64];
	int		MODE;
};

static struct webview		G__VIEW__;
static struct S_PICTURE		G__PICTURE__;
static struct S_PICTURE		G__WHOLE__; /* BEFORE THE CROP. BGRA NULL = NONE */
static volatile LONG		G__BUSY__;
static volatile LONG		G__ASKED__; /* THE PAGE ASKED FOR THE UPDATE */
static int					G__ENGLISH__;
static int					G__UPDATES__; /* 0 = -NOUPDATE */
static int					G__UPDATED__; /* 1 = -UPDATED: IT JUST HAPPENED */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                       SMALL THINGS                       |  \\//  */
/*   \/   #==========================================================#   \/   */
static void
	ADD(struct S_TEXT *T, const char *DATA, size_t SIZE)
{
	if (T->FAILED)
		return ;

	if (T->SIZE + SIZE + 1 > T->ROOM)
	{
		size_t	ROOM;
		char	*BIGGER;

		ROOM = T->ROOM * 2 + 1024;

		while (ROOM < T->SIZE + SIZE + 1)
			ROOM *= 2;

		BIGGER = (char *)realloc(T->DATA, ROOM);

		if (!BIGGER)
		{
			T->FAILED = 1;
			return ;
		}

		T->DATA = BIGGER;
		T->ROOM = ROOM;
	}

	memcpy(T->DATA + T->SIZE, DATA, SIZE);
	T->SIZE += SIZE;
	T->DATA[T->SIZE] = '\0';
}

static void
	ADDS(struct S_TEXT *T, const char *TEXT)
{
	ADD(T, TEXT, strlen(TEXT));
}

static void
	ADDF(struct S_TEXT *T, const char *FORMAT, int VALUE)
{
	char	LINE[64];

	snprintf(LINE, sizeof(LINE), FORMAT, VALUE);
	ADDS(T, LINE);
}

/* A JSON STRING, IN QUOTES */
static void
	ADD_JSON(struct S_TEXT *T, const char *TEXT)
{
	ADD(T, "\"", 1);

	for (; *TEXT; ++TEXT)
	{
		const unsigned char	C = (unsigned char)*TEXT;

		if (C == '"' || C == '\\')
		{
			ADD(T, "\\", 1);
			ADD(T, TEXT, 1);
		}
		else if (C < 0X20)
			ADDF(T, "\\u%04x", C);
		else
			ADD(T, TEXT, 1);
	}

	ADD(T, "\"", 1);
}

static void
	ADD_BASE64(struct S_TEXT *T, const unsigned char *DATA, size_t SIZE)
{
	static const char	DIGITS[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t				I;

	for (I = 0; I < SIZE; I += 3)
	{
		const unsigned long	A = DATA[I];
		unsigned long		B;
		unsigned long		C;
		char				FOUR[4];

		B = 0;
		C = 0;

		if (I + 1 < SIZE)
			B = DATA[I + 1];

		if (I + 2 < SIZE)
			C = DATA[I + 2];

		FOUR[0] = DIGITS[A >> 2];
		FOUR[1] = DIGITS[((A & 3) << 4) | (B >> 4)];
		FOUR[2] = DIGITS[((B & 15) << 2) | (C >> 6)];
		FOUR[3] = DIGITS[C & 63];

		if (I + 1 >= SIZE)
			FOUR[2] = '=';

		if (I + 2 >= SIZE)
			FOUR[3] = '=';

		ADD(T, FOUR, 4);
	}
}

/* BASE64 TO BYTES (OTHER CHARACTERS ARE SKIPPED). NULL = NO MEMORY */
static unsigned char
	*FROM_BASE64(const char *TEXT, size_t *SIZE)
{
	unsigned char	*DATA;
	unsigned long	BITS;
	int				HELD;

	DATA = (unsigned char *)malloc(strlen(TEXT) / 4 * 3 + 3);

	if (!DATA)
		return (NULL);

	*SIZE = 0;
	BITS = 0;
	HELD = 0;

	for (; *TEXT && *TEXT != '='; ++TEXT)
	{
		const char	C = *TEXT;
		int			V;

		if (C >= 'A' && C <= 'Z')
			V = C - 'A';
		else if (C >= 'a' && C <= 'z')
			V = C - 'a' + 26;
		else if (C >= '0' && C <= '9')
			V = C - '0' + 52;
		else if (C == '+' || C == '-')
			V = 62;
		else if (C == '/' || C == '_')
			V = 63;
		else
			continue ;

		BITS = (BITS << 6) | (unsigned long)V;
		HELD += 6;

		if (HELD >= 8)
		{
			HELD -= 8;
			DATA[(*SIZE)++] = (unsigned char)((BITS >> HELD) & 0XFF);
		}
	}

	return (DATA);
}

/* UTF-8 TO UTF-16. 0 = IT DOES NOT FIT */
static int
	TO_WIDE(const char *TEXT, WCHAR *OUTPUT, int ROOM)
{
	return (MultiByteToWideChar(CP_UTF8, 0, TEXT, -1, OUTPUT, ROOM) > 0);
}

static void
	ANSWER(const char *SEQ, struct S_TEXT *T)
{
	if (T->FAILED || !T->DATA)
		webview_return(&G__VIEW__, SEQ, 0, "{\"code\":5}");
	else
		webview_return(&G__VIEW__, SEQ, 0, T->DATA);

	free(T->DATA);
}

/* THE LANGUAGE TO READ AND SAVE IN: NULL IS TURKISH */
static const WCHAR
	*LANGUAGE(void)
{
	if (G__ENGLISH__)
		return (L"en-US");

	return (NULL);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                       SMALL THINGS                       |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        THE PHOTO                         |  \\//  */
/*   \/   #==========================================================#   \/   */
/*
# THE NEW PHOTO (OR WHY THERE IS NONE) TO THE PAGE, WITH A SMALL JPEG OF IT.
# NEW = 1: ANOTHER PHOTO (NOT A CROP OF THIS ONE), SO THE WHOLE ONE GOES.
*/
static void
	ANSWER_PHOTO(const char *SEQ, int CODE, struct S_PICTURE *P, int NEW)
{
	struct S_TEXT	T;
	unsigned char	*JPEG;
	size_t			SIZE;

	memset(&T, 0, sizeof(T));
	JPEG = NULL;

	if (CODE == PICCELL_DONE)
		JPEG = PICTURE_JPEG(P, PREVIEW_SIDE, &SIZE);

	if (CODE == PICCELL_DONE && !JPEG)
		CODE = PICCELL_NO_PICTURE;

	if (CODE != PICCELL_DONE)
	{
		PICTURE_FREE(P);
		ADDF(&T, "{\"code\":%d}", CODE);
		ANSWER(SEQ, &T);
		return ;
	}

	if (NEW)
		PICTURE_FREE(&G__WHOLE__);

	PICTURE_FREE(&G__PICTURE__);
	G__PICTURE__ = *P;
	ADDF(&T, "{\"code\":0,\"w\":%d", P->W);
	ADDF(&T, ",\"h\":%d,\"preview\":\"data:image/jpeg;base64,", P->H);
	ADD_BASE64(&T, JPEG, SIZE);
	ADDS(&T, "\"}");
	free(JPEG);
	ANSWER(SEQ, &T);
}

static void
	ON_OPEN(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_PICTURE	P;
	OPENFILENAMEW		DIALOG;
	WCHAR				PATH[PATH_MAX_W];
	const WCHAR			*TITLE;

	(void)ARGS;
	(void)DATA;

	if (G__BUSY__)
	{
		webview_return(W, SEQ, 0, "{\"cancel\":1}");
		return ;
	}

	TITLE = L"Tablonun fotoğrafını seçin";

	if (G__ENGLISH__)
		TITLE = L"Choose the photo of the table";

	PATH[0] = L'\0';
	memset(&DIALOG, 0, sizeof(DIALOG));
	DIALOG.lStructSize = sizeof(DIALOG);
	DIALOG.hwndOwner = W->priv.hwnd;
	DIALOG.lpstrFilter =
		L"Pictures / Resimler\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tif;*.tiff;"
		L"*.heic;*.webp\0All files / Tüm dosyalar\0*.*\0";
	DIALOG.lpstrFile = PATH;
	DIALOG.nMaxFile = PATH_MAX_W;
	DIALOG.lpstrTitle = TITLE;
	DIALOG.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (!GetOpenFileNameW(&DIALOG))
	{
		webview_return(W, SEQ, 0, "{\"cancel\":1}");
		return ;
	}

	ANSWER_PHOTO(SEQ, PICTURE_LOAD(PATH, &P), &P, 1);
}

static void
	ON_PASTE(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_PICTURE	P;

	(void)ARGS;
	(void)DATA;

	if (G__BUSY__)
	{
		webview_return(W, SEQ, 0, "{\"cancel\":1}");
		return ;
	}

	ANSWER_PHOTO(SEQ, PICTURE_LOAD(NULL, &P), &P, 1);
}

static void
	ON_DROP(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_PICTURE	P;
	wv_value			*LIST;
	unsigned char		*BYTES;
	const char			*TEXT;
	size_t				SIZE;
	int					CODE;

	(void)DATA;

	if (G__BUSY__)
	{
		webview_return(W, SEQ, 0, "{\"cancel\":1}");
		return ;
	}

	LIST = wv_parse(ARGS);
	TEXT = wv_string(wv_at(LIST, 0));
	BYTES = NULL;
	CODE = PICCELL_NO_PICTURE;
	memset(&P, 0, sizeof(P));

	if (TEXT)
		BYTES = FROM_BASE64(TEXT, &SIZE);

	if (BYTES)
		CODE = PICTURE_FROM_MEMORY(BYTES, SIZE, &P);

	free(BYTES);
	wv_free(LIST);
	ANSWER_PHOTO(SEQ, CODE, &P, 1);
}

/* ONE NUMBER OF THE PAGE (0 TO 1) TO A PIXEL OF A SIDE OF SIDE PIXELS */
static int
	PIXEL_OF(const wv_value *LIST, int INDEX, int SIDE)
{
	double	V;

	V = wv_number(wv_at(LIST, INDEX));

	if (V < 0.0)
		V = 0.0;

	if (V > 1.0)
		V = 1.0;

	return ((int)(V * SIDE + 0.5));
}

/* THE PART OF THE PHOTO THE USER DREW A BOX AROUND BECOMES THE PHOTO */
static void
	ON_CROP(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_PICTURE	PART;
	wv_value			*LIST;
	int					X0;
	int					Y0;
	int					X1;
	int					Y1;

	(void)DATA;

	if (!G__PICTURE__.BGRA || G__BUSY__)
	{
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	LIST = wv_parse(ARGS);
	X0 = PIXEL_OF(LIST, 0, G__PICTURE__.W);
	Y0 = PIXEL_OF(LIST, 1, G__PICTURE__.H);
	X1 = PIXEL_OF(LIST, 2, G__PICTURE__.W);
	Y1 = PIXEL_OF(LIST, 3, G__PICTURE__.H);
	wv_free(LIST);

	/* TOO SMALL TO HOLD A TABLE: NOTHING CHANGES */
	if (
		X1 - X0 < 16 ||
		Y1 - Y0 < 16 ||
		!PICTURE_CROP(&G__PICTURE__, X0, Y0, X1 - X0, Y1 - Y0, &PART)
	)
	{
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	/* THE FIRST CROP KEEPS THE WHOLE PHOTO, SO IT CAN COME BACK */
	if (!G__WHOLE__.BGRA)
	{
		G__WHOLE__ = G__PICTURE__;
		memset(&G__PICTURE__, 0, sizeof(G__PICTURE__));
	}

	ANSWER_PHOTO(SEQ, PICCELL_DONE, &PART, 0);
}

static void
	ON_WHOLE(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_PICTURE	P;

	(void)ARGS;
	(void)DATA;

	if (!G__WHOLE__.BGRA || G__BUSY__)
	{
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	P = G__WHOLE__;
	memset(&G__WHOLE__, 0, sizeof(G__WHOLE__));
	ANSWER_PHOTO(SEQ, PICCELL_DONE, &P, 0);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        THE PHOTO                         |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         READING                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/* {code, used, rows: [[...]], check: [[0, 1...]]} */
static void
	TABLE_JSON(struct S_TEXT *T, const struct S_TABLE *TABLE, int USED)
{
	int	R;
	int	C;

	ADDS(T, "{\"code\":0,\"used\":");

	if (USED == CONVERT_HAND)
		ADDS(T, "\"hand\",\"rows\":[");
	else
		ADDS(T, "\"print\",\"rows\":[");

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		if (R)
			ADDS(T, ",");

		ADDS(T, "[");

		for (C = 0; C < TABLE->COLS; ++C)
		{
			const char	*CELL = TABLE->CELL[R * TABLE->COLS + C];

			if (C)
				ADDS(T, ",");

			if (CELL)
				ADD_JSON(T, CELL);
			else
				ADDS(T, "\"\"");
		}

		ADDS(T, "]");
	}

	ADDS(T, "],\"check\":[");

	for (R = 0; R < TABLE->ROWS && TABLE->CHECK; ++R)
	{
		if (R)
			ADDS(T, ",");

		ADDS(T, "[");

		for (C = 0; C < TABLE->COLS; ++C)
		{
			if (C)
				ADDS(T, ",");

			ADDF(T, "%d", TABLE->CHECK[R * TABLE->COLS + C]);
		}

		ADDS(T, "]");
	}

	ADDS(T, "]}");
}

static DWORD WINAPI
	READ_THREAD(LPVOID PARAMETER)
{
	struct S_JOB	*JOB;
	struct S_TABLE	TABLE;
	struct S_TEXT	T;
	int				USED;
	int				CODE;

	JOB = (struct S_JOB *)PARAMETER;
	memset(&T, 0, sizeof(T));
	CODE = CONVERT(&G__PICTURE__, JOB->MODE, LANGUAGE(), NULL, &TABLE, &USED);

	if (CODE == PICCELL_DONE)
		TABLE_JSON(&T, &TABLE, USED);
	else
		ADDF(&T, "{\"code\":%d}", CODE);

	LAYOUT_FREE(&TABLE);
	ANSWER(JOB->SEQ, &T);
	free(JOB);
	InterlockedExchange(&G__BUSY__, 0);
	return (0);
}

static void
	ON_READ(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_JOB	*JOB;
	wv_value		*LIST;
	const char		*MODE;
	HANDLE			THREAD;

	(void)DATA;

	if (!G__PICTURE__.BGRA || InterlockedExchange(&G__BUSY__, 1))
	{
		webview_return(W, SEQ, 0, "{\"code\":2}");
		return ;
	}

	JOB = (struct S_JOB *)calloc(1, sizeof(*JOB));

	if (!JOB)
	{
		InterlockedExchange(&G__BUSY__, 0);
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	snprintf(JOB->SEQ, sizeof(JOB->SEQ), "%s", SEQ);
	LIST = wv_parse(ARGS);
	MODE = wv_string(wv_at(LIST, 0));
	JOB->MODE = CONVERT_AUTO;

	if (MODE && !strcmp(MODE, "print"))
		JOB->MODE = CONVERT_PRINT;

	if (MODE && !strcmp(MODE, "hand"))
		JOB->MODE = CONVERT_HAND;

	wv_free(LIST);
	THREAD = CreateThread(NULL, 0, READ_THREAD, JOB, 0, NULL);

	if (!THREAD)
	{
		free(JOB);
		InterlockedExchange(&G__BUSY__, 0);
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	CloseHandle(THREAD);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         READING                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          SAVING                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE ROWS THE PAGE SENT ([["A", "B"], ...]) TO A TABLE. 1 = DONE */
static int
	TABLE_OF(const wv_value *ROWS, struct S_TABLE *TABLE)
{
	int	R;

	memset(TABLE, 0, sizeof(*TABLE));
	TABLE->ROWS = wv_count(ROWS);

	for (R = 0; R < TABLE->ROWS; ++R)
		if (wv_count(wv_at(ROWS, R)) > TABLE->COLS)
			TABLE->COLS = wv_count(wv_at(ROWS, R));

	if (TABLE->ROWS < 1 || TABLE->COLS < 1)
		return (0);

	TABLE->CELL = (char **)calloc(
		(size_t)TABLE->ROWS * TABLE->COLS,
		sizeof(char *)
	);

	if (!TABLE->CELL)
		return (0);

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		const wv_value	*ROW = wv_at(ROWS, R);
		int				C;

		for (C = 0; C < wv_count(ROW); ++C)
		{
			const char	*TEXT = wv_string(wv_at(ROW, C));
			char		*COPY;

			if (!TEXT || !TEXT[0])
				continue ;

			COPY = (char *)malloc(strlen(TEXT) + 1);

			if (COPY)
				strcpy(COPY, TEXT);

			TABLE->CELL[R * TABLE->COLS + C] = COPY;
		}
	}

	return (1);
}

/* WHERE TO SAVE: A SAVE DIALOG. 0 = CANCELLED */
static int
	ASK_PATH(const char *KIND, WCHAR *PATH)
{
	OPENFILENAMEW	DIALOG;
	SYSTEMTIME		NOW;
	const WCHAR		*FILTER;
	const WCHAR		*EXT;

	FILTER = L"Excel (*.xlsx)\0*.xlsx\0";
	EXT = L"xlsx";

	if (!strcmp(KIND, "csv"))
	{
		FILTER = L"CSV (*.csv)\0*.csv\0";
		EXT = L"csv";
	}
	else if (!strcmp(KIND, "pdf"))
	{
		FILTER = L"PDF (*.pdf)\0*.pdf\0";
		EXT = L"pdf";
	}

	GetLocalTime(&NOW);
	_snwprintf(
		PATH,
		PATH_MAX_W,
		L"PICCELL %04d-%02d-%02d %02d.%02d.%ls",
		NOW.wYear,
		NOW.wMonth,
		NOW.wDay,
		NOW.wHour,
		NOW.wMinute,
		EXT
	);
	PATH[PATH_MAX_W - 1] = L'\0';
	memset(&DIALOG, 0, sizeof(DIALOG));
	DIALOG.lStructSize = sizeof(DIALOG);
	DIALOG.hwndOwner = G__VIEW__.priv.hwnd;
	DIALOG.lpstrFilter = FILTER;
	DIALOG.lpstrFile = PATH;
	DIALOG.nMaxFile = PATH_MAX_W;
	DIALOG.lpstrDefExt = EXT;
	DIALOG.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	return (GetSaveFileNameW(&DIALOG) != 0);
}

static void
	ON_SAVE(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_TABLE	TABLE;
	struct S_TEXT	T;
	wv_value		*LIST;
	const char		*KIND;
	WCHAR			PATH[PATH_MAX_W];
	char			NAME[PATH_MAX_W * 3];
	int				CODE;

	(void)DATA;
	LIST = wv_parse(ARGS);
	KIND = wv_string(wv_at(LIST, 0));

	if (!KIND || !TABLE_OF(wv_at(LIST, 1), &TABLE))
	{
		wv_free(LIST);
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	if (!ASK_PATH(KIND, PATH))
	{
		LAYOUT_FREE(&TABLE);
		wv_free(LIST);
		webview_return(W, SEQ, 0, "{\"cancel\":1}");
		return ;
	}

	CODE = CONVERT_SAVE(&TABLE, PATH, LANGUAGE());
	LAYOUT_FREE(&TABLE);
	wv_free(LIST);
	memset(&T, 0, sizeof(T));
	ADDF(&T, "{\"code\":%d,\"path\":", CODE);

	if (
		!WideCharToMultiByte(
			CP_UTF8,
			0,
			PATH,
			-1,
			NAME,
			(int)sizeof(NAME),
			NULL,
			NULL
		)
	)
		NAME[0] = '\0';

	ADD_JSON(&T, NAME);
	ADDS(&T, "}");
	ANSWER(SEQ, &T);
}

/* TAB SEPARATED TEXT ON THE CLIPBOARD: EXCEL PASTES IT INTO CELLS */
static void
	ON_COPY(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	struct S_TABLE	TABLE;
	struct S_TEXT	T;
	wv_value		*LIST;
	HGLOBAL			MEMORY;
	int				CHARS;
	int				R;

	(void)DATA;
	LIST = wv_parse(ARGS);
	memset(&T, 0, sizeof(T));

	if (TABLE_OF(wv_at(LIST, 0), &TABLE))
	{
		for (R = 0; R < TABLE.ROWS; ++R)
		{
			int	C;

			for (C = 0; C < TABLE.COLS; ++C)
			{
				const char	*CELL = TABLE.CELL[R * TABLE.COLS + C];

				if (C)
					ADDS(&T, "\t");

				if (CELL)
					ADDS(&T, CELL);
			}

			ADDS(&T, "\r\n");
		}

		LAYOUT_FREE(&TABLE);
	}

	wv_free(LIST);
	CHARS = 0;
	MEMORY = NULL;

	if (T.DATA && !T.FAILED)
		CHARS = MultiByteToWideChar(CP_UTF8, 0, T.DATA, -1, NULL, 0);

	if (CHARS > 0)
		MEMORY = GlobalAlloc(GMEM_MOVEABLE, (size_t)CHARS * sizeof(WCHAR));

	if (MEMORY)
	{
		WCHAR	*WIDE = (WCHAR *)GlobalLock(MEMORY);

		if (WIDE)
		{
			MultiByteToWideChar(CP_UTF8, 0, T.DATA, -1, WIDE, CHARS);
			GlobalUnlock(MEMORY);
		}

		if (WIDE && OpenClipboard(W->priv.hwnd))
		{
			EmptyClipboard();

			if (SetClipboardData(CF_UNICODETEXT, MEMORY))
				MEMORY = NULL; /* THE CLIPBOARD OWNS IT NOW */

			CloseClipboard();
		}

		if (MEMORY)
			GlobalFree(MEMORY);
	}

	free(T.DATA);
	webview_return(W, SEQ, 0, "{\"code\":0}");
}

/* THE CELLS THE USER FIXED: THEIR WORDS ARE KNOWN FROM NOW ON */
static void
	ON_LEARN(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	wv_value	*LIST;
	wv_value	*TEXTS;
	int			I;

	(void)DATA;
	LIST = wv_parse(ARGS);
	TEXTS = wv_at(LIST, 0);

	for (I = 0; I < wv_count(TEXTS); ++I)
	{
		const char	*TEXT = wv_string(wv_at(TEXTS, I));

		if (TEXT)
			CONVERT_LEARN(TEXT);
	}

	wv_free(LIST);
	webview_return(W, SEQ, 0, "{\"code\":0}");
}

static void
	ON_SHOW(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	wv_value	*LIST;
	const char	*PATH;
	WCHAR		WIDE[PATH_MAX_W];

	(void)DATA;
	LIST = wv_parse(ARGS);
	PATH = wv_string(wv_at(LIST, 0));

	if (PATH && TO_WIDE(PATH, WIDE, PATH_MAX_W))
		ShellExecuteW(W->priv.hwnd, L"open", WIDE, NULL, NULL, SW_SHOWNORMAL);

	wv_free(LIST);
	webview_return(W, SEQ, 0, "{\"code\":0}");
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          SAVING                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        THE WINDOW                        |  \\//  */
/*   \/   #==========================================================#   \/   */
/* 1 = WINDOWS 10 OR NEWER, WHERE THE OCR OF WINDOWS IS */
static int
	HAS_OCR(void)
{
	typedef LONG	(WINAPI *T_GET_VERSION)(OSVERSIONINFOW *);
	T_GET_VERSION	GET_VERSION;
	OSVERSIONINFOW	INFO;
	HMODULE			NTDLL;

	NTDLL = GetModuleHandleW(L"ntdll.dll");

	if (!NTDLL)
		return (0);

	GET_VERSION = (T_GET_VERSION)(void (*)(void))GetProcAddress(
		NTDLL,
		"RtlGetVersion"
	);
	memset(&INFO, 0, sizeof(INFO));
	INFO.dwOSVersionInfoSize = sizeof(INFO);

	if (!GET_VERSION || GET_VERSION(&INFO) != 0)
		return (0);

	return (INFO.dwMajorVersion >= 10);
}

static void
	ON_INFO(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	char		ANSWER_TEXT[128];
	const char	*LANG;

	(void)ARGS;
	(void)DATA;
	LANG = "tr";

	if (G__ENGLISH__)
		LANG = "en";

	snprintf(
		ANSWER_TEXT,
		sizeof(ANSWER_TEXT),
		"{\"lang\":\"%s\",\"ocr\":%d,\"reader\":%d,"
		"\"version\":\"%s\",\"updated\":%d}",
		LANG,
		HAS_OCR(),
		CONVERT_START(NULL),
		PICCELL_VERSION_TEXT,
		G__UPDATED__
	);
	webview_return(W, SEQ, 0, ANSWER_TEXT);
}

static void
	ON_LANGUAGE(
		struct webview *W,
		const char *SEQ,
		const char *ARGS,
		void *DATA
	)
{
	wv_value	*LIST;
	const char	*NAME;

	(void)DATA;
	LIST = wv_parse(ARGS);
	NAME = wv_string(wv_at(LIST, 0));
	G__ENGLISH__ = NAME && !strcmp(NAME, "en");
	wv_free(LIST);
	webview_return(W, SEQ, 0, "{\"code\":0}");
}

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                         UPDATES                          |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE CHECK AND THE DOWNLOAD (SECONDS), THEN THE ANSWER TO PICCELL.UPDATE() */
static DWORD WINAPI
	UPDATE_THREAD(LPVOID PARAMETER)
{
	char	*SEQ;
	char	TAG[64];
	char	SAFE[64];
	char	ANSWER_TEXT[128];
	int		I;
	int		N;

	SEQ = (char *)PARAMETER;
	N = 0;

	if (UPDATE_FIND(TAG, sizeof(TAG)) == UPDATE_READY)
	{
		/* ONLY THE LETTERS A TAG NEEDS, SO IT CAN NOT BREAK THE ANSWER */
		for (I = 0; TAG[I] && N + 1 < (int)sizeof(SAFE); ++I)
			if (
				(TAG[I] >= '0' && TAG[I] <= '9') ||
				(TAG[I] >= 'A' && TAG[I] <= 'Z') ||
				(TAG[I] >= 'a' && TAG[I] <= 'z') ||
				TAG[I] == '.' ||
				TAG[I] == '-' ||
				TAG[I] == '_'
			)
				SAFE[N++] = TAG[I];
	}

	SAFE[N] = '\0';

	if (N)
		snprintf(
			ANSWER_TEXT,
			sizeof(ANSWER_TEXT),
			"{\"ready\":1,\"tag\":\"%s\"}",
			SAFE
		);
	else
		snprintf(ANSWER_TEXT, sizeof(ANSWER_TEXT), "{\"ready\":0}");

	webview_return(&G__VIEW__, SEQ, 0, ANSWER_TEXT);
	free(SEQ);
	return (0);
}

/* {"ready":1,"tag":"20.26.11.25"} ONCE THE NEW EXE IS HERE, ELSE READY 0 */
static void
	ON_UPDATE(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	char			*COPY;
	HANDLE			THREAD;
	const size_t	SIZE = strlen(SEQ) + 1;

	(void)ARGS;
	(void)DATA;

	if (!G__UPDATES__ || InterlockedExchange(&G__ASKED__, 1))
	{
		webview_return(W, SEQ, 0, "{\"ready\":0}");
		return ;
	}

	COPY = (char *)malloc(SIZE);

	if (!COPY)
	{
		webview_return(W, SEQ, 0, "{\"ready\":0}");
		return ;
	}

	memcpy(COPY, SEQ, SIZE);
	THREAD = CreateThread(NULL, 0, UPDATE_THREAD, COPY, 0, NULL);

	if (!THREAD)
	{
		free(COPY);
		webview_return(W, SEQ, 0, "{\"ready\":0}");
		return ;
	}

	CloseHandle(THREAD);
}

/* "RESTART NOW": THE NEW EXE TAKES OVER AND OPENS THE WINDOW AGAIN */
static void
	ON_RESTART(struct webview *W, const char *SEQ, const char *ARGS, void *DATA)
{
	(void)ARGS;
	(void)DATA;

	if (!UPDATE_APPLY(1))
	{
		webview_return(W, SEQ, 0, "{\"code\":5}");
		return ;
	}

	webview_return(W, SEQ, 0, "{\"code\":0}");
	webview_terminate(W);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                         UPDATES                          |  //\\  */
/* '""""' #==========================================================# '""""' */

/* THE PAGE FROM THE "UI" RESOURCE, ENDING WITH 0. NULL = NONE */
static char
	*LOAD_PAGE(void)
{
	HRSRC		FOUND;
	HGLOBAL		HANDLE;
	const void	*BYTES;
	char		*PAGE;
	DWORD		SIZE;

	FOUND = FindResourceW(NULL, L"UI", (LPCWSTR)RT_RCDATA);
	HANDLE = NULL;
	BYTES = NULL;

	if (FOUND)
		HANDLE = LoadResource(NULL, FOUND);

	if (HANDLE)
		BYTES = LockResource(HANDLE);

	if (!BYTES)
		return (NULL);

	SIZE = SizeofResource(NULL, FOUND);
	PAGE = (char *)malloc((size_t)SIZE + 1);

	if (!PAGE)
		return (NULL);

	memcpy(PAGE, BYTES, SIZE);
	PAGE[SIZE] = '\0';
	return (PAGE);
}

int
	UI_RUN(const WCHAR *WORD_FILE, int UPDATES, int UPDATED)
{
	char	*PAGE;

	G__UPDATES__ = UPDATES;
	G__UPDATED__ = UPDATED;
	CONVERT_START(WORD_FILE);
	G__ENGLISH__ = PRIMARYLANGID(GetUserDefaultUILanguage()) != LANG_TURKISH;
	PAGE = LOAD_PAGE();

	if (!PAGE)
	{
		MessageBoxW(NULL, L"PICCELL: no page.", L"PICCELL", MB_ICONERROR);
		return (PICCELL_USAGE);
	}

	webview_set_app_name("PICCELL");
	memset(&G__VIEW__, 0, sizeof(G__VIEW__));
	G__VIEW__.title = "PICCELL";
	G__VIEW__.url = "about:blank";
	G__VIEW__.width = 1180;
	G__VIEW__.height = 760;
	G__VIEW__.resizable = 1;
	G__VIEW__.window.center = 1;
	webview_bind(&G__VIEW__, "piccell.info", ON_INFO, NULL);
	webview_bind(&G__VIEW__, "piccell.open", ON_OPEN, NULL);
	webview_bind(&G__VIEW__, "piccell.paste", ON_PASTE, NULL);
	webview_bind(&G__VIEW__, "piccell.drop", ON_DROP, NULL);
	webview_bind(&G__VIEW__, "piccell.crop", ON_CROP, NULL);
	webview_bind(&G__VIEW__, "piccell.whole", ON_WHOLE, NULL);
	webview_bind(&G__VIEW__, "piccell.read", ON_READ, NULL);
	webview_bind(&G__VIEW__, "piccell.save", ON_SAVE, NULL);
	webview_bind(&G__VIEW__, "piccell.copy", ON_COPY, NULL);
	webview_bind(&G__VIEW__, "piccell.show", ON_SHOW, NULL);
	webview_bind(&G__VIEW__, "piccell.learn", ON_LEARN, NULL);
	webview_bind(&G__VIEW__, "piccell.language", ON_LANGUAGE, NULL);
	webview_bind(&G__VIEW__, "piccell.update", ON_UPDATE, NULL);
	webview_bind(&G__VIEW__, "piccell.restart", ON_RESTART, NULL);

	if (webview_init(&G__VIEW__) != 0)
	{
		free(PAGE);
		MessageBoxW(
			NULL,
			L"PICCELL: the window could not be opened.",
			L"PICCELL",
			MB_ICONERROR
		);
		return (PICCELL_USAGE);
	}

	webview_set_html(&G__VIEW__, PAGE);

	while (webview_loop(&G__VIEW__, 1) == 0)
	{}

	/* A READ OR A DOWNLOAD STILL RUNNING ENDS WITH THE PROCESS */
	webview_exit(&G__VIEW__);
	UPDATE_APPLY(0); /* A NEW EXE THAT IS READY TAKES THIS ONE'S PLACE */
	free(PAGE);
	PICTURE_FREE(&G__PICTURE__);
	PICTURE_FREE(&G__WHOLE__);
	return (PICCELL_DONE);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        THE WINDOW                        |  //\\  */
/* '""""' #==========================================================# '""""' */
