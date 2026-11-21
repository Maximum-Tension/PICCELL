/******************************************************************************\
# C - convert                                    #       Maximum Tension       #
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

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define CP_UTF8
 * #define INVALID_FILE_ATTRIBUTES
 * #define MAX_PATH
 * #define RT_RCDATA
 * #define TRUE
 * typedef	DWORD;
 * typedef	HGLOBAL;
 * typedef	HRSRC;
 * typedef	LPCWSTR;
 * typedef	WCHAR;
 * BOOL	CopyFileW(LPCWSTR, LPCWSTR, BOOL);
 * BOOL	CreateDirectoryW(LPCWSTR, LPSECURITY_ATTRIBUTES);
 * HRSRC	FindResourceW(HMODULE, LPCWSTR, LPCWSTR);
 * DWORD	GetEnvironmentVariableW(LPCWSTR, LPWSTR, DWORD);
 * DWORD	GetFileAttributesW(LPCWSTR);
 * DWORD	GetModuleFileNameW(HMODULE, LPWSTR, DWORD);
 * HGLOBAL	LoadResource(HMODULE, HRSRC);
 * LPVOID	LockResource(HGLOBAL);
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
#include <stdio.h>
/******* """""""""
 * #define SEEK_END
 * #define SEEK_SET
 * typedef	FILE;
 * FILE		*_wfopen(const wchar_t *, const wchar_t *);
 * int		fclose(FILE *);
 * int		fprintf(FILE *, const char *, ...);
 * size_t	fread(void *, size_t, size_t, FILE *);
 * int		fseek(FILE *, long, int);
 * long		ftell(FILE *);
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
 * int		_wcsnicmp(const wchar_t *, const wchar_t *, size_t);
 * void		*memset(void *, int, size_t);
 * int		strncmp(const char *, const char *, size_t);
 * wchar_t	*wcscat(wchar_t *, const wchar_t *);
 * wchar_t	*wcscpy(wchar_t *, const wchar_t *);
 */
#include "piccell.h"
/******* """""""""
 * #define PICCELL_DONE
 * #define PICCELL_NO_OCR
 * #define PICCELL_NO_READER
 * #define PICCELL_NO_TEXT
 * #define PICCELL_NO_WRITE
 * struct S_WORD;
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 * int	LAYOUT_BUILD(const struct S_WORD *, int, struct S_TABLE *);
 */
#include "picture.h"
/******* """""""""""
 * struct S_PICTURE;
 * unsigned char	*PICTURE_GRAY(const struct S_PICTURE *);
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
#include "reader.h"
/******* """"""""""
 * struct S_READER;
 * struct S_READER	*READER_LOAD(const void *, size_t);
 */
#include "hand.h"
/******* """"""""
 * struct S_HAND;
 * void				HAND_ADD_WORDS(const char *);
 * void				HAND_CLOSE(struct S_HAND *);
 * double			HAND_JUDGE(struct S_HAND *, const struct S_WORD *, int);
 * struct S_HAND	*HAND_OPEN(const unsigned char *, int, int);
 * void				HAND_SET_READER(struct S_READER *);
 * int				HAND_TABLE(struct S_HAND *, struct S_TABLE *);
 */
#include "export.h"
/******* """"""""""
 * int	EXPORT_KIND(const char *);
 * int	EXPORT_WRITE(const struct S_TABLE *, int, int, FILE *);
 */
#include "convert.h"
/******* """""""""""
 * #define CONVERT_AUTO
 * #define CONVERT_HAND
 * #define CONVERT_PRINT
 */
/* **************************** [^] INCLUDES [^] **************************** */

/*
# THE OCR READ THE PAGE WELL WHEN EVERY CHARACTER IT SAW COSTS THE READER
# LESS THAN THIS (LOG). A HANDWRITTEN PAGE READ BY THE OCR COSTS FAR MORE.
*/
#define PRINTED_COST 1.0

