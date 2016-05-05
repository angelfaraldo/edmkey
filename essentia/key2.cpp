/*
 * Copyright (C) 2006-2013  Music Technology Group - Universitat Pompeu Fabra
 *
 * This file is part of Essentia
 *
 * Essentia is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation (FSF), either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the Affero GNU General Public License
 * version 3 along with this program.  If not, see http://www.gnu.org/licenses/
 */

#include "key2.h"
#include "essentiamath.h"

using namespace std;

namespace essentia {
namespace standard {

const char* Key2::name = "Key2";
const char* Key2::description = DOC("Using pitch profile classes, this algorithm calculates the best matching key estimate for a given HPCP. The algorithm was severely adapted and changed from the original implementation for readability and speed.\n"
"\n"
"Key will throw exceptions either when the input pcp size is not a positive multiple of 12 or if the key could not be found. Also if parameter \"scale\" is set to \"minor\" and the profile type is set to \"weichai\"\n"
"\n"
"  Abouth the Key Profiles:\n"
"  - 'Diatonic' - binary profile with diatonic notes of both modes. Could be useful for ambient music or diatonic music which is not strictly 'tonal functional'.\n"
"  - 'Tonic Triad' - just the notes of the major and minor chords. Exclusively for testing.\n"
"  - 'Krumhansl' - reference key profiles after cognitive experiments with users. They should work generally fine for pop music.\n"
"  - 'Temperley' - key profiles extracted from corpus analysis of euroclassical music. Therefore, they perform best on this repertoire (especially in minor).\n"
"  - 'Shaath' -  profiles based on Krumhansl's specifically tuned to popular and electronic music.\n"
"  - 'Noland' - profiles from Bach's 'Well Tempered Klavier'.\n" 
"  - 'edma' - automatic profiles extracted from corpus analysis of electronic dance music [3]. They normally perform better that Shaath's\n"
"  - 'edmm' - automatic profiles extracted from corpus analysis of electronic dance music and manually tweaked according to heuristic observation. It will report major modes (which are poorly represented in EDM) as minor, but improve performance otherwise [3].\n"

"References:\n"
"  [1] E. Gómez, \"Tonal Description of Polyphonic Audio for Music Content\n"
"  Processing,\" INFORMS Journal on Computing, vol. 18, no. 3, pp. 294–304,\n"
"  2006.\n\n"
"  [2] D. Temperley, \"What's key for key? The Krumhansl-Schmuckler\n"
"  key-finding algorithm reconsidered\", Music Perception vol. 17, no. 1,\n"
"  pp. 65-100, 1999.\n"
"  [3] Á. Faraldo, E. Gómez, S. Jordà, P.Herrera, \"Key Estimation in Electronic\n"
"  Dance Music. Proceedings of the 38th International Conference on information\n"
"  Retrieval, Padova, 2016.");


void Key2::configure() {

  _profileType = parameter("profileType").toString();

  const char* keyNames[] = { "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab" };
  _keys = arrayToVector<string>(keyNames);

  Real profileTypes[][12] = {

//    I       bII     II      bIII    III     IV      #IV     V       bVI     VI      bVII    VII
    { 1.0000, 0.1573, 0.4200, 0.1570, 0.5296, 0.3669, 0.1632, 0.7711, 0.1676, 0.3827, 0.2113, 0.2965 }, // bmtg1
    { 1.0000, 0.2330, 0.3615, 0.3905, 0.2925, 0.3777, 0.1961, 0.7425, 0.2701, 0.2161, 0.4228, 0.2272 },
    { 1.0000, 0.2608, 0.3528, 0.2935, 0.4393, 0.3580, 0.2137, 0.7809, 0.2578, 0.2539, 0.3233, 0.2615 },

    { 1.00  , 0.10  , 0.42  , 0.10  , 0.53  , 0.37  , 0.10  , 0.77  , 0.10  , 0.38  , 0.21  , 0.30   }, // bmtg2
    { 1.00  , 0.10  , 0.36  , 0.39  , 0.29  , 0.38  , 0.10  , 0.74  , 0.27  , 0.10  , 0.42  , 0.23   },
    { 1.00  , 0.26  , 0.35  , 0.29  , 0.44  , 0.36  , 0.21  , 0.78  , 0.26  , 0.25  , 0.32  , 0.26   },
    
    { 1.00  , 0.00  , 0.42  , 0.00  , 0.53  , 0.37  , 0.00  , 0.76  , 0.00  , 0.38,   0.21  , 0.30   }, // bmtg3
    { 1.00  , 0.00  , 0.36  , 0.39  , 0.10  , 0.37  , 0.00  , 0.76  , 0.27  , 0.00  , 0.42  , 0.23   },
    { 1.00  , 0.26  , 0.35  , 0.29  , 0.44  , 0.37  , 0.21  , 0.76  , 0.26  , 0.25  , 0.32  , 0.26   },

    { 1.00  , 0.29  , 0.50  , 0.40  , 0.60  , 0.56  , 0.32  , 0.80  , 0.31  , 0.45  , 0.42  , 0.39   }, // edma
    { 1.00  , 0.31  , 0.44  , 0.58  , 0.33  , 0.49  , 0.29  , 0.78  , 0.43  , 0.29  , 0.53  , 0.32   },
    { 1.00  , 0.26  , 0.35  , 0.29  , 0.44  , 0.36  , 0.21  , 0.78  , 0.26  , 0.25  , 0.32  , 0.26   }
//    I       bII     II      bIII    III     IV      #IV     V       bVI     VI      bVII    VII 

};

#define SET_PROFILE(i) _M = arrayToVector<Real>(profileTypes[3*i]); _m = arrayToVector<Real>(profileTypes[3*i+1]); _O = arrayToVector<Real>(profileTypes[3*i+2])//; _P = arrayToVector<Real>(profileTypes[5*i+3]); _F = arrayToVector<Real>(profileTypes[5*i+4])

  if      (_profileType == "bmtg1") { SET_PROFILE(0); }
  else if (_profileType == "bmtg2") { SET_PROFILE(1); }
  else if (_profileType == "bmtg3") { SET_PROFILE(2); }
  else if (_profileType == "edma" ) { SET_PROFILE(3); }
  else {
    throw EssentiaException("Key2: Unsupported profile type: ", _profileType);
  }
 
  resize(parameter("pcpSize").toInt());
}


void Key2::compute() {

  const vector<Real>& pcp = _pcp.get();

  int pcpsize = (int)pcp.size();
  int n = pcpsize/12;

  if (pcpsize < 12 || pcpsize % 12 != 0)
    throw EssentiaException("Key2: input PCP size is not a positive multiple of 12");

  if (pcpsize != (int)_profile_dom.size()) {
    resize(pcpsize);
  }

  // Compute Correlation
  // Means
  Real mean_pcp = mean(pcp);
  Real std_pcp = 0;

  // Standard Deviations
  for (int i=0; i<pcpsize; i++)
    std_pcp += (pcp[i] - mean_pcp) * (pcp[i] - mean_pcp);
  std_pcp = sqrt(std_pcp);

  // Correlation Matrix
  int keyIndex = -1; // index of the first maximum
  Real max     = -1;     // first maximum
  Real max2    = -1;    // second maximum
  int scale    = MAJOR;  // scale

  // Compute maximum for major, minor, flat, peak and other.
  Real maxMajor     = -1;
  Real max2Major    = -1;
  int keyIndexMajor = -1;

  Real maxMinor     = -1;
  Real max2Minor    = -1;
  int keyIndexMinor = -1;

  Real maxOther     = -1;
  Real max2Other    = -1;
  int keyIndexOther = -1;

	// Real maxPeak = -1;
 //  Real max2Peak = -1;
 //  int keyIndexPeak = -1;

 //  Real maxFlat = -1;
 //  Real max2Flat = -1;
 //  int keyIndexFlat = -1;


  // calculate the correlation between the profiles and the PCP...
  // we shift the profile around to find the best match
  for (int shift=0; shift<pcpsize; shift++) {
    Real corrMajor = correlation(pcp, mean_pcp, std_pcp, _profile_doM, _mean_profile_M, _std_profile_M, shift);
    // Compute maximum value for major keys
    if (corrMajor > maxMajor) {
      max2Major = maxMajor;
      maxMajor = corrMajor;
      keyIndexMajor = shift;
    }

    Real corrMinor = correlation(pcp, mean_pcp, std_pcp, _profile_dom, _mean_profile_m, _std_profile_m, shift);
    // Compute maximum value for minor keys
    if (corrMinor > maxMinor) {
      max2Minor = maxMinor;
      maxMinor = corrMinor;
      keyIndexMinor = shift;
    }

    Real corrOther = correlation(pcp, mean_pcp, std_pcp, _profile_doO, _mean_profile_O, _std_profile_O, shift);
    // Compute maximum value for major keys
    if (corrOther > maxOther) {
      max2Other = maxOther;
      maxOther = corrOther;
      keyIndexOther = shift;
    }

  //   Real corrPeak = correlation(pcp, mean_pcp, std_pcp, _profile_doP, _mean_profile_P, _std_profile_P, shift);
  //   // Compute maximum value for major keys
  //   if (corrPeak > maxPeak) {
  //     max2Peak = maxPeak;
  //     maxPeak = corrPeak;
  //     keyIndexPeak = shift;
  //   }

		// Real corrFlat = correlation(pcp, mean_pcp, std_pcp, _profile_doF, _mean_profile_F, _std_profile_F, shift);
  //   // Compute maximum value for major keys
  //   if (corrFlat > maxFlat) {
  //     max2Flat = maxFlat;
  //     maxFlat = corrFlat;
  //     keyIndexFlat = shift;
  //   }
  }



  if (maxMajor > maxMinor && maxMajor > maxOther) {  // && maxMajor > maxPeak && maxMajor > maxFlat
    keyIndex = (int) (keyIndexMajor *  12 / pcpsize + 0.5);
    scale = MAJOR;
    max = maxMajor;
    max2 = max2Major;
  }

  else if (maxMinor >= maxMajor && maxMinor >= maxOther) {  // && maxMinor > maxPeak && maxMinor > maxFlat
    keyIndex = (int) (keyIndexMinor * 12 / pcpsize + 0.5);
    scale = MINOR;
    max = maxMinor;
    max2 = max2Minor;
    }

	else if (maxOther > maxMajor && maxOther > maxMinor) {   // && maxOther > maxPeak && maxOther > maxFlat
    keyIndex = (int) (keyIndexOther * 12 / pcpsize + 0.5);
    scale = OTHER;
    max = maxOther;
    max2 = max2Other;
    }
  
  // else if (maxPeak > maxMajor && maxPeak > maxMinor && maxPeak > maxOther && maxPeak > maxFlat) {
  //   keyIndex = (int) (keyIndexPeak * 12 / pcpsize + 0.5);
  //   scale = PEAK;
  //   max = maxPeak;
  //   max2 = max2Peak;
  //   }

  // else {
  //   keyIndex = (int) (keyIndexFlat * 12 / pcpsize + 0.5);
  //   scale = FLAT;
  //   max = maxFlat;
  //   max2 = max2Flat;
  // }


  if (keyIndex < 0) {
    throw EssentiaException("Key2: keyIndex smaller than zero. Could not find key.");
  }

  //////////////////////////////////////////////////////////////////////////////
  // Here we calculate the outputs...

  // first three outputs are key, scale and strength
  _key.get() = _keys[keyIndex];

  if (scale == MAJOR) {
    _scale.get() = "major";
  }

  else if (scale == MINOR) {
    _scale.get() = "minor";
  }

  else if (scale == OTHER) {
    _scale.get() = "minor";
  }

 //  else if (scale == PEAK) {
 //    _scale.get() = "monotonic"; // monotonic
 //  }

	// else {
 //    _scale.get() = "noisy-or-atonal"; // noise/atonal
  //}

  _strength.get() = max;

  // this one outputs the relative difference between the maximum and the
  // second highest maximum (i.e. Compute second highest correlation peak)
  _firstToSecondRelativeStrength.get() = (max - max2) / max;

}

// this function resizes and interpolates the profiles to fit the
// pcp size...
void Key2::resize(int pcpsize) {
  ///////////////////////////////////////////////////////////////////
  // Interpolate to get pcpsize values
  int n = pcpsize/12;

  _profile_doM.resize(pcpsize);
  _profile_dom.resize(pcpsize);
  _profile_doO.resize(pcpsize);
  // _profile_doP.resize(pcpsize);
  // _profile_doF.resize(pcpsize);

  for (int i=0; i<12; i++) {
    _profile_doM[i*n] = _M[i];
    _profile_dom[i*n] = _m[i];
    _profile_doO[i*n] = _O[i];
    // _profile_doP[i*n] = _P[i];
    // _profile_doF[i*n] = _F[i];

    // Two interpolated values
    Real incr_M, incr_m, incr_O, incr_P, incr_F;
    if (i == 11) {
      incr_M = (_M[11] - _M[0]) / n;
      incr_m = (_m[11] - _m[0]) / n;
      incr_O = (_O[11] - _O[0]) / n;
      // incr_P = (_O[11] - _P[0]) / n;
      // incr_F = (_O[11] - _F[0]) / n;
    }
    else {
      incr_M = (_M[i] - _M[i+1]) / n;
      incr_m = (_m[i] - _m[i+1]) / n;
      incr_O = (_O[i] - _O[i+1]) / n;
      // incr_P = (_P[i] - _P[i+1]) / n;
      // incr_F = (_F[i] - _F[i+1]) / n;
    }

    for (int j=1; j<=(n-1); j++) {
      _profile_doM[i*n+j] = _M[i] - j * incr_M;
      _profile_dom[i*n+j] = _m[i] - j * incr_m;
      _profile_doO[i*n+j] = _O[i] - j * incr_O;
     	// _profile_doP[i*n+j] = _P[i] - j * incr_P;
      // _profile_doF[i*n+j] = _F[i] - j * incr_F;			
    }
  }

  _mean_profile_M = mean(_profile_doM);
  _mean_profile_m = mean(_profile_dom);
  _mean_profile_O = mean(_profile_doO);
  // _mean_profile_P = mean(_profile_doP);
  // _mean_profile_F = mean(_profile_doF);
  _std_profile_M = 0;
  _std_profile_m = 0;
  _std_profile_O = 0;
  // _std_profile_P = 0;
  // _std_profile_F = 0;

  // Compute Standard Deviations
  for (int i=0; i<pcpsize; i++) {
    _std_profile_M += (_profile_doM[i] - _mean_profile_M) * (_profile_doM[i] - _mean_profile_M);
    _std_profile_m += (_profile_dom[i] - _mean_profile_m) * (_profile_dom[i] - _mean_profile_m);
    _std_profile_O += (_profile_doO[i] - _mean_profile_O) * (_profile_doO[i] - _mean_profile_O);
    // _std_profile_P += (_profile_doP[i] - _mean_profile_P) * (_profile_doP[i] - _mean_profile_P);
    // _std_profile_F += (_profile_doF[i] - _mean_profile_F) * (_profile_doF[i] - _mean_profile_F);
  }
  _std_profile_M = sqrt(_std_profile_M);
  _std_profile_m = sqrt(_std_profile_m);
  _std_profile_O = sqrt(_std_profile_O);
  //_std_profile_P = sqrt(_std_profile_P);
  //_std_profile_F = sqrt(_std_profile_F);
}


// correlation coefficient with 'shift'
// one of the vectors is shifted in time, and then the correlation is calculated,
// just like a cross-correlation
Real Key2::correlation(const vector<Real>& v1, const Real mean1, const Real std1, const vector<Real>& v2, const Real mean2, const Real std2, const int shift) const
{
  Real r = 0.0;
  int size = (int)v1.size();

  for (int i=0; i<size; i++)
  {
    int index = (i - shift) % size;

    if (index < 0) {
      index += size;
    }

    r += (v1[i] - mean1) * (v2[index] - mean2);
  }

  r /= std1*std2;

  return r;
}


} // namespace standard
} // namespace essentia

