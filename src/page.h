/******************************************************************************\
# H - page                                       #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/20 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# A PHOTO OF A HANDWRITTEN PAGE TO ITS TEXT LINES. EVERY LINE COMES OUT
# STRAIGHT, CUT TO PAGE_HEIGHT PIXELS HIGH AND READY FOR THE READER: 0 IS
# PAPER, 1 IS INK. A HANDWRITTEN LINE MAY LEAN AND WANDER, SO THE PAGE IS CUT
# IN STRIPS, THE LINES ARE FOUND IN EVERY STRIP AND JOINED FROM STRIP TO
# STRIP. EVERY PIECE OF INK GOES TO THE LINE THAT PASSES CLOSEST TO IT.
*/

#ifndef PAGE_H
#define PAGE_H

#define PAGE_HEIGHT 32 /* HEIGHT OF A LINE FOR THE READER */
#define PAGE_DASHES 32 /* THE MOST "-" A LINE REMEMBERS */
#define PAGE_CUTS 32 /* THE MOST DRAWN COLUMN LINES A LINE CROSSES */

struct	S_LINE
{
	int		X0; /* WHERE THE LINE IS ON THE PAGE */
	int		Y0;
	int		X1;
	int		Y1;
	int		LEFT; /* PAGE X OF THE FIRST COLUMN OF INK[] */
	float	SCALE; /* PAGE PIXELS PER COLUMN OF INK[] */
	int		W; /* COLUMNS OF INK[] */
	float	*INK; /* PAGE_HEIGHT ROWS OF W */
	int		DASHES; /* LONE FLAT STROKES IN THE MIDDLE OF THE LINE: "-" */
	int		DASH_FROM[PAGE_DASHES]; /* THEIR COLUMNS OF INK[] */
	int		DASH_TO[PAGE_DASHES];
	int		CUTS; /* DRAWN COLUMN LINES (OF A TABLE) THE LINE CROSSES */
	int		CUT_AT[PAGE_CUTS]; /* THEIR COLUMNS OF INK[] */
	float	Y_LEFT; /* PAGE Y OF ITS MIDDLE AT X0 AND AT X1 (IT MAY SLANT) */
	float	Y_RIGHT;
	int		TABLE; /* THE DRAWN TABLE ITS MIDDLE IS IN, -1 = NONE */
};

/*
# GRAY: WIDTH * HEIGHT BYTES, 0 BLACK, 255 WHITE. RETURNS THE NUMBER OF LINES
# (FROM THE TOP) IN *LINES, A MALLOC'D LIST; 0 WHEN NONE OR NO MEMORY.
*/
extern int	PAGE_LINES(
	const unsigned char *GRAY,
	int WIDTH,
	int HEIGHT,
	struct S_LINE **LINES
);
extern void	PAGE_FREE(struct S_LINE *LINES, int COUNT);
/* THE USUAL HEIGHT OF A LETTER ON THE PAGE, FROM THE LAST PAGE_LINES() */
extern int	PAGE_LETTER_HEIGHT(void);
/*
# A POINT OF THE LAST PAGE GIVEN TO PAGE_LINES() TO THE PAGE ITS LINES ARE
# ON (A TURNED PHOTO IS TURNED BACK BEFORE THE LINES ARE FOUND)
*/
extern void	PAGE_TURN(float *X, float *Y);
/*
# THE LYING LINES OF THE DRAWN TABLES OF THE LAST PAGE (PIECES OF THEM,
# BETWEEN THE COLUMN LINES): *ENDS GETS 4 VALUES FOR EVERY ONE, X AND Y OF
# ITS LEFT END, X AND Y OF ITS RIGHT END. RETURNS HOW MANY.
*/
extern int	PAGE_RULES(const float **ENDS);

#endif /* !PAGE_H */
