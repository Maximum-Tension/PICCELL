/******************************************************************************\
# H - layout                                     #       Maximum Tension       #
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
# OCR WORDS IN, A TABLE OF CELLS OUT. NO WINDOWS CALLS HERE, SO THE SAME FILE
# ALSO BUILDS FOR THE TEST PROGRAM ON LINUX.
*/

#ifndef LAYOUT_H
#define LAYOUT_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <stdio.h>
/******* """""""""
 * typedef	FILE;
 */
#include "piccell.h"
/******* """""""""
 * struct S_WORD;
 */
/* **************************** [^] INCLUDES [^] **************************** */

struct	S_TABLE
{
	int				ROWS;
	int				COLS;
	char			**CELL; /* CELL[ROW * COLS + COL], UTF-8, NULL WHEN EMPTY */
	unsigned char	*CHECK; /* THE SAME PLACES: 1 = NOT SURE. OR NULL */
};

/* 1 = A TABLE CAME OUT, 0 = NO WORDS OR NO MEMORY */
extern int	LAYOUT_BUILD(
	const struct S_WORD *WORDS,
	int COUNT,
	struct S_TABLE *TABLE
);
/*
# A TABLE WHOSE COLUMNS ARE KNOWN (A TABLE DRAWN BY HAND): THE WORDS COME
# ROW BY ROW, ROWS[I] THE ROW OF WORD I (NULL: EVERY OCR LINE IS A ROW). A
# WORD GOES TO THE COLUMN BETWEEN THE TWO EDGES (X IN PIXELS, FROM THE LEFT)
# ITS MIDDLE IS BETWEEN. WORDS OF ONE CELL ARE JOINED WITH A SPACE, WORDS
# WITHOUT TEXT ARE LEFT OUT. 1 = A TABLE CAME OUT
*/
extern int	LAYOUT_GRID(
	const struct S_WORD *WORDS,
	const int *ROWS,
	int COUNT,
	const float *EDGES,
	int EDGE_COUNT,
	struct S_TABLE *TABLE
);
extern void	LAYOUT_FREE(struct S_TABLE *TABLE);
/*
# THE ROWS OF MORE GO UNDER THE ROWS OF TABLE (AN EMPTY ROW BETWEEN THEM),
# AND MORE IS EMPTIED. 1 = DONE, 0 = NO MEMORY (BOTH STAY AS THEY WERE)
*/
extern int	LAYOUT_APPEND(struct S_TABLE *TABLE, struct S_TABLE *MORE);
/* TAB BETWEEN CELLS, CRLF AFTER EVERY ROW. 1 = WRITTEN */
extern int	LAYOUT_WRITE(const struct S_TABLE *TABLE, FILE *OUTPUT);

#endif /* !LAYOUT_H */
