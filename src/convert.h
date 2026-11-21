/******************************************************************************\
# H - convert                                    #       Maximum Tension       #
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

/*
# A PICTURE TO A TABLE, PRINTED OR HANDWRITTEN, AND A TABLE TO A FILE. THE
# COMMAND LINE (MAIN.C) AND THE WINDOW (UI.C) BOTH COME HERE.
# - PRINTED: THE OCR OF WINDOWS READS IT, LAYOUT.C MAKES THE TABLE.
# - HANDWRITTEN: THE READER BUILT INTO PICCELL.EXE READS IT (HAND.C).
# - AUTO: THE OCR READS IT FIRST, AND THE READER JUDGES WHAT IT SAW ON A FEW
#   LINES. WHEN THE OCR SAW THE INK WELL, IT IS A PRINTED PAGE.
*/

#ifndef CONVERT_H
#define CONVERT_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * typedef	WCHAR;
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 */
#include "picture.h"
/******* """""""""""
 * struct S_PICTURE;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define CONVERT_AUTO 0
#define CONVERT_PRINT 1 /* THE OCR OF WINDOWS 10 AND 11 */
#define CONVERT_HAND 2 /* THE READER IN PICCELL.EXE */

/*
# THE READER (FROM THE RESOURCES OF PICCELL.EXE, OR READER.BIN NEXT TO IT) AND
# THE KNOWN WORDS OF WORD_FILE (NULL = NONE). 1 = THE READER IS THERE.
*/
extern int	CONVERT_START(const WCHAR *WORD_FILE);
/*
# LANGUAGE: "tr", "en-US"... OR NULL. DUMP: NULL, OR A FILE FOR WHAT THE OCR
# SAW. *USED GETS CONVERT_PRINT OR CONVERT_HAND. A PICCELL_ CODE.
*/
extern int	CONVERT(
	const struct S_PICTURE *PICTURE,
	int MODE,
	const WCHAR *LANGUAGE,
	const WCHAR *DUMP,
	struct S_TABLE *TABLE,
	int *USED
);
/* THE TABLE TO A FILE; ITS KIND COMES FROM ITS NAME (.XLSX, .CSV, .PDF...) */
extern int	CONVERT_SAVE(
	const struct S_TABLE *TABLE,
	const WCHAR *PATH,
	const WCHAR *LANGUAGE
);
/*
# A WORD THE USER FIXED IN THE WINDOW: KNOWN FROM NOW ON, ALSO NEXT TIME
# (%APPDATA%\PICCELL\WORDS.TXT). 1 = LEARNED.
*/
extern int	CONVERT_LEARN(const char *TEXT);
/* 1 = NUMBERS ARE WRITTEN THE TURKISH WAY ("1.234,56") FOR THIS LANGUAGE */
extern int	CONVERT_TURKISH(const WCHAR *LANGUAGE);

#endif /* !CONVERT_H */
