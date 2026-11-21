/******************************************************************************\
# H - reader                                     #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/19 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/19 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE LINE READER: A SMALL CONVOLUTIONAL NETWORK, TRAINED ON HANDWRITING AND
# PRINTED FONTS (TOOLS/HAND). IT GETS ONE LINE, PAGE_HEIGHT PIXELS HIGH (0 IS
# PAPER, 1 IS INK), AND GIVES FOR EVERY 4 COLUMNS OF THE LINE HOW LIKELY EVERY
# CHARACTER IS (CLASS 0 = NOTHING HERE). ITS NUMBERS ARE ONE BLOCK OF BYTES
# (READER.BIN, IN THE EXE AS A RESOURCE):
# - "TRD1", CLASS COUNT, LINE HEIGHT, THE CHARACTERS (UTF-8), STEP COUNT;
# - STEPS: CONVOLUTION (INT8 WEIGHTS WITH A SCALE PER OUTPUT, FLOAT BIASES),
#   MAX POOLING, KEEP (A COPY FOR LATER) AND ADD (THE KEPT COPY IS ADDED).
*/

#ifndef READER_H
#define READER_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <string.h>
/******* """"""""""
 * typedef	size_t;
 */
#include "hand.h"
/******* """"""""
 * struct S_READER;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define READER_CHARS_MAX 256 /* BYTES OF THE CHARACTER LIST */

struct	S_READER;

/* NULL WHEN THE BYTES ARE NOT A READER OR THERE IS NO MEMORY */
extern struct S_READER	*READER_LOAD(const void *DATA, size_t SIZE);
extern void				READER_FREE(struct S_READER *READER);
/* NUMBER OF CLASSES: CLASS 0 IS "NOTHING", CLASS K IS THE K-TH CHARACTER */
extern int				READER_CLASSES(const struct S_READER *READER);
/* THE CHARACTER OF CLASS K (K >= 1), UTF-8, ENDING WITH 0 */
extern const char		*READER_CHAR(const struct S_READER *READER, int K);
/*
# INK: PAGE_HEIGHT ROWS OF W COLUMNS. *STEPS GETS THE NUMBER OF OUTPUT
# STEPS; THE RESULT IS A MALLOC'D LIST OF *STEPS ROWS OF READER_CLASSES()
# LOG PROBABILITIES, OR NULL.
*/
extern float			*READER_RUN(
	const struct S_READER *READER,
	const float *INK,
	int W,
	int *STEPS
);

#endif /* !READER_H */
