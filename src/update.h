/******************************************************************************\
# H - update                                     #       Maximum Tension       #
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
# THE GITHUB RELEASE AUTO UPDATER. WHEN THE WINDOW OPENS, PICCELL ASKS GITHUB
# FOR THE LATEST RELEASE OF MAXIMUM-TENSION/PICCELL IN THE BACKGROUND. IT
# UPDATES WHEN THE SHA-256 GITHUB GIVES FOR THAT RELEASE'S PICCELL.EXE IS NOT
# THE SHA-256 OF THE RUNNING EXE, SO TWO RELEASES ON THE SAME DAY WORK.
# TWO SAFETY RULES: A RELEASE WITH AN OLDER DATE THAN THIS EXE'S VERSION IS
# NEVER INSTALLED, AND AN EXE NEXT TO "SRC\PICCELL.H" (A TEST BUILD IN THE
# SOURCE FOLDER) NEVER UPDATES ITSELF.
# 1. UPDATE_FIND(): PICCELL.EXE OF THAT RELEASE IS DOWNLOADED INTO %TMP% AND ITS
# SHA-256 IS CHECKED AGAINST THE SAME DIGEST AGAIN. A WRONG OR MISSING DIGEST
# MEANS NO UPDATE. THE WINDOW SAYS IT IS READY; NOTHING STOPS THE WORK.
# 2. UPDATE_APPLY(): WHEN THE WINDOW CLOSES (OR "RESTART NOW" IS PRESSED), THE
# NEW EXE IS STARTED, PAUSED, AND A NOTE NEXT TO IT IN %TMP% TELLS IT WHERE
# THE OLD EXE IS. THEN THE OLD ONE CLOSES.
# 3. UPDATE_FINISH(): THE NEW ONE ("-UPDATE-FINISH") READS THE NOTE, WAITS FOR
# THE OLD ONE TO CLOSE, MOVES ITSELF TO THAT PATH AND DELETES THE NOTE. AFTER
# "RESTART NOW" IT OPENS THE WINDOW AGAIN, FROM THE OLD PATH.
# NO NETWORK, A RATE LIMIT OR ANY ERROR: NOTHING HAPPENS. COMMAND LINE RUNS
# (THE EXCEL ADD-IN) NEVER ASK GITHUB, SO THEY STAY FAST AND OFFLINE.
*/

#ifndef UPDATE_H
#define UPDATE_H

/* **************************** [v] INCLUDES [v] **************************** */
#include <stddef.h>
/******* """"""""""
 * typedef	size_t;
 */
/* **************************** [^] INCLUDES [^] **************************** */

#define UPDATE_REPO L"Maximum-Tension/PICCELL"

#ifdef _WIN64
#define UPDATE_ASSET "PICCELL.exe"
#else /* !_WIN64 */
#define UPDATE_ASSET "PICCELL32.exe" /* THE 32-BIT BUILD STAYS 32-BIT */
#endif /* _WIN64 */

#define UPDATE_NONE 0 /* OFFLINE, THE LATEST ALREADY, OR NOT ALLOWED HERE */
#define UPDATE_READY 1 /* THE NEW EXE IS IN %TMP%, CHECKED */

/* BLOCKS FOR SECONDS: RUN IT IN A THREAD. TAG GETS THE RELEASE NAME */
extern int	UPDATE_FIND(char *TAG, size_t CAP);
/* OPEN_AGAIN 1 = "RESTART NOW". 1 = THE NEW EXE TAKES OVER: CLOSE NOW */
extern int	UPDATE_APPLY(int OPEN_AGAIN);
/* WE ARE THE NEW EXE IN %TMP% ("-UPDATE-FINISH"): TAKE THE OLD ONE'S PLACE */
extern void	UPDATE_FINISH(void);

#endif /* !UPDATE_H */
