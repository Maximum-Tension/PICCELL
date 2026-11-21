/******************************************************************************\
# C - lines                                      #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/18 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/20 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# TABLE BORDERS CONFUSE THE OCR: IT READS THEM AS "|", "[", "_" OR GLUES THEM
# TO THE LETTERS NEXT TO THEM. LETTER STROKES ARE SHORT, BORDERS ARE LONG:
# 1. A PIXEL IS DARK WHEN IT IS 12 DARKER THAN THE AVERAGE AROUND IT (25X25),
#    SO SHADOWS AND UNEVEN LIGHT ON A PHOTO DO NOT MATTER.
# 2. A RUN OF DARK PIXELS LONGER THAN 1/30 OF THE PICTURE (AT LEAST 40) IS A
#    LINE. ONE PIXEL ABOVE AND BELOW COUNTS TOO, SO A SLIGHTLY TURNED LINE IS
#    STILL ONE RUN. THE SAME FOR STANDING LINES.
# 3. LINE PIXELS, AND THE PIXELS NEXT TO THEM, GET THE COLOUR OF THE PAPER:
#    THE AVERAGE OF THE PIXELS AROUND THAT ARE AS LIGHT AS THEIR AVERAGE OR
#    LIGHTER. (THE AVERAGE OF ALL OF THEM, LINES AND LETTERS TOO, LEFT A GREY
#    LINE THE OCR READ AS "l".)
*/

/* **************************** [v] INCLUDES [v] **************************** */
#include <stdlib.h>
/******* """"""""""
 * void	*calloc(size_t, size_t);
 * void	free(void *);
 * void	*malloc(size_t);
 */
#include <string.h>
/******* """"""""""
 * typedef	size_t;
 */
#include "lines.h"
/******* """""""""
 * int	LINES_ERASE(unsigned char *, int, int);
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define AROUND 12 /* THE AVERAGE IS TAKEN OVER (2 * 12 + 1) PIXELS EACH WAY */
#define DARKER 12 /* A DARK PIXEL IS THIS MUCH DARKER THAN ITS AVERAGE */
#define LINE_PART 30 /* A LINE IS LONGER THAN 1/30 OF THE PICTURE */
#define LINE_LEAST 40 /* AND LONGER THAN 40 PIXELS */
#define PAPERISH 4 /* NEXT TO A LINE, PIXELS THIS MUCH DARKER ARE ERASED TOO */
#define PAPER_NOISE 3 /* PAPER MAY BE THIS MUCH DARKER THAN ITS AVERAGE */

/* MARK[] |= BIT WHERE A RUN OF SET NEARBY[] IS AT LEAST LEAST LONG */
static void
	MARK_RUNS(
		const unsigned char *NEARBY,
		unsigned char *MARK,
		int COUNT,
		int STEP,
		int LEAST
	)
{
	int	I;

	I = 0;

	while (I < COUNT)
	{
		int	START;

		while (I < COUNT && !NEARBY[I * STEP])
			++I;

		START = I;

		while (I < COUNT && NEARBY[I * STEP])
			++I;

		if (I - START >= LEAST)
		{
			int	J;

			for (J = START; J < I; ++J)
				MARK[J * STEP] = 1;
		}
	}
}

/* V KEPT INSIDE 0 ... COUNT - 1 */
static int
	CLAMP(int V, int COUNT)
{
	if (V < 0)
		return (0);

	if (V >= COUNT)
		return (COUNT - 1);

	return (V);
}

/*
# THE AVERAGE OF THE (2 * AROUND + 1) SQUARE AROUND EVERY PIXEL, IN TWO
# PASSES: FIRST ALONG THE ROWS, THEN ALONG THE COLUMNS.
*/
static void
	BOX_AVERAGE(
		const unsigned char *GRAY,
		unsigned char *MEAN,
		unsigned short *SUMS,
		int WIDTH,
		int HEIGHT
	)
{
	const unsigned int	AREA = (2 * AROUND + 1) * (2 * AROUND + 1);
	int					X;
	int					Y;

	for (Y = 0; Y < HEIGHT; ++Y)
	{
		unsigned int	SUM;
		int				K;

		SUM = 0;

		for (K = -AROUND; K <= AROUND; ++K)
			SUM += GRAY[Y * WIDTH + CLAMP(K, WIDTH)];

		for (X = 0; X < WIDTH; ++X)
		{
			SUMS[Y * WIDTH + X] = (unsigned short)SUM;
			SUM -= GRAY[Y * WIDTH + CLAMP(X - AROUND, WIDTH)];
			SUM += GRAY[Y * WIDTH + CLAMP(X + AROUND + 1, WIDTH)];
		}
	}

	for (X = 0; X < WIDTH; ++X)
	{
		unsigned int	SUM;
		int				K;

		SUM = 0;

		for (K = -AROUND; K <= AROUND; ++K)
			SUM += SUMS[CLAMP(K, HEIGHT) * WIDTH + X];

		for (Y = 0; Y < HEIGHT; ++Y)
		{
			MEAN[Y * WIDTH + X] = (unsigned char)(SUM / AREA);
			SUM -= SUMS[CLAMP(Y - AROUND, HEIGHT) * WIDTH + X];
			SUM += SUMS[CLAMP(Y + AROUND + 1, HEIGHT) * WIDTH + X];
		}
	}
}

