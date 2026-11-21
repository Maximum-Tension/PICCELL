/******************************************************************************\
# H - ui                                         #       Maximum Tension       #
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
# THE WINDOW OF PICCELL: A PAGE (RES/UI.HTML) IN A WEBVIEW (WEBVIEW-C-WIN32,
# WEBVIEW2 OR THE OLD INTERNET EXPLORER ON WINDOWS 7). OPEN, PASTE OR DROP A
# PHOTO, CROP IT, READ IT, FIX THE CELLS, SAVE IT AS EXCEL, CSV OR PDF, OR
# COPY IT.
*/

#ifndef UI_H
#define UI_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * typedef	WCHAR;
 */
/* **************************** [^] INCLUDES [^] **************************** */

/*
# WORD_FILE: KNOWN WORDS, OR NULL. UPDATES: 0 = NO UPDATE CHECK (-NOUPDATE).
# RETURNS THE EXIT CODE OF PICCELL.EXE.
*/
extern int	UI_RUN(const WCHAR *WORD_FILE, int UPDATES);

#endif /* !UI_H */
