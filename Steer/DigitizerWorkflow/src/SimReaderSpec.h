// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef O2_STEER_SIMREADERSPEC_H
#define O2_STEER_SIMREADERSPEC_H

#include "Framework/DataProcessorSpec.h"

namespace o2
{
namespace steer
{
struct SubspecRange {
  int min = 0;
  int max = 0;
};

o2::framework::DataProcessorSpec getSimReaderSpec(SubspecRange range, const std::vector<std::string>& simprefixes, const std::vector<int>& tpcsectors);
} // namespace steer
} // namespace o2

#endif // O2_STEER_SIMREADERSPEC_H