#include "poolstorage.h"
#include "algorithmfactory.h"

namespace essentia {
namespace streaming {

const char* Key2::name = standard::Key2::name;
const char* Key2::description = standard::Key2::description;

Key2::Key2() : AlgorithmComposite() {

  _key2Algo = standard::AlgorithmFactory::create("Key2");
  _poolStorage = new PoolStorage<std::vector<Real> >(&_pool, "internal.hpcp");

  declareInput(_poolStorage->input("data"), 1, "pcp", "the input pitch class profile");

  declareOutput(_key, 0, "key", "the estimated key, from A to G");
  declareOutput(_scale, 0, "scale", "the scale of the key (major, minor or unknown)");
  declareOutput(_strength, 0, "strength", "the strength of the estimated key");
}

Key2::~Key2() {
  delete _key2Algo;
  delete _poolStorage;
}


AlgorithmStatus Key2::process() {
  if (!shouldStop()) return PASS;

  const vector<vector<Real> >& hpcpKey = _pool.value<vector<vector<Real> > >("internal.hpcp");
  vector<Real> hpcpAverage = meanFrames(hpcpKey);
  string key;
  string scale;
  Real strength;
  Real firstToSecondRelativeStrength;
  _key2Algo->configure("profileType", "bmtg2");
  _key2Algo->input("pcp").set(hpcpAverage);
  _key2Algo->output("key").set(key);
  _key2Algo->output("scale").set(scale);
  _key2Algo->output("strength").set(strength);
  _key2Algo->output("firstToSecondRelativeStrength").set(firstToSecondRelativeStrength);
  _key2Algo->compute();

  _key.push(key);
  _scale.push(scale);
  _strength.push(strength);

  return FINISHED;
}


void Key2::reset() {
  AlgorithmComposite::reset();
  _key2Algo->reset();
}

} // namespace streaming
} // namespace essentia
