#!/usr/local/bin/python
#  -*- coding: UTF-8 -*-

import os
import sys
import numpy as np
import essentia.standard as estd

# ======================= #
# KEY ESTIMATION SETTINGS #
# ======================= #

# File Settings
# -------------
SAMPLE_RATE                  = 44100
VALID_FILE_TYPES             = {'.wav', '.mp3', 'flac', '.aiff', '.ogg'}

# Analysis Parameters
# ------------------
SPECTRAL_WHITENING           = True
DETUNING_CORRECTION          = True
DETUNING_CORRECTION_SCOPE    = 'average'  # {'average', 'frame'}
WINDOW_SIZE                  = 4096
HOP_SIZE                     = 4 * WINDOW_SIZE
WINDOW_SHAPE                 = 'hann'
MIN_HZ                       = 25
MAX_HZ                       = 3500
SPECTRAL_PEAKS_THRESHOLD     = 0.0001
SPECTRAL_PEAKS_MAX           = 60
HPCP_BAND_PRESET             = False
HPCP_SPLIT_HZ                = 250       # if HPCP_BAND_PRESET is True
HPCP_HARMONICS               = 4
HPCP_NON_LINEAR              = True
HPCP_NORMALIZE               = True
HPCP_SHIFT                   = False
HPCP_REFERENCE_HZ            = 440
HPCP_SIZE                    = 36
HPCP_WEIGHT_WINDOW_SEMITONES = 1         # semitones
HPCP_WEIGHT_TYPE             = 'cosine'  # {'none', 'cosine', 'squaredCosine'}

# Scope and Key Detector Method
# -----------------------------
AVOID_TIME_EDGES             = 0         # percentage of track-length not analysed on the edges.
FIRST_N_SECS                 = 0         # analyse first n seconds of each track (0 = full track)
SKIP_FIRST_MINUTE            = False
KEY_PROFILE                  = 'edma'    # "{temperley, shaath, edma, edmm}"

# ===================== #
# FUNCTION DECLARATIONS #
# ===================== #

def results_directory(out_dir):
    """
    creates a sub-folder in the specified directory
    with some of the algorithm parameters.
    :type out_dir: str
    """
    if not os.path.isdir(out_dir):
        print "CREATING DIRECTORY '{0}'.".format(out_dir)
        if not os.path.isabs(out_dir):
            raise IOError("Not a valid path name.")
        else:
            os.mkdir(out_dir)
    return out_dir


def shift_pcp(pcp, pcp_size=12):
    """
    Shifts a pcp to the nearest tempered bin.
    :type pcp: list
    :type pcp_size: int
    """
    tuning_resolution = pcp_size / 12
    max_val = np.max(pcp)
    if max_val <= [0]:
        max_val = [1]
    pcp = np.divide(pcp, max_val)
    max_val_index = np.where(pcp == 1)
    max_val_index = max_val_index[0][0] % tuning_resolution
    if max_val_index > (tuning_resolution / 2):
        shift_distance = tuning_resolution - max_val_index
    else:
        shift_distance = max_val_index
    pcp = np.roll(pcp, shift_distance)
    return pcp


def pcp_gate(pcp, threshold):
    """
    Zeroes vector elements with values under a certain threshold.
    """
    for i in range(len(pcp)):
        if pcp[i] < threshold:
            pcp[i] = 0
    return pcp