static struct S_READER	*G__READER__;
static int				G__LEARNED__; /* 1 = THE LEARNED WORDS WERE LOADED */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                          FILES                           |  \\//  */
/*   \/   #==========================================================#   \/   */
/* A WHOLE FILE, ENDING WITH 0. NULL WHEN IT CAN NOT BE READ */
static char
	*READ_ALL(const WCHAR *PATH, size_t *SIZE)
{
	FILE	*F;
	char	*DATA;
	long	LEN;

	F = _wfopen(PATH, L"rb");

	if (!F)
		return (NULL);

	DATA = NULL;

	if (fseek(F, 0, SEEK_END) == 0 && (LEN = ftell(F)) >= 0)
	{
		DATA = (char *)malloc((size_t)LEN + 1);

		if (
			DATA &&
			(
				fseek(F, 0, SEEK_SET) ||
				fread(DATA, 1, (size_t)LEN, F) != (size_t)LEN
			)
		)
		{
			free(DATA);
			DATA = NULL;
		}
	}

	fclose(F);

	if (!DATA)
		return (NULL);

	DATA[LEN] = '\0';
	*SIZE = (size_t)LEN;
	return (DATA);
}

/* THE READER FROM THE "READER" RESOURCE, OR READER.BIN NEXT TO THE EXE */
static struct S_READER
	*LOAD_READER(void)
{
	struct S_READER	*R;
	WCHAR			PATH[MAX_PATH + 16];
	HRSRC			FOUND;
	char			*DATA;
	size_t			SIZE;
	DWORD			LEN;

	FOUND = FindResourceW(NULL, L"READER", (LPCWSTR)RT_RCDATA);

	if (FOUND)
	{
		HGLOBAL		HANDLE = LoadResource(NULL, FOUND);
		const void	*BYTES = NULL;

		if (HANDLE)
			BYTES = LockResource(HANDLE);

		if (BYTES)
			return (READER_LOAD(BYTES, SizeofResource(NULL, FOUND)));
	}

	LEN = GetModuleFileNameW(NULL, PATH, MAX_PATH);

	if (!LEN || LEN >= MAX_PATH)
		return (NULL);

	while (LEN && PATH[LEN - 1] != L'\\' && PATH[LEN - 1] != L'/')
		--LEN;

	wcscpy(PATH + LEN, L"reader.bin");
	DATA = READ_ALL(PATH, &SIZE);

	if (!DATA)
		return (NULL);

	R = READER_LOAD(DATA, SIZE);
	free(DATA);
	return (R);
}

/*
# %APPDATA%\PICCELL\WORDS.TXT: THE WORDS THE USER FIXED IN THE WINDOW, SO
# THE NEXT PAGES READ THEM RIGHT. MAKE: 1 = THE FOLDER IS MADE WHEN MISSING.
# 0 = NO SUCH PLACE.
*/
static int
	LEARNED_PATH(WCHAR *PATH, int MAKE)
{
	DWORD	LEN;

	LEN = GetEnvironmentVariableW(L"APPDATA", PATH, MAX_PATH);

	if (!LEN || LEN >= MAX_PATH - 32)
		return (0);

	wcscpy(PATH + LEN, L"\\PICCELL");

	if (MAKE)
		CreateDirectoryW(PATH, NULL);

	wcscat(PATH, L"\\words.txt");
	return (1);
}

/*
# PICCELL WAS CALLED PICELL, AND TABLO BEFORE THAT. THE WORDS ONE OF THEM
# LEARNED (%APPDATA%\PICELL\WORDS.TXT, THEN %APPDATA%\TABLO\WORDS.TXT) ARE
# COPIED OVER ONCE, WHILE PICCELL HAS NONE OF ITS OWN. THE OLD FILE STAYS.
*/
static void
	OLD_WORDS(void)
{
	static const WCHAR	*const OLD_NAMES[2] = {
		L"\\PICELL\\words.txt",
		L"\\TABLO\\words.txt"
	};
	WCHAR				OLD[MAX_PATH];
	WCHAR				PATH[MAX_PATH];
	DWORD				LEN;
	int					I;

	LEN = GetEnvironmentVariableW(L"APPDATA", OLD, MAX_PATH);

	if (
		!LEN ||
		LEN >= MAX_PATH - 32 ||
		!LEARNED_PATH(PATH, 0) ||
		GetFileAttributesW(PATH) != INVALID_FILE_ATTRIBUTES
	)
		return ;

	for (I = 0; I < 2; ++I)
	{
		wcscpy(OLD + LEN, OLD_NAMES[I]);

		if (GetFileAttributesW(OLD) == INVALID_FILE_ATTRIBUTES)
			continue ;

		if (LEARNED_PATH(PATH, 1))
			CopyFileW(OLD, PATH, TRUE);

		return ;
	}
}

