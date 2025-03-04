// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "TOFCalibration/LHCClockCalibrator.h"
#include "Framework/Logger.h"
#include "MathUtils/fit.h"
#include "CommonUtils/MemFileHelper.h"
#include "CCDB/CcdbApi.h"
#include "DetectorsCalibration/Utils.h"
#include "DetectorsRaw/HBFUtils.h"

namespace o2
{
namespace tof
{

using Slot = o2::calibration::TimeSlot<o2::tof::LHCClockDataHisto>;
using o2::math_utils::fitGaus;
using LHCphase = o2::dataformats::CalibLHCphaseTOF;
using clbUtils = o2::calibration::Utils;

//_____________________________________________
LHCClockDataHisto::LHCClockDataHisto()
{
  LOG(info) << "Default c-tor, not to be used";
}

//_____________________________________________
void LHCClockDataHisto::fill(const gsl::span<const o2::dataformats::CalibInfoTOF> data)
{
  // fill container
  for (int i = data.size(); i--;) {
    auto ch = data[i].getTOFChIndex();
    auto dt = data[i].getDeltaTimePi();
    auto tot = data[i].getTot();
    auto corr = calibApi->getTimeCalibration(ch, tot); // we take into account LHCphase, offsets and time slewing
    dt -= corr;

    //    printf("ch=%d - tot=%f - corr=%f -> dtcorr = %f (range=%f, bin=%d)\n",ch,tot,corr,dt,range,int((dt+range)*v2Bin));

    dt += range;
    if (dt > 0 && dt < 2 * range) {
      histo[int(dt * v2Bin)]++;
      entries++;
    }
  }
}

//_____________________________________________
void LHCClockDataHisto::merge(const LHCClockDataHisto* prev)
{
  // merge data of 2 slots
  for (int i = histo.size(); i--;) {
    histo[i] += prev->histo[i];
  }
  entries += prev->entries;
}

//_____________________________________________
void LHCClockDataHisto::print() const
{
  LOG(info) << entries << " entries";
}

//===================================================================

//_____________________________________________
void LHCClockCalibrator::initOutput()
{
  // Here we initialize the vector of our output objects
  mInfoVector.clear();
  mLHCphaseVector.clear();
  return;
}

//_____________________________________________
void LHCClockCalibrator::finalizeSlot(Slot& slot)
{
  // Extract results for the single slot
  o2::tof::LHCClockDataHisto* c = slot.getContainer();
  LOG(info) << "Finalize slot " << slot.getTFStart() << " <= TF <= " << slot.getTFEnd() << " with "
            << c->getEntries() << " entries";
  std::array<double, 3> fitValues;
  double fitres = fitGaus(c->nbins, c->histo.data(), -(c->range), c->range, fitValues, nullptr, 2., true);
  if (fitres >= 0) {
    LOG(info) << "Fit result " << fitres << " Mean = " << fitValues[1] << " Sigma = " << fitValues[2];
  } else {
    LOG(error) << "Fit failed with result = " << fitres;
  }

  // TODO: the timestamp is now given with the TF index, but it will have
  // to become an absolute time. This is true both for the lhc phase object itself
  // and the CCDB entry
  std::map<std::string, std::string> md;
  LHCphase l;
  l.addLHCphase(0, fitValues[1]);
  l.addLHCphase(999999999, fitValues[1]);
  auto clName = o2::utils::MemFileHelper::getClassName(l);
  auto flName = o2::ccdb::CcdbApi::generateFileName(clName);

  auto starting = slot.getStartTimeMS() - 10000; // start 10 seconds before
  auto stopping = slot.getEndTimeMS() + 10000;   // stop 10 seconds after
  LOG(info) << "starting = " << starting << " - stopping = " << stopping << " -> phase = " << fitValues[1] << " ps";

  mInfoVector.emplace_back("TOF/Calib/LHCphase", clName, flName, md, starting, stopping);
  mLHCphaseVector.emplace_back(l);

  slot.print();
}

//_____________________________________________
Slot& LHCClockCalibrator::emplaceNewSlot(bool front, TFType tstart, TFType tend)
{
  auto& cont = getSlots();
  auto& slot = front ? cont.emplace_front(tstart, tend) : cont.emplace_back(tstart, tend);
  slot.setContainer(std::make_unique<LHCClockDataHisto>(mNBins, mRange, mCalibTOFapi));
  return slot;
}

} // end namespace tof
} // end namespace o2
