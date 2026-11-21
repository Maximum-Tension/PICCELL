/******************************************************************************\
# H - version                                    #       Maximum Tension       #
################################################################################
#                                                #      -__            __-     #
# Teoman Deniz                                   #  :    :!1!-_    _-!1!:    : #
# maximum-tension.com                            #  ::                      :: #
#                                                #  :!:    : :: : :  :  ::::!: #
# +.....................++.....................+ #   :!:: :!:!1:!:!::1:::!!!:  #
# : C - Maximum Tension :: Create - 2026/11/20 : #   ::!::!!1001010!:!11!!::   #
# :---------------------::---------------------: #   :!1!!11000000000011!!:    #
# : License - GPL-3.0   :: Update - 2026/11/21 : #    ::::!!!1!!1!!!1!!!::     #
# +.....................++.....................+ #       ::::!::!:::!::::      #
\******************************************************************************/

/*
# THE VERSION OF PICCELL. IT IS THE RELEASE DATE: YY.YY.MM.DD, SO 2026/11/20 IS
# 20.26.11.20. THE GITHUB RELEASE TAG STARTS WITH THE SAME DATE, ANYTHING MAY
# FOLLOW ("20.26.11.20-MK1151"). THE UPDATER NEVER INSTALLS A RELEASE WITH AN
# OLDER DATE; WHICH ONE IS NEWER IS DECIDED BY THE SHA-256 (SEE UPDATE.H).
# THIS FILE IS ALSO READ BY THE RESOURCE COMPILER, SO IT ONLY HOLDS PLAIN
# DEFINES.
*/

#ifndef VERSION_H
#define VERSION_H

/* ************************* [v] RELEASE NUMBER [v] ************************* */
#define PICCELL_VERSION_A 20
#define PICCELL_VERSION_B 26
#define PICCELL_VERSION_C 11
#define PICCELL_VERSION_D 21
#define PICCELL_VERSION_TEXT "20.26.11.21"
/* THE FOUR NUMBERS FOR THE RESOURCE FILE (FILEVERSION, PRODUCTVERSION) */
#define PICCELL_VERSION_LIST \
	PICCELL_VERSION_A, PICCELL_VERSION_B, PICCELL_VERSION_C, PICCELL_VERSION_D
/* ************************* [^] RELEASE NUMBER [^] ************************* */

#endif /* !VERSION_H */