def estimate_key(input_audio_file, output_text_file):
    """
    This function estimates the overall key of an audio track
    optionaly with extra modal information.
    :type input_audio_file: str
    :type output_text_file: str
    """
    loader = estd.MonoLoader(filename=input_audio_file,
                             sampleRate=SAMPLE_RATE)
    cut = estd.FrameCutter(frameSize=WINDOW_SIZE,
                           hopSize=HOP_SIZE)
    window = estd.Windowing(size=WINDOW_SIZE,
                            type=WINDOW_SHAPE)
    rfft = estd.Spectrum(size=WINDOW_SIZE)
    sw = estd.SpectralWhitening(maxFrequency=MAX_HZ,
                                sampleRate=SAMPLE_RATE)
    speaks = estd.SpectralPeaks(magnitudeThreshold=SPECTRAL_PEAKS_THRESHOLD,
                                maxFrequency=MAX_HZ,
                                minFrequency=MIN_HZ,
                                maxPeaks=SPECTRAL_PEAKS_MAX,
                                sampleRate=SAMPLE_RATE)
    hpcp = estd.HPCP(bandPreset=HPCP_BAND_PRESET,
                     harmonics=HPCP_HARMONICS,
                     maxFrequency=MAX_HZ,
                     minFrequency=MIN_HZ,
                     nonLinear=HPCP_NON_LINEAR,
                     normalized=HPCP_NORMALIZE,
                     referenceFrequency=HPCP_REFERENCE_HZ,
                     sampleRate=SAMPLE_RATE,
                     size=HPCP_SIZE,
                     splitFrequency=HPCP_SPLIT_HZ,
                     weightType=HPCP_WEIGHT_TYPE,
                     windowSize=HPCP_WEIGHT_WINDOW_SEMITONES,
                     maxShifted=HPCP_SHIFT)
    key = estd.KeyEDM(pcpSize=HPCP_SIZE, profileType=KEY_PROFILE)
    audio = loader()
    duration = len(audio)
    chroma = []
    if SKIP_FIRST_MINUTE and duration > (SAMPLE_RATE * 60):
        audio = audio[SAMPLE_RATE * 60:]
        duration = len(audio)
    if FIRST_N_SECS > 0:
        if duration > (FIRST_N_SECS * SAMPLE_RATE):
            audio = audio[:FIRST_N_SECS * SAMPLE_RATE]
            duration = len(audio)
    if AVOID_TIME_EDGES > 0:
        initial_sample = (AVOID_TIME_EDGES * duration) / 100
        final_sample = duration - initial_sample
        audio = audio[initial_sample:final_sample]
        duration = len(audio)
    number_of_frames = duration / HOP_SIZE
    for bang in range(number_of_frames):
        spek = rfft(window(cut(audio)))
        p1, p2 = speaks(spek)
        if SPECTRAL_WHITENING:
            p2 = sw(spek, p1, p2)
        pcp = hpcp(p1, p2)
        sum_pcp = np.sum(pcp)
        if sum_pcp > 0:
            if not DETUNING_CORRECTION or DETUNING_CORRECTION_SCOPE == 'average':
                chroma.append(pcp)
            elif DETUNING_CORRECTION and DETUNING_CORRECTION_SCOPE == 'frame':
                pcp = shift_pcp(pcp, HPCP_SIZE)
                chroma.append(pcp)
            else:
                raise NameError("SHIFT_SCOPE must be set to 'frame' or 'average'.")
    if not chroma:
        return 'Silence'
    chroma = np.sum(chroma, axis=0)
    if DETUNING_CORRECTION and DETUNING_CORRECTION_SCOPE == 'average':
        chroma = shift_pcp(list(chroma), HPCP_SIZE)
    chroma = chroma.tolist()
    estimation_1 = key(chroma)
    key = estimation_1[0] + '\t' + estimation_1[1]
    textfile = open(output_text_file, 'w')
    textfile.write(key + '\n')
    textfile.close()
    return key


if __name__ == "__main__":

    from time import clock
    from argparse import ArgumentParser

    clock()
    parser = ArgumentParser(description="Key Estimation Algorithm")
    parser.add_argument("input",
                        help="file (dir if in --batch_mode) to analyse")
    parser.add_argument("output",
                        help="file (dir if in --batch_mode) to write results to")
    parser.add_argument("-b", "--batch_mode",
                        action="store_true",
                        help="batch analyse a whole directory")
    parser.add_argument("-v", "--verbose",
                        action="store_true",
                        help="print progress to console")
    parser.add_argument("-p", "--profile",
                        help="specify a key template (temperley, shaat, edma, edmm). Defaults to edma")
    args = parser.parse_args()

    if args.profile:
        KEY_PROFILE = args.profile
    if args.verbose:
        print("key profile used:", KEY_PROFILE)

    if not args.batch_mode:
        if not os.path.isfile(args.input):
            print "\nWARNING:"
            print "Could not find {0}".format(args.input)
            print "Are you sure is it a valid filename?\n"
            sys.exit()
        elif os.path.isfile(args.input):
            print "\nAnalysing:\t{0}".format(args.input)
            print "Exporting to:\t{0}.".format(args.output)
            estimation = estimate_key(args.input, args.output)
            if args.verbose:
                print ":\t{0}".format(estimation),
        else:
            raise IOError("Unknown ERROR in single file mode")
    else:
        if os.path.isdir(args.input):
            analysis_folder = args.input[1 + args.input.rfind('/'):]
            if os.path.isfile(args.output):
                print "\nWARNING:"
                print "It seems that you are trying to write onto an existing file"
                print "In batch_mode, the output argument must be a directory".format(args.output)
                print "Type 'Fkey3 -h' for help\n"
                sys.exit()
            output_dir = results_directory(args.output)
            list_all_files = os.listdir(args.input)
            print "\nAnalysing audio files in:\t{0}".format(args.input)
            print "Writing estimation files to:\t{0}\n".format(args.output)
            count_files = 0
            for a_file in list_all_files:
                if any(soundfile_type in a_file for soundfile_type in VALID_FILE_TYPES):
                    input_file = args.input + '/' + a_file
                    output_file = args.output + '/' + a_file[:-4] + '.txt'
                    estimation = estimate_key(input_file, output_file)
                    if args.verbose:
                        print "{0} - {1}".format(input_file, estimation)
                    count_files += 1
            print "{0} audio files analysed".format(count_files, clock())
        else:
            raise IOError("Unknown ERROR in batch mode")
    print "Finished in:\t{0} secs.\n".format(clock())