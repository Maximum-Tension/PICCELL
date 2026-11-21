/******************************************************************************\
# H - picture                                    #       Maximum Tension       #
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
# A PICTURE FROM A FILE, THE CLIPBOARD OR MEMORY (WIC: JPEG, PNG, BMP, GIF,
# TIFF, AND HEIC OR WEBP WHEN WINDOWS HAS THEIR CODECS), TURNED THE WAY THE
# PHONE MEANT IT, AS PLAIN PIXELS.
*/

#ifndef PICTURE_H
#define PICTURE_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <windows.h>
/******* """""""""""
 * typedef	WCHAR;
 */
#include <string.h>
/******* """"""""""
 * typedef	size_t;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define PICTURE_SIDE_MAX 8000 /* A BIGGER PHOTO IS MADE SMALLER WHEN LOADED */

struct	S_PICTURE
{
	int				W;
	int				H;
	unsigned char	*BGRA; /* 4 BYTES A PIXEL, ROW AFTER ROW */
};

/* PATH NULL = THE CLIPBOARD. RETURNS A PICCELL_ CODE (PICCELL_DONE, ...) */
extern int				PICTURE_LOAD(const WCHAR *PATH, struct S_PICTURE *P);
/* THE BYTES OF A PICTURE FILE (A PHOTO DROPPED ON THE WINDOW): SAME CODES */
extern int				PICTURE_FROM_MEMORY(
	const void *DATA,
	size_t SIZE,
	struct S_PICTURE *P
);
/* A COPY IN ANOTHER SIZE (MADE SMALLER BY AVERAGING). NULL = NO MEMORY */
extern unsigned char	*PICTURE_RESIZE(
	const struct S_PICTURE *P,
	int W,
	int H
);
/* A JPEG AT MOST SIDE PIXELS WIDE AND HIGH (*SIZE BYTES). NULL = FAILED */
extern unsigned char	*PICTURE_JPEG(
	const struct S_PICTURE *P,
	int SIDE,
	size_t *SIZE
);
/* ONE GRAY BYTE A PIXEL (0 BLACK, 255 WHITE). NULL = NO MEMORY */
extern unsigned char	*PICTURE_GRAY(const struct S_PICTURE *P);
/* THE PART X, Y, W, H OF A PICTURE AS A NEW ONE. 0 = NOT INSIDE, NO MEMORY */
extern int				PICTURE_CROP(
	const struct S_PICTURE *P,
	int X,
	int Y,
	int W,
	int H,
	struct S_PICTURE *PART
);
extern void				PICTURE_FREE(struct S_PICTURE *P);

#endif /* !PICTURE_H */
