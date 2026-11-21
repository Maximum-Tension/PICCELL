/******************************************************************************\
# C - main                                       #       Maximum Tension       #
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
# PICCELL.EXE: A PHOTO OF A TABLE, PRINTED OR HANDWRITTEN, TO EXCEL, CSV, PDF
# OR TAB SEPARATED TEXT. WITHOUT ARGUMENTS IT OPENS ITS WINDOW; THE EXCEL
# ADD-IN RUNS IT WITH A PICTURE AND A .TSV FILE.
#
#   PICCELL.exe [-noupdate]
#   PICCELL.exe <picture> <result> [-mode auto|print|hand] [-lang tr|en]
#             [-words <known words.txt>] [-dump <ocr words.txt>]
#             [-check <marks.tsv>]
#   PICCELL.exe -clip <result> ...
#
# THE KIND OF THE RESULT COMES FROM ITS NAME: .xlsx, .csv, .pdf, OR ANY
# OTHER NAME FOR TAB SEPARATED TEXT. -dump WRITES WHAT THE OCR SAW, FOR TESTS.
# -check WRITES A 1 FOR EVERY CELL TO LOOK AT AND A 0 FOR THE OTHERS, SO THE
# EXCEL ADD-IN CAN MAKE THEM YELLOW. THE WINDOW LOOKS FOR A NEW RELEASE ON
# GITHUB (UPDATE.H), UNLESS -noupdate IS GIVEN; -update-finish IS THE SECOND
# HALF OF AN UPDATE AND NOT FOR PEOPLE.
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * #define ATTACH_PARENT_PROCESS
 * #define INVALID_HANDLE_VALUE
 * #define MB_ICONINFORMATION
 * #define MB_OK
 * #define STD_OUTPUT_HANDLE
 * typedef	DWORD;
 * typedef	HANDLE;
 * typedef	WCHAR;
 * BOOL		AttachConsole(DWORD);
 * BOOL		FreeConsole(void);
 * HANDLE	GetStdHandle(DWORD);
 * int		MessageBoxA(HWND, LPCSTR, LPCSTR, UINT);
 * BOOL		WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
 */
#include <stdio.h>
/******* """""""""
 * typedef	FILE;
 * FILE	*_wfopen(const wchar_t *, const wchar_t *);
 * int		fclose(FILE *);
 * int		fputc(int, FILE *);
 * int		fputs(const char *, FILE *);
 */
#include <string.h>
/******* """"""""""
 * #define NULL
 * size_t	strlen(const char *);
 * int		wcscmp(const wchar_t *, const wchar_t *);
 */
#include "piccell.h"
/******* """""""""
 * #define PICCELL_DONE
 * #define PICCELL_NO_WRITE
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
 * void	PICTURE_FREE(struct S_PICTURE *);
 * int	PICTURE_LOAD(const WCHAR *, struct S_PICTURE *);
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
 * int	CONVERT_START(const WCHAR *);
 */
#include "update.h"
/******* """"""""""
 * void	UPDATE_FINISH(void);
 */
#include "ui.h"
/******* """"""
 * int	UI_RUN(const WCHAR *, int);
 */
/* **************************** [^] INCLUDES [^] **************************** */

static const char	G__USAGE__[] =
	"PICCELL - a photo of a table (printed or handwritten) to Excel\r\n"
	"\r\n"
	"  PICCELL.exe [-noupdate]        the window\r\n"
	"  PICCELL.exe <picture> <result> [-mode auto|print|hand] [-lang tr|en]\r\n"
	"            [-words <known words.txt>] [-dump <ocr words.txt>]\r\n"
	"            [-check <marks.tsv>]\r\n"
	"  PICCELL.exe -clip <result> ...\r\n"
	"\r\n"
	"<result> is .xlsx, .csv, .pdf, or tab separated text for any other\r\n"
	"name. -clip reads the picture on the clipboard. -check writes 1 for\r\n"
	"every cell to check and 0 for the others. Exit codes: 0 done,\r\n"
	"1 usage, 2 not a picture, 3 no picture on the clipboard, 4 no Windows\r\n"
	"OCR (Windows 10 or 11 needed), 5 no text found, 6 result not written,\r\n"
	"7 no handwriting reader in PICCELL.exe. The window looks for a new\r\n"
	"release on GitHub when it opens; -noupdate turns that off.\r\n";

/* THE HELP, IN THE CONSOLE PICCELL WAS STARTED FROM (IT HAS NONE OF ITS OWN) */
static void
	SHOW_USAGE(void)
{
	HANDLE	OUTPUT;
	DWORD	WRITTEN;

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		MessageBoxA(NULL, G__USAGE__, "PICCELL", MB_OK | MB_ICONINFORMATION);
		return ;
	}

	OUTPUT = GetStdHandle(STD_OUTPUT_HANDLE);

	if (OUTPUT && OUTPUT != INVALID_HANDLE_VALUE)
		WriteFile(
			OUTPUT,
			G__USAGE__,
			(DWORD)strlen(G__USAGE__),
			&WRITTEN,
			NULL
		);

	FreeConsole();
}