/* WHAT THE OCR SAW, ONE WORD PER LINE: LINE, X, Y, W, H, TEXT */
static void
	WRITE_WORDS(const WCHAR *PATH, const struct S_WORD *WORDS, int COUNT)
{
	FILE	*OUTPUT;
	int		I;

	OUTPUT = _wfopen(PATH, L"wb");

	if (!OUTPUT)
		return ;

	for (I = 0; I < COUNT; ++I)
		fprintf(
			OUTPUT,
			"%d\t%.1f\t%.1f\t%.1f\t%.1f\t%s\n",
			WORDS[I].LINE,
			WORDS[I].X,
			WORDS[I].Y,
			WORDS[I].W,
			WORDS[I].H,
			WORDS[I].TEXT
		);

	fclose(OUTPUT);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                          FILES                           |  //\\  */
/* '""""' #==========================================================# '""""' */

/* .,,,,. #==========================================================# .,,,,. */
/*  \\//  |                        PUBLIC API                        |  \\//  */
/*   \/   #==========================================================#   \/   */
/* THE KNOWN WORDS OF A FILE (UTF-8, A BOM AT ITS START IS SKIPPED) */
static void
	ADD_WORD_FILE(const WCHAR *PATH)
{
	size_t	SIZE;
	char	*TEXT;

	TEXT = READ_ALL(PATH, &SIZE);

	if (TEXT && strncmp(TEXT, "\xEF\xBB\xBF", 3) == 0)
		HAND_ADD_WORDS(TEXT + 3);
	else if (TEXT)
		HAND_ADD_WORDS(TEXT);

	free(TEXT);
}

int
	CONVERT_START(const WCHAR *WORD_FILE)
{
	WCHAR	PATH[MAX_PATH];

	if (!G__READER__)
	{
		G__READER__ = LOAD_READER();
		HAND_SET_READER(G__READER__);
	}

	if (!G__LEARNED__)
	{
		OLD_WORDS();

		if (LEARNED_PATH(PATH, 0))
			ADD_WORD_FILE(PATH);
	}

	G__LEARNED__ = 1;

	if (WORD_FILE)
		ADD_WORD_FILE(WORD_FILE);

	return (G__READER__ != NULL);
}

/*
# 1 = A TEXT WORTH LEARNING: 2 TO 40 BYTES, SOME LETTERS, NO DIGITS, NO
# TAB OR NEW LINE.
*/
static int
	WORTH_LEARNING(const char *TEXT)
{
	const size_t	LEN = strlen(TEXT);
	int				LETTERS;

	if (LEN < 2 || LEN > 40 || strpbrk(TEXT, "0123456789\t\r\n"))
		return (0);

	LETTERS = 0;

	for (; *TEXT; ++TEXT)
	{
		const unsigned char	C = (unsigned char)*TEXT;

		LETTERS += (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z');
		LETTERS += C >= 0XC0;
	}

	return (LETTERS >= 2);
}

int
	CONVERT_LEARN(const char *TEXT)
{
	WCHAR	PATH[MAX_PATH];
	FILE	*F;
	char	WORD[64];
	size_t	LEN;

	while (*TEXT == ' ')
		++TEXT;

	LEN = strlen(TEXT);

	while (LEN && TEXT[LEN - 1] == ' ')
		--LEN;

	if (LEN >= sizeof(WORD))
		return (0);

	memcpy(WORD, TEXT, LEN);
	WORD[LEN] = '\0';

	if (!WORTH_LEARNING(WORD) || HAND_KNOWN(WORD) || !LEARNED_PATH(PATH, 1))
		return (0);

	F = _wfopen(PATH, L"ab");

	if (!F)
		return (0);

	fprintf(F, "%s\r\n", WORD);
	fclose(F);
	HAND_ADD_WORDS(WORD);
	return (1);
}

int
	CONVERT_TURKISH(const WCHAR *LANGUAGE)
{
	return (!LANGUAGE || _wcsnicmp(LANGUAGE, L"en", 2) != 0);
}

int
	CONVERT(
		const struct S_PICTURE *PICTURE,
		int MODE,
		const WCHAR *LANGUAGE,
		const WCHAR *DUMP,
		struct S_TABLE *TABLE,
		int *USED
	)
{
	struct S_WORD	*WORDS;
	struct S_HAND	*HAND;
	unsigned char	*GRAY;
	int				COUNT;
	int				CODE;

	memset(TABLE, 0, sizeof(*TABLE));
	WORDS = NULL;
	COUNT = 0;
	CODE = PICCELL_NO_OCR;
	*USED = CONVERT_PRINT;

	if (MODE != CONVERT_HAND)
		CODE = OCR_READ(PICTURE, LANGUAGE, &WORDS, &COUNT);

	if (DUMP && CODE == PICCELL_DONE)
		WRITE_WORDS(DUMP, WORDS, COUNT);

	if (MODE == CONVERT_PRINT || (CODE == PICCELL_DONE && !G__READER__))
	{
		if (CODE == PICCELL_DONE && !LAYOUT_BUILD(WORDS, COUNT, TABLE))
			CODE = PICCELL_NO_TEXT;

		free(WORDS);
		return (CODE);
	}

	if (!G__READER__)
		return (PICCELL_NO_READER);

	GRAY = PICTURE_GRAY(PICTURE);
	HAND = NULL;

	if (GRAY)
		HAND = HAND_OPEN(GRAY, PICTURE->W, PICTURE->H);

	free(GRAY);

	/* AUTO: WHAT THE OCR SAW, JUDGED BY THE READER */
	if (MODE == CONVERT_AUTO && CODE == PICCELL_DONE)
	{
		double	COST;

		COST = -1.0;

		if (HAND)
			COST = HAND_JUDGE(HAND, WORDS, COUNT);

		if (COST < PRINTED_COST && LAYOUT_BUILD(WORDS, COUNT, TABLE))
		{
			HAND_CLOSE(HAND);
			free(WORDS);
			return (PICCELL_DONE);
		}
	}

	free(WORDS);
	*USED = CONVERT_HAND;
	CODE = PICCELL_NO_TEXT;

	if (HAND && HAND_TABLE(HAND, TABLE))
		CODE = PICCELL_DONE;

	HAND_CLOSE(HAND);
	return (CODE);
}

int
	CONVERT_SAVE(
		const struct S_TABLE *TABLE,
		const WCHAR *PATH,
		const WCHAR *LANGUAGE
	)
{
	char	NAME[MAX_PATH * 3];
	FILE	*OUTPUT;
	int		CODE;

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

	OUTPUT = _wfopen(PATH, L"wb");

	if (!OUTPUT)
		return (PICCELL_NO_WRITE);

	CODE = PICCELL_NO_WRITE;

	if (
		EXPORT_WRITE(
			TABLE,
			EXPORT_KIND(NAME),
			CONVERT_TURKISH(LANGUAGE),
			OUTPUT
		)
	)
		CODE = PICCELL_DONE;

	if (fclose(OUTPUT))
		CODE = PICCELL_NO_WRITE;

	return (CODE);
}
/*   /\   #==========================================================#   /\   */
/*  //\\  |                        PUBLIC API                        |  //\\  */
/* '""""' #==========================================================# '""""' */
