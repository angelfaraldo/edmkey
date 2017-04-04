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

#ifndef ESSENTIA_KEYEDM_H
#define ESSENTIA_KEYEDM_H

#include "algorithm.h"

namespace essentia {
namespace standard {

class KeyEDM : public Algorithm {

 private:
  Input<std::vector<Real> > _pcp;

  Output<std::string> _key;
  Output<std::string> _scale;
  Output<Real> _strength;
  Output<Real> _firstToSecondRelativeStrength;

 public:

  KeyEDM() {
    declareInput(_pcp, "pcp", "the input pitch class profile");

    declareOutput(_key, "key", "the estimated key, from A to G");
    declareOutput(_scale, "scale", "the scale of the key (major or minor)");
    declareOutput(_strength, "strength", "the strength of the estimated key");
    declareOutput(_firstToSecondRelativeStrength, "firstToSecondRelativeStrength", "the relative strength difference between the best estimate and second best estimate of the key");
  }

  void declareParameters() {
    declareParameter("profileType", "the type of polyphic profile to use for correlation calculation", "{edma,edmm,bmtg1,bmtg2,bmtg3}", "edma");
    declareParameter("pcpSize", "number of array elements used to represent a semitone times 12 (this parameter is only a hint, during computation, the size of the input PCP is used instead)", "[12,inf)", 36);
  }


  void compute();
  void configure();

  static const char* name;
  static const char* category;
  static const char* description;

protected:
  enum Scales {
    MAJOR = 0,
    MINOR = 1
  };

  std::vector<Real> _m;
  std::vector<Real> _M;
  std::vector<Real> _profile_doM;
  std::vector<Real> _profile_dom;

  Real _mean_profile_M;
  Real _mean_profile_m;
  Real _std_profile_M;
  Real _std_profile_m;

  std::string _profileType;

  std::vector<std::string> _keys;

  Real correlation(const std::vector<Real>& v1, const Real mean1, const Real std1, const std::vector<Real>& v2, const Real mean2, const Real std2, const int shift) const;
  void resize(int size);
};

} // namespace standard
} // namespace essentia


#include "streamingalgorithmcomposite.h"
#include "pool.h"

namespace essentia {
namespace streaming {

class KeyEDM : public AlgorithmComposite {
 protected:
  Sink<std::vector<Real> > _pcp;

  Source<std::string> _key;
  Source<std::string> _scale;
  Source<Real> _strength;

  Pool _pool;
  Algorithm* _poolStorage;
  standard::Algorithm* _keyEDMAlgo;

 public:
  KeyEDM();
  ~KeyEDM();

  void declareParameters() {
    declareParameter("profileType", "the type of polyphohic profile to use for correlation calculation", "{edma,edmm,bmtg1,bmtg2,bmtg3}", "edma");
    declareParameter("pcpSize", "number of array elements used to represent a semitone times 12 (this parameter is only a hint, during computation, the size of the input PCP is used instead)", "[12,inf)", 36);
  }

  void configure() {
    _keyEDMAlgo->configure(INHERIT("profileType"),
                           INHERIT("pcpSize"));
  }

  void declareProcessOrder() {
    declareProcessStep(SingleShot(_poolStorage));
    declareProcessStep(SingleShot(this));
  }

  AlgorithmStatus process();
  void reset();

  static const char* name;
  static const char* category;
  static const char* description;

};

} // namespace streaming
} // namespace essentia

#endif // ESSENTIA_KEYEDM_H
