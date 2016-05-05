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

#ifndef ESSENTIA_KEY2_H
#define ESSENTIA_KEY2_H

#include "algorithm.h"

namespace essentia {
namespace standard {

class Key2 : public Algorithm {

 private:
  Input<std::vector<Real> > _pcp;

  Output<std::string> _key;
  Output<std::string> _scale;
  Output<Real> _strength;
  Output<Real> _firstToSecondRelativeStrength;

 public:

  Key2() {
    declareInput(_pcp, "pcp", "the input pitch class profile");

    declareOutput(_key, "key", "the estimated key, from A to G");
    declareOutput(_scale, "scale", "the scale of the key (major or minor)");
    declareOutput(_strength, "strength", "the strength of the estimated key");
    declareOutput(_firstToSecondRelativeStrength, "firstToSecondRelativeStrength", "the relative strength difference between the best estimate and second best estimate of the key");
  }

  void declareParameters() {
    declareParameter("profileType", "the type of polyphic profile to use for correlation calculation", "{bmtg1,bmtg2,bmtg3,edma}", "bmtg2");
    declareParameter("pcpSize", "number of divisions per octave (12*i). This parameter is only a hint; During computation the size of the input PCP is used instead)", "[12,inf)", 36);
  }

  void compute();
  void configure();

  static const char* name;
  static const char* description;

protected:
  enum Scales {
    MAJOR   = 0,
    MINOR   = 1,
    OTHER   = 2,
    // PEAK    = 3,
    // FLAT    = 4
  };

  std::vector<Real> _M;
  std::vector<Real> _m;
  std::vector<Real> _O;
  // std::vector<Real> _P;
  // std::vector<Real> _F;
  std::vector<Real> _profile_doM;
  std::vector<Real> _profile_dom;
  std::vector<Real> _profile_doO;
  // std::vector<Real> _profile_doP;
  // std::vector<Real> _profile_doF;

  Real _mean_profile_M;
  Real _mean_profile_m;
  Real _mean_profile_O;
  // Real _mean_profile_P;
  // Real _mean_profile_F;
  Real _std_profile_M;
  Real _std_profile_m;
  Real _std_profile_O;
  // Real _std_profile_P;
  // Real _std_profile_F;

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

class Key2 : public AlgorithmComposite {
 protected:
  Sink<std::vector<Real> > _pcp;

  Source<std::string> _key;
  Source<std::string> _scale;
  Source<Real> _strength;

  Pool _pool;
  Algorithm* _poolStorage;
  standard::Algorithm* _key2Algo;

 public:
  Key2();
  ~Key2();

  void declareParameters() {
    declareParameter("profileType", "the type of polyphic profile to use for correlation calculation", "{bmtg1,bmtg2,bmtg3,edma}", "bmtg2");
    declareParameter("pcpSize", "number of divisions per octave (12*i). This parameter is only a hint; During computation the size of the input PCP is used instead)", "[12,inf)", 36);
  }

  void configure() {
    _key2Algo->configure(INHERIT("profileType"),
                         INHERIT("pcpSize"));
  }

  void declareProcessOrder() {
    declareProcessStep(SingleShot(_poolStorage));
    declareProcessStep(SingleShot(this));
  }

  AlgorithmStatus process();
  void reset();

  static const char* name;
  static const char* description;

};

} // namespace streaming
} // namespace essentia

#endif // ESSENTIA_KEY2_H