/*
# THE COLOUR OF THE PAPER AROUND EVERY PIXEL: THE AVERAGE OF THE PIXELS OF
# THE (2 * AROUND + 1) SQUARE THAT ARE LIGHT[]. MEAN WHERE NONE OF THEM IS.
*/
static void
	BOX_PAPER(
		const unsigned char *GRAY,
		const unsigned char *LIGHT,
		const unsigned char *MEAN,
		unsigned char *PAPER,
		unsigned short *SUMS,
		unsigned char *COUNTS,
		int WIDTH,
		int HEIGHT
	)
{
	int	X;
	int	Y;

	for (Y = 0; Y < HEIGHT; ++Y)
	{
		const unsigned char	*ROW = GRAY + (size_t)Y * WIDTH;
		const unsigned char	*TAKE = LIGHT + (size_t)Y * WIDTH;
		unsigned int		SUM;
		unsigned int		COUNT;
		int					K;

		SUM = 0;
		COUNT = 0;

		for (K = -AROUND; K <= AROUND; ++K)
		{
			const int	AT = CLAMP(K, WIDTH);

			SUM += ROW[AT] * (unsigned int)TAKE[AT];
			COUNT += TAKE[AT];
		}

		for (X = 0; X < WIDTH; ++X)
		{
			const int	OUT = CLAMP(X - AROUND, WIDTH);
			const int	IN = CLAMP(X + AROUND + 1, WIDTH);

			SUMS[(size_t)Y * WIDTH + X] = (unsigned short)SUM;
			COUNTS[(size_t)Y * WIDTH + X] = (unsigned char)COUNT;
			SUM -= ROW[OUT] * (unsigned int)TAKE[OUT];
			COUNT -= TAKE[OUT];
			SUM += ROW[IN] * (unsigned int)TAKE[IN];
			COUNT += TAKE[IN];
		}
	}

	for (X = 0; X < WIDTH; ++X)
	{
		unsigned int	SUM;
		unsigned int	COUNT;
		int				K;

		SUM = 0;
		COUNT = 0;

		for (K = -AROUND; K <= AROUND; ++K)
		{
			SUM += SUMS[(size_t)CLAMP(K, HEIGHT) * WIDTH + X];
			COUNT += COUNTS[(size_t)CLAMP(K, HEIGHT) * WIDTH + X];
		}

		for (Y = 0; Y < HEIGHT; ++Y)
		{
			const size_t	OUT = (size_t)CLAMP(Y - AROUND, HEIGHT) * WIDTH;
			const size_t	IN = (size_t)CLAMP(Y + AROUND + 1, HEIGHT) * WIDTH;

			if (COUNT)
				PAPER[(size_t)Y * WIDTH + X] = (unsigned char)(SUM / COUNT);
			else
				PAPER[(size_t)Y * WIDTH + X] = MEAN[(size_t)Y * WIDTH + X];

			SUM -= SUMS[OUT + X];
			COUNT -= COUNTS[OUT + X];
			SUM += SUMS[IN + X];
			COUNT += COUNTS[IN + X];
		}
	}
}

/* THE SHORTEST LINE ACROSS A SIDE OF SIZE PIXELS */
static int
	SHORTEST_LINE(int SIZE)
{
	if (SIZE / LINE_PART > LINE_LEAST)
		return (SIZE / LINE_PART);

	return (LINE_LEAST);
}

