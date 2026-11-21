/******************************************************************************\
# H - ocr                                        #       Maximum Tension       #
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
# READS THE WORDS OF A PICTURE WITH THE OCR BUILT INTO WINDOWS 10 AND 11.
*/

#ifndef OCR_H
#define OCR_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * typedef	WCHAR;
 */
#include "piccell.h"
/******* """""""""
 * struct S_WORD;
 */
#include "picture.h"
/******* """""""""""
 * struct S_PICTURE;
 */
/* **************************** [^] INCLUDES [^] **************************** */

/*
# LANGUAGE: "tr", "en-US"..., OR NULL FOR TURKISH WHEN WINDOWS HAS IT, ELSE
# THE LANGUAGES OF THE USER. RETURNS A PICCELL_ EXIT CODE; ON PICCELL_DONE
# *WORDS IS A MALLOC'D LIST OF *COUNT WORDS, IN PIXELS OF THE PICTURE.
*/
extern int	OCR_READ(
	const struct S_PICTURE *PICTURE,
	const WCHAR *LANGUAGE,
	struct S_WORD **WORDS,
	int *COUNT
);

#endif /* !OCR_H */
