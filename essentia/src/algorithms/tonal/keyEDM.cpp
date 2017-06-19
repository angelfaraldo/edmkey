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

#include "keyEDM.h"
#include "essentiamath.h"

using namespace std;

namespace essentia {
namespace standard {

const char* KeyEDM::name = "KeyEDM";
const char* KeyEDM::category = "Tonal";
const char* KeyEDM::description = DOC("Using pitch profile classes, this algorithm calculates the best matching key estimate for a given HPCP. The algorithm was severely adapted and changed from the original implementation for readability and speed.\n"
"\n"
"Key will throw exceptions either when the input pcp size is not a positive multiple of 12 or if the key could not be found."
"\n"
"  Abouth the Key Profiles:\n"
"  - 'edma' - automatic profiles extracted from corpus analysis of electronic dance music [2]. They normally perform better that Shaath's\n"
"  - 'edmm' - automatic profiles extracted from corpus analysis of electronic dance music and manually tweaked according to heuristic observation. It will report major modes (which are poorly represented in EDM) as minor, but improve performance otherwise [2].\n"

"References:\n"
"  [1] E. Gómez, \"Tonal Description of Polyphonic Audio for Music Content\n"
"  Processing,\" INFORMS Journal on Computing, vol. 18, no. 3, pp. 294–304,\n"
"  2006.\n\n"
"  [2] Á. Faraldo, E. Gómez, S. Jordà, P.Herrera, \"Key Estimation in Electronic\n"
"  Dance Music. Proceedings of the 38th International Conference on information\n"
"  Retrieval, Padova, 2016.");


void KeyEDM::configure() {

  _profileType = parameter("profileType").toString();

  const char* keyNames[] = { "A", "Bb", "B", "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab" };
  _keys = arrayToVector<string>(keyNames);

  Real profileTypes[][12] = {

     // A revised version of the key profiles, by David Temperley, see [2]
    { 5.0, 2.0, 3.5, 2.0, 4.5, 4.0, 2.0, 4.5, 2.0, 3.5, 1.5, 4.0 },
    { 5.0, 2.0, 3.5, 4.5, 2.0, 4.0, 2.0, 4.5, 3.5, 2.0, 1.5, 4.0 },

    // Shaath
    { 6.6, 2.0, 3.5, 2.3, 4.6, 4.0, 2.5, 5.2, 2.4, 3.7, 2.3, 3.4 }, 
    { 6.5, 2.7, 3.5, 5.4, 2.6, 3.5, 2.5, 5.2, 4.0, 2.7, 4.3, 3.2 },

    // edma
    { 0.16519551, 0.04749026, 0.08293076, 0.06687112, 0.09994645, 0.09274123, 0.05294487, 0.13159476, 0.05218986, 0.07443653, 0.06940723, 0.0642515  },
    { 0.17235348, 0.05336489, 0.0761009,  0.10043649, 0.05621498, 0.08527853, 0.0497915,  0.13451001, 0.07458916, 0.05003023, 0.09187879, 0.05545106 },

    // edmm
    { 0.083, 0.083, 0.083, 0.083, 0.083, 0.083, 0.083, 0.083, 0.083, 0.083, 0.083, 0.083 },
    { 0.17235348, 0.04, 0.0761009,  0.12, 0.05621498, 0.08527853, 0.0497915,  0.13451001, 0.07458916, 0.05003023, 0.09187879, 0.05545106 },

};

#define SET_PROFILE(i) _M = arrayToVector<Real>(profileTypes[2*i]); _m = arrayToVector<Real>(profileTypes[2*i+1])

  if      (_profileType == "temperley")  { SET_PROFILE(0);  }
  else if (_profileType == "shaath")     { SET_PROFILE(1);  }
  else if (_profileType == "edma")       { SET_PROFILE(2);  }
  else if (_profileType == "edmm")       { SET_PROFILE(3);  }
  else {
    throw EssentiaException("KeyEDM: Unsupported profile type: ", _profileType);
  }
 
  resize(parameter("pcpSize").toInt());
}


void KeyEDM::compute() {

  const vector<Real>& pcp = _pcp.get();

  int pcpsize = (int)pcp.size();
  int n = pcpsize/12;

  if (pcpsize < 12 || pcpsize % 12 != 0)
    throw EssentiaException("KeyEDM: input PCP size is not a positive multiple of 12");

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
  Real max = -1;     // first maximum
  Real max2 = -1;    // second maximum
  int scale = MAJOR; // scale

  // Compute maximum for both major and minor
  Real maxMaj = -1;
  Real max2Maj = -1;
  int keyIndexMaj = -1;

  Real maxMin = -1;
  Real max2Min = -1;
  int keyIndexMin = -1;

  // calculate the correlation between the profiles and the PCP...
  // we shift the profile around to find the best match
  for (int shift=0; shift<pcpsize; shift++) {
    Real corrMajor = correlation(pcp, mean_pcp, std_pcp, _profile_doM, _mean_profile_M, _std_profile_M, shift);
    // Compute maximum value for major keys
    if (corrMajor > maxMaj) {
      max2Maj = maxMaj;
	      maxMaj = corrMajor;
      keyIndexMaj = shift;
    }

    Real corrMinor = correlation(pcp, mean_pcp, std_pcp, _profile_dom, _mean_profile_m, _std_profile_m, shift);
    // Compute maximum value for minor keys
    if (corrMinor > maxMin) {
      max2Min = maxMin;
      maxMin = corrMinor;
      keyIndexMin = shift;
    }
  }

  if (maxMaj >= maxMin) {
    keyIndex = (int) (keyIndexMaj *  12 / pcpsize + .5);
    scale = MAJOR;
    max = maxMaj;
    max2 = max2Maj;
  }
  else {
    keyIndex = (int) (keyIndexMin * 12 / pcpsize + .5);
    scale = MINOR;
    max = maxMin;
    max2 = max2Min;
  }

  if (keyIndex < 0) {
    throw EssentiaException("KeyEDM: keyIndex smaller than zero. Could not find key.");
  }

  //////////////////////////////////////////////////////////////////////////////
  // Here we calculate the outputs...
  // first three outputs are key, scale and strength
  _key.get() = _keys[keyIndex];
  _scale.get() = scale == MAJOR ? "major" : "minor";
  _strength.get() = max;

  // this one outputs the relative difference between the maximum and the
  // second highest maximum (i.e. Compute second highest correlation peak)
  _firstToSecondRelativeStrength.get() = (max - max2) / max;

}

// this function resizes and interpolates the profiles to fit the
// pcp size...
void KeyEDM::resize(int pcpsize) {
  ///////////////////////////////////////////////////////////////////
  // Interpolate to get pcpsize values
  int n = pcpsize/12;

  _profile_doM.resize(pcpsize);
  _profile_dom.resize(pcpsize);

  for (int i=0; i<12; i++) {

    _profile_doM[i*n] = _M[i];
    _profile_dom[i*n] = _m[i];

    // Two interpolated values
    Real incr_M, incr_m;
    if (i == 11) {
      incr_M = (_M[11] - _M[0]) / n;
      incr_m = (_m[11] - _m[0]) / n;
    }
    else {
      incr_M = (_M[i] - _M[i+1]) / n;
      incr_m = (_m[i] - _m[i+1]) / n;
    }

    for (int j=1; j<=(n-1); j++) {
      _profile_doM[i*n+j] = _M[i] - j * incr_M;
      _profile_dom[i*n+j] = _m[i] - j * incr_m;
    }
  }

  _mean_profile_M = mean(_profile_doM);
  _mean_profile_m = mean(_profile_dom);
  _std_profile_M = 0;
  _std_profile_m = 0;

  // Compute Standard Deviations
  for (int i=0; i<pcpsize; i++) {
    _std_profile_M += (_profile_doM[i] - _mean_profile_M) * (_profile_doM[i] - _mean_profile_M);
    _std_profile_m += (_profile_dom[i] - _mean_profile_m) * (_profile_dom[i] - _mean_profile_m);
  }
  _std_profile_M = sqrt(_std_profile_M);
  _std_profile_m = sqrt(_std_profile_m);
}

// correlation coefficient with 'shift'
// on of the vectors is shifted in time, and then the correlation is calculated,
// just like a cross-correlation
Real KeyEDM::correlation(const vector<Real>& v1, const Real mean1, const Real std1, const vector<Real>& v2, const Real mean2, const Real std2, const int shift) const
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

const char* KeyEDM::name = standard::KeyEDM::name;
const char* KeyEDM::category = standard::KeyEDM::category;
const char* KeyEDM::description = standard::KeyEDM::description;

KeyEDM::KeyEDM() : AlgorithmComposite() {

  _keyEDMAlgo = standard::AlgorithmFactory::create("KeyEDM");
  _poolStorage = new PoolStorage<std::vector<Real> >(&_pool, "internal.hpcp");

  declareInput(_poolStorage->input("data"), 1, "pcp", "the input pitch class profile");
  
  declareOutput(_key, 0, "key", "the estimated key, from A to G");
  declareOutput(_scale, 0, "scale", "the scale of the key (major or minor)");
  declareOutput(_strength, 0, "strength", "the strength of the estimated key");
}

KeyEDM::~KeyEDM() {
  delete _keyEDMAlgo;
  delete _poolStorage;
}


AlgorithmStatus KeyEDM::process() {
  if (!shouldStop()) return PASS;

  const vector<vector<Real> >& hpcpKey = _pool.value<vector<vector<Real> > >("internal.hpcp");
  vector<Real> hpcpAverage = meanFrames(hpcpKey);
  string key;
  string scale;
  Real strength;
  Real firstToSecondRelativeStrength;
  _keyEDMAlgo->configure("profileType", "edma");
  _keyEDMAlgo->input("pcp").set(hpcpAverage);
  _keyEDMAlgo->output("key").set(key);
  _keyEDMAlgo->output("scale").set(scale);
  _keyEDMAlgo->output("strength").set(strength);
  _keyEDMAlgo->output("firstToSecondRelativeStrength").set(firstToSecondRelativeStrength);
  _keyEDMAlgo->compute();

  _key.push(key);
  _scale.push(scale);
  _strength.push(strength);

  return FINISHED;
}


void KeyEDM::reset() {
  AlgorithmComposite::reset();
  _keyEDMAlgo->reset();
}


} // namespace streaming
} // namespace essentia