/******************************************************************************\
# H - ctc                                        #       Maximum Tension       #
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
# FROM WHAT THE READER SAW TO TEXT. THE READER GIVES, FOR EVERY STEP OF A
# LINE, HOW LIKELY EVERY CLASS IS (CLASS 0 = NOTHING). A TEXT IS READ BY
# TAKING THE BEST CLASS AT EVERY STEP AND DROPPING REPEATS AND NOTHINGS
# ("--CC-RR-EE-A-M" IS "CREAM"). CTC_SCORE TELLS HOW WELL ANY GIVEN TEXT FITS
# A PIECE OF THE LINE: THAT IS HOW A GUESS IS CHECKED AGAINST THE INK.
*/

#ifndef CTC_H
#define CTC_H

#define CTC_NODES_MAX 64
#define CTC_ARCS_MAX 256

struct	S_GLYPH
{
	int	CLASS;
	int	FIRST; /* FIRST AND LAST STEP OF THE LINE WHERE IT WAS SEEN */
	int	LAST;
};

/*
# THE BEST CLASS AT EVERY STEP, WITH THE NOTHINGS AND THE REPEATS DROPPED.
# ALLOWED: NULL, OR ONE BYTE PER CLASS (1 = MAY BE USED). STEPS FROM ... TO
# (NOT INCLUDED). RETURNS THE NUMBER OF GLYPHS WRITTEN (AT MOST MOST).
*/
extern int		CTC_GREEDY(
	const float *LOGP,
	int CLASSES,
	int FROM,
	int TO,
	const unsigned char *ALLOWED,
	struct S_GLYPH *GLYPHS,
	int MOST
);
/*
# LOG OF HOW LIKELY THE CLASSES LABELS[0 ... COUNT - 1] ARE ON STEPS FROM ...
# TO (ALL WAYS OF PLACING THEM ADDED UP). VERY NEGATIVE WHEN THEY DO NOT FIT.
*/
extern double	CTC_SCORE(
	const float *LOGP,
	int CLASSES,
	int FROM,
	int TO,
	const int *LABELS,
	int COUNT
);

/* A GRAMMAR: NODES AND ARCS. EVERY ARC SEES ONE NEW GLYPH; NODE 0 STARTS */
struct	S_ARC
{
	int					FROM;
	int					TO;
	const unsigned char	*ALLOWED; /* ONE BYTE PER CLASS, NULL = ANY CLASS */
	float				COST; /* ADDED TO THE SCORE (0 OR NEGATIVE) */
};

struct	S_GRAMMAR
{
	int				NODES;
	int				ARCS;
	int				TAG[CTC_NODES_MAX]; /* GIVEN BACK WITH EVERY GLYPH */
	unsigned char	FINAL[CTC_NODES_MAX]; /* 1 = THE LINE MAY END HERE */
	struct S_ARC	ARC[CTC_ARCS_MAX];
};

/*
# THE BEST TEXT ON STEPS FROM ... TO THAT THE GRAMMAR ALLOWS. GLYPHS AND
# TAGS (THE TAG OF THE NODE EVERY GLYPH LED TO; TAGS MAY BE NULL) GET AT
# MOST MOST ITEMS, *COUNT THEIR NUMBER. RETURNS THE SCORE OF THAT WAY (LOG),
# VERY NEGATIVE WHEN THE GRAMMAR CAN NOT FIT.
*/
extern double	CTC_GRAMMAR(
	const float *LOGP,
	int CLASSES,
	int FROM,
	int TO,
	const struct S_GRAMMAR *G,
	struct S_GLYPH *GLYPHS,
	int *TAGS,
	int MOST,
	int *COUNT
);

#endif /* !CTC_H */
