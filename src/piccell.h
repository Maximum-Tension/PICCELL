/******************************************************************************\
# H - piccell                                    #       Maximum Tension       #
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
# WHAT THE OCR HALF AND THE TABLE HALF OF PICCELL.EXE SHARE.
*/

#ifndef PICCELL_H
#define PICCELL_H

#define PICCELL_WORD_MAX 128 /* BYTES OF ONE WORD, UTF-8 */

/* *************************** [v] EXIT CODES [v] *************************** */
/* THE EXCEL ADD-IN TURNS THESE INTO MESSAGES */
#define PICCELL_DONE 0
#define PICCELL_USAGE 1 /* WRONG ARGUMENTS */
#define PICCELL_NO_PICTURE 2 /* THE FILE IS NOT A PICTURE OR CAN NOT BE READ */
#define PICCELL_NO_CLIP 3 /* NO PICTURE ON THE CLIPBOARD */
#define PICCELL_NO_OCR 4 /* NO WINDOWS OCR HERE (WINDOWS 10 OR 11 NEEDED) */
#define PICCELL_NO_TEXT 5 /* NO TEXT WAS FOUND IN THE PICTURE */
#define PICCELL_NO_WRITE 6 /* THE RESULT FILE CAN NOT BE WRITTEN */
#define PICCELL_NO_READER 7 /* NO HANDWRITING READER IN THE EXE (BAD BUILD) */
/* *************************** [^] EXIT CODES [^] *************************** */

struct	S_WORD
{
	int		LINE; /* WORDS OF ONE OCR LINE SHARE THIS NUMBER */
	float	X; /* LEFT EDGE, IN PIXELS */
	float	Y; /* TOP EDGE */
	float	W;
	float	H;
	char	TEXT[PICCELL_WORD_MAX]; /* UTF-8 */
};

#endif /* !PICCELL_H */
