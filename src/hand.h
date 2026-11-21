/******************************************************************************\
# H - hand                                       #       Maximum Tension       #
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
# A PHOTO OF A HANDWRITTEN (OR PRINTED) PAGE TO A TABLE, WITHOUT WINDOWS OCR:
# THE PAGE FINDER CUTS THE LINES, THE READER READS THEM, AND THE PAGE ITSELF
# HELPS TO CORRECT THEM:
# - A LIST ("10098 - CREAM - GRAY - 12 TOP") IS CUT AT ITS DASHES; OTHER
#   PAGES GO TO THE TABLE MAKER OF LAYOUT.C, LIKE THE WORDS OF WINDOWS OCR.
# - IN A COLUMN OF NUMBERS ONLY DIGITS ARE READ ("1O" IS "10").
# - A WORD THAT COMES AGAIN AND AGAIN IN A COLUMN WINS OVER A RARE ONE THAT
#   LOOKS ALMOST THE SAME ("CRAAM" IS "CREAM"), WHEN THE INK FITS IT TOO.
# - WHEN A TOTAL IS WRITTEN UNDER A LIST AND THE NUMBERS DO NOT ADD UP TO IT,
#   THE ONE NUMBER WHOSE NEXT BEST READING MAKES THE SUM RIGHT IS CHANGED.
*/

#ifndef HAND_H
#define HAND_H

/* **************************** [v] INCLUDES [v] **************************** */
#include "piccell.h"
/******* """""""""
 * struct S_WORD;
 */
#include "layout.h"
/******* """"""""""
 * struct S_TABLE;
 */
#include "reader.h"
/******* """"""""""
 * struct S_READER;
 */
/* **************************** [^] INCLUDES [^] **************************** */

struct	S_READER;
struct	S_HAND;
struct	S_WORD;

/* THE READER TO USE (HAND_READ FREES NOTHING) */
extern void				HAND_SET_READER(struct S_READER *READER);
/* 1 = A KNOWN WORD (BUILT IN OR ADDED), IN ANY CASE */
extern int				HAND_KNOWN(const char *TEXT);
/* MORE KNOWN WORDS: ONE PER LINE, OR SPLIT BY , OR ; (UTF-8) */
extern void				HAND_ADD_WORDS(const char *TEXT);
/*
# THE LINES OF A PAGE (NOT READ YET). GRAY: WIDTH * HEIGHT BYTES, 0 BLACK,
# 255 WHITE. NULL = NO LINES, NO READER OR NO MEMORY.
*/
extern struct S_HAND	*HAND_OPEN(
	const unsigned char *GRAY,
	int WIDTH,
	int HEIGHT
);
/*
# HOW WELL THE WORDS OF THE WINDOWS OCR (IN PIXELS OF THE SAME PICTURE) FIT
# WHAT THE READER SEES, ON A FEW LINES: THE COST OF EVERY CHARACTER (NEAR 0
# = THE OCR READ IT WELL, A PRINTED PAGE). -1 = NOTHING TO JUDGE; HUGE = THE
# OCR MISSED MANY LINES.
*/
extern double			HAND_JUDGE(
	struct S_HAND *H,
	const struct S_WORD *WORDS,
	int COUNT
);
/* THE PAGE READ AND MADE A TABLE. 1 = A TABLE CAME OUT (LAYOUT_FREE IT) */
extern int				HAND_TABLE(struct S_HAND *H, struct S_TABLE *TABLE);
extern void				HAND_CLOSE(struct S_HAND *H);
/*
# ALL OF IT AT ONCE. 1 = A TABLE CAME OUT IN *TABLE (FREE IT WITH
# LAYOUT_FREE), 0 = NO TEXT, NO READER OR NO MEMORY.
*/
extern int				HAND_READ(
	const unsigned char *GRAY,
	int WIDTH,
	int HEIGHT,
	struct S_TABLE *TABLE
);

#endif /* !HAND_H */