/* THE CELLS TO LOOK AT: 1 OR 0 PER CELL, A ROW PER LINE. 1 = WRITTEN */
static int
	WRITE_CHECKS(const struct S_TABLE *TABLE, const WCHAR *PATH)
{
	FILE	*OUTPUT;
	int		R;
	int		C;

	OUTPUT = _wfopen(PATH, L"wb");

	if (!OUTPUT)
		return (0);

	for (R = 0; R < TABLE->ROWS; ++R)
	{
		for (C = 0; C < TABLE->COLS; ++C)
		{
			int	MARK;

			MARK = 0;

			if (TABLE->CHECK)
				MARK = TABLE->CHECK[R * TABLE->COLS + C] != 0;

			if (C)
				fputc('\t', OUTPUT);

			fputc('0' + MARK, OUTPUT);
		}

		fputs("\r\n", OUTPUT);
	}

	return (fclose(OUTPUT) == 0);
}

/* "auto", "print" OR "hand". -1 WHEN IT IS NONE OF THEM */
static int
	MODE_OF(const WCHAR *NAME)
{
	if (!wcscmp(NAME, L"auto"))
		return (CONVERT_AUTO);

	if (!wcscmp(NAME, L"print"))
		return (CONVERT_PRINT);

	if (!wcscmp(NAME, L"hand"))
		return (CONVERT_HAND);

	return (-1);
}

int
	wmain(int argc, WCHAR **argv)
{
	struct S_PICTURE	PICTURE;
	struct S_TABLE		TABLE;
	const WCHAR			*SOURCE;
	const WCHAR			*RESULT;
	const WCHAR			*LANGUAGE;
	const WCHAR			*WORD_FILE;
	const WCHAR			*DUMP;
	const WCHAR			*CHECKS;
	int					MODE;
	int					UPDATES;
	int					USED;
	int					CODE;
	int					I;

	SOURCE = NULL;
	RESULT = NULL;
	LANGUAGE = NULL;
	WORD_FILE = NULL;
	DUMP = NULL;
	CHECKS = NULL;
	MODE = CONVERT_AUTO;
	UPDATES = 1;

	if (argc >= 2 && !wcscmp(argv[1], L"-update-finish"))
	{
		UPDATE_FINISH();
		return (PICCELL_DONE);
	}

	for (I = 1; I < argc; ++I)
	{
		if (!wcscmp(argv[I], L"-noupdate"))
			UPDATES = 0;
		else if (!wcscmp(argv[I], L"-lang") && I + 1 < argc)
			LANGUAGE = argv[++I];
		else if (!wcscmp(argv[I], L"-words") && I + 1 < argc)
			WORD_FILE = argv[++I];
		else if (!wcscmp(argv[I], L"-dump") && I + 1 < argc)
			DUMP = argv[++I];
		else if (!wcscmp(argv[I], L"-mode") && I + 1 < argc)
			MODE = MODE_OF(argv[++I]);
		else if (!wcscmp(argv[I], L"-check") && I + 1 < argc)
			CHECKS = argv[++I];
		else if (!SOURCE)
			SOURCE = argv[I];
		else if (!RESULT)
			RESULT = argv[I];
		else
			MODE = -1;
	}

	/* NOTHING BUT -words AND -noupdate: THE WINDOW */
	if (!SOURCE && !LANGUAGE && !DUMP && !CHECKS && MODE == CONVERT_AUTO)
		return (UI_RUN(WORD_FILE, UPDATES));

	if (!SOURCE || !RESULT || MODE < 0)
	{
		SHOW_USAGE();
		return (PICCELL_USAGE);
	}

	CONVERT_START(WORD_FILE);

	if (!wcscmp(SOURCE, L"-clip"))
		CODE = PICTURE_LOAD(NULL, &PICTURE); /* NULL MEANS THE CLIPBOARD */
	else
		CODE = PICTURE_LOAD(SOURCE, &PICTURE);

	if (CODE != PICCELL_DONE)
		return (CODE);

	CODE = CONVERT(&PICTURE, MODE, LANGUAGE, DUMP, &TABLE, &USED);
	PICTURE_FREE(&PICTURE);

	if (CODE == PICCELL_DONE)
		CODE = CONVERT_SAVE(&TABLE, RESULT, LANGUAGE);

	if (CODE == PICCELL_DONE && CHECKS && !WRITE_CHECKS(&TABLE, CHECKS))
		CODE = PICCELL_NO_WRITE;

	LAYOUT_FREE(&TABLE);
	return (CODE);
}
