#! /bin/sh
#----------------------------------------------------------------------------
# test suite for IRMP
#
# usage:
#
#        ./test-suite.sh
#
# Copyright (c) 2010 Frank Meyer - frank(at)fli4l.de
#
# $Id: test-suite.sh,v 1.6 2010/05/25 14:44:41 fm Exp $
#----------------------------------------------------------------------------

for j in                            \
    Dbox.txt                        \
    DK_Digital.txt                  \
    Grundig_TP715.txt               \
    Grundig_TP715_SatTV.txt         \
    Grundig_TP715_Video.txt         \
    Matsushita.txt                  \
    Nokia.txt                       \
    Panasonic-Blue-Ray.txt          \
    RC5-Taste.txt                   \
    Samsung_DVD_Rec_00062C.txt      \
    Samsung_TV.txt                  \
    Sony-RM-S-310.txt               \
    sony-rm-s311.txt                \
    Sony-RM-U305C.txt               \
    Sony-RMT-D142P-DVD.txt          \
    Sony-RMT-V406.txt               \
    Sony_RM-S315_lange.txt          \
    Sony_Bravia_RM-ED0009_new.txt   \
    Yamaha-RAV388.txt               \
    apple.txt                       \
    bo_beolink1000-10kHz.txt        \
    denon.txt                       \
    elta_radio.txt                  \
    nec-repetition.txt              \
    nec.txt                         \
    nubert-subwoofer.txt            \
    orion_vcr_07660BM070.txt        \
    panasonic-scan.txt              \
    rc5.txt                         \
    rc5x-79.txt                     \
    rc5x.txt                        \
    rc6-hold.txt                    \
    rc6.txt                         \
    sharp-denon.txt                 \
    sharp-denon2.txt
do
    echo "testing $j ..."
    if ../irmp < $j | grep -q error
    then
        ../irmp < $j | grep error
        echo "test failed"
        exit 1
    fi
done
echo "all tests successful"
exit 0