int
	LINES_ERASE(unsigned char *PIXELS, int WIDTH, int HEIGHT)
{
	const size_t	SIZE = (size_t)WIDTH * (size_t)HEIGHT;
	const int		COUNT = (int)SIZE;
	unsigned char	*GRAY;
	unsigned char	*MEAN;
	unsigned char	*DARK;
	unsigned char	*NEARBY;
	unsigned char	*LINE;
	unsigned char	*PAPER;
	unsigned char	*COUNTS;
	unsigned short	*SUMS;
	int				X;
	int				Y;
	int				I;

	GRAY = (unsigned char *)calloc(SIZE, 1);
	MEAN = (unsigned char *)calloc(SIZE, 1);
	DARK = (unsigned char *)malloc(SIZE);
	NEARBY = (unsigned char *)calloc(SIZE, 1);
	LINE = (unsigned char *)calloc(SIZE, 1);
	PAPER = (unsigned char *)malloc(SIZE);
	COUNTS = (unsigned char *)malloc(SIZE);
	SUMS = (unsigned short *)malloc(SIZE * sizeof(unsigned short));

	if (
		!GRAY || !MEAN || !DARK || !NEARBY || !LINE || !PAPER || !COUNTS ||
		!SUMS
	)
	{
		free(GRAY);
		free(MEAN);
		free(DARK);
		free(NEARBY);
		free(LINE);
		free(PAPER);
		free(COUNTS);
		free(SUMS);
		return (0);
	}

	for (I = 0; I < COUNT; ++I)
	{
		const unsigned char	*PIXEL = PIXELS + (size_t)I * 4;

		GRAY[I] = (unsigned char)(
			(PIXEL[0] * 29 + PIXEL[1] * 150 + PIXEL[2] * 77) >> 8
		);
	}

	BOX_AVERAGE(GRAY, MEAN, SUMS, WIDTH, HEIGHT);

	for (I = 0; I < COUNT; ++I)
	{
		DARK[I] = GRAY[I] + DARKER < MEAN[I];
		NEARBY[I] = GRAY[I] >= MEAN[I]; /* LIGHT: PAPER, NOT INK */
	}

	BOX_PAPER(GRAY, NEARBY, MEAN, PAPER, SUMS, COUNTS, WIDTH, HEIGHT);

	/* ONCE MORE, WITH WHAT IS AS LIGHT AS THAT PAPER: EDGES OF STROKES GO */
	for (I = 0; I < COUNT; ++I)
		NEARBY[I] = GRAY[I] + PAPER_NOISE >= PAPER[I];

	BOX_PAPER(GRAY, NEARBY, MEAN, PAPER, SUMS, COUNTS, WIDTH, HEIGHT);
	free(SUMS);
	free(COUNTS);

	/* LYING LINES: DARK HERE, ABOVE OR BELOW */
	for (I = 0; I < COUNT; ++I)
	{
		NEARBY[I] = (
			DARK[I] ||
			(I >= WIDTH && DARK[I - WIDTH]) ||
			(I + WIDTH < COUNT && DARK[I + WIDTH])
		);
	}

	for (Y = 0; Y < HEIGHT; ++Y)
	{
		MARK_RUNS(
			NEARBY + (size_t)Y * WIDTH,
			LINE + (size_t)Y * WIDTH,
			WIDTH,
			1,
			SHORTEST_LINE(WIDTH)
		);
	}

	/* STANDING LINES: DARK HERE, ON THE LEFT OR ON THE RIGHT */
	for (I = 0; I < COUNT; ++I)
	{
		X = I % WIDTH;
		NEARBY[I] = (
			DARK[I] ||
			(X > 0 && DARK[I - 1]) ||
			(X + 1 < WIDTH && DARK[I + 1])
		);
	}

	for (X = 0; X < WIDTH; ++X)
		MARK_RUNS(NEARBY + X, LINE + X, HEIGHT, WIDTH, SHORTEST_LINE(HEIGHT));

	/* GROW THE LINES BY ONE PIXEL EACH WAY INTO NEARBY */
	for (I = 0; I < COUNT; ++I)
	{
		X = I % WIDTH;
		NEARBY[I] = (
			LINE[I] ||
			(I >= WIDTH && LINE[I - WIDTH]) ||
			(I + WIDTH < COUNT && LINE[I + WIDTH]) ||
			(X > 0 && LINE[I - 1]) ||
			(X + 1 < WIDTH && LINE[I + 1])
		);
	}

	for (I = 0; I < COUNT; ++I)
	{
		if (NEARBY[I] && GRAY[I] + PAPERISH < PAPER[I])
		{
			unsigned char	*PIXEL = PIXELS + (size_t)I * 4;

			PIXEL[0] = PAPER[I];
			PIXEL[1] = PAPER[I];
			PIXEL[2] = PAPER[I];
		}
	}

	free(PAPER);
	free(GRAY);
	free(MEAN);
	free(DARK);
	free(NEARBY);
	free(LINE);
	return (1);
}
