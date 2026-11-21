/******************************************************************************\
# H - lines                                      #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/18 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/18 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# ERASES TABLE BORDERS FROM A PICTURE BEFORE THE OCR READS IT. NO WINDOWS
# CALLS HERE, SO THE SAME FILE ALSO BUILDS FOR THE TEST PROGRAM ON LINUX.
*/

#ifndef LINES_H
#define LINES_H

/* PIXELS: 4 BYTES EACH (B, G, R, A), ROWS ONE AFTER THE OTHER. 1 = DONE */
extern int	LINES_ERASE(unsigned char *PIXELS, int WIDTH, int HEIGHT);

#endif /* !LINES_H */
