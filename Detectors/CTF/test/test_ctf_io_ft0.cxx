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

#define BOOST_TEST_MODULE Test FT0CTFIO
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "CommonUtils/NameConf.h"
#include "FT0Reconstruction/CTFCoder.h"
#include "FT0Simulation/FT0DigParam.h"
#include "Framework/Logger.h"
#include <TFile.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <cstring>

using namespace o2::ft0;

BOOST_AUTO_TEST_CASE(CTFTest)
{
  std::vector<Digit> digits;
  std::vector<ChannelData> channels;
  TStopwatch sw;
  sw.Start();
  o2::InteractionRecord ir(0, 0);

  int trg_gate = FT0DigParam::Instance().mTime_trg_gate;
  constexpr int MAXChan = 4 * (Geometry::NCellsA + Geometry::NCellsC);
  for (int idig = 0; idig < 1000; idig++) {
    ir += 1 + gRandom->Integer(200);
    uint8_t ich = gRandom->Poisson(10);
    auto start = channels.size();
    int16_t tMeanA = 0, tMeanC = 0;
    int32_t ampTotA = 0, ampTotC = 0;
    int8_t nChanA = 0, nChanC = 0;
    uint8_t eventFlag = 10;
    while (ich < MAXChan) {
      int16_t t = -2048 + gRandom->Integer(2048 * 2);
      uint16_t q = gRandom->Integer(4096);
      uint8_t chain = gRandom->Rndm() > 0.5 ? 0 : 1;
      channels.emplace_back(ich, t, q, chain);
      if (std::abs(t) < trg_gate) {
        if (ich < 4 * uint8_t(Geometry::NCellsA)) {
          nChanA++;
          ampTotA += q;
          tMeanA += t;
        } else {
          nChanC++;
          ampTotC += q;
          tMeanC += t;
        }
      }
      ich += 1 + gRandom->Poisson(10);
    }
    if (nChanA) {
      tMeanA /= nChanA;
      ampTotA *= 0.125;
    } else {
      tMeanA = o2::fit::Triggers::DEFAULT_TIME;
      ampTotA = o2::fit::Triggers::DEFAULT_AMP;
    }
    if (nChanC) {
      tMeanC /= nChanC;
      ampTotC *= 0.125; // sum/8
    } else {
      tMeanC = o2::fit::Triggers::DEFAULT_TIME;
      ampTotC = o2::fit::Triggers::DEFAULT_AMP;
    }
    Triggers trig;
    trig.setTriggers(gRandom->Integer(128), nChanA, nChanC, ampTotA, ampTotC, tMeanA, tMeanC);
    auto end = channels.size();
    digits.emplace_back(start, end - start, ir, trig, idig);
    digits[idig].setEventStatus(eventFlag);
  }

  LOG(info) << "Generated " << channels.size() << " channels in " << digits.size() << " digits " << sw.CpuTime() << " s";

  sw.Start();
  std::vector<o2::ctf::BufferType> vec;
  {
    CTFCoder coder(o2::ctf::CTFCoderBase::OpType::Encoder);
    coder.encode(vec, digits, channels); // compress
  }
  sw.Stop();
  LOG(info) << "Compressed in " << sw.CpuTime() << " s";

  // writing
  {
    sw.Start();
    TFile flOut("test_ctf_ft0.root", "recreate");
    TTree ctfTree(std::string(o2::base::NameConf::CTFTREENAME).c_str(), "O2 CTF tree");
    auto* ctfImage = o2::ft0::CTF::get(vec.data());
    ctfImage->print();
    ctfImage->appendToTree(ctfTree, "FT0");
    ctfTree.Write();
    sw.Stop();
    LOG(info) << "Wrote to tree in " << sw.CpuTime() << " s";
  }

  // reading
  vec.clear();
  {
    sw.Start();
    TFile flIn("test_ctf_ft0.root");
    std::unique_ptr<TTree> tree((TTree*)flIn.Get(std::string(o2::base::NameConf::CTFTREENAME).c_str()));
    BOOST_CHECK(tree);
    o2::ft0::CTF::readFromTree(vec, *(tree.get()), "FT0");
    sw.Stop();
    LOG(info) << "Read back from tree in " << sw.CpuTime() << " s";
  }

  std::vector<Digit> digitsD;
  std::vector<ChannelData> channelsD;

  sw.Start();
  const auto ctfImage = o2::ft0::CTF::getImage(vec.data());
  {
    CTFCoder coder(o2::ctf::CTFCoderBase::OpType::Decoder);
    coder.decode(ctfImage, digitsD, channelsD); // decompress
  }
  sw.Stop();
  LOG(info) << "Decompressed in " << sw.CpuTime() << " s";

  BOOST_CHECK(digitsD.size() == digits.size());
  BOOST_CHECK(channelsD.size() == channels.size());
  LOG(info) << "  BOOST_CHECK digitsD.size() " << digitsD.size() << " digits.size() " << digits.size() << " BOOST_CHECK(channelsD.size()  " << channelsD.size() << " channels.size()) " << channels.size();

  for (int i = digits.size(); i--;) {
    const auto& dor = digits[i];
    const auto& ddc = digitsD[i];
    LOG(debug) << " dor " << dor.mTriggers.print();
    LOG(debug) << " ddc " << ddc.mTriggers.print();

    BOOST_CHECK(dor.mIntRecord == ddc.mIntRecord);
    BOOST_CHECK(dor.mTriggers.getNChanA() == ddc.mTriggers.getNChanA());
    BOOST_CHECK(dor.mTriggers.getNChanC() == ddc.mTriggers.getNChanC());
    BOOST_CHECK(dor.mTriggers.getAmplA() == ddc.mTriggers.getAmplA());
    BOOST_CHECK(dor.mTriggers.getAmplC() == ddc.mTriggers.getAmplC());
    BOOST_CHECK(dor.mTriggers.getTimeA() == ddc.mTriggers.getTimeA());
    BOOST_CHECK(dor.mTriggers.getTimeC() == ddc.mTriggers.getTimeC());
    BOOST_CHECK(dor.mTriggers.getTriggersignals() == ddc.mTriggers.getTriggersignals());
  }
  for (int i = channels.size(); i--;) {
    const auto& cor = channels[i];
    const auto& cdc = channelsD[i];
    BOOST_CHECK(cor.ChId == cdc.ChId);
    BOOST_CHECK(cor.ChainQTC == cdc.ChainQTC);
    BOOST_CHECK(cor.CFDTime == cdc.CFDTime);
    BOOST_CHECK(cor.QTCAmpl == cdc.QTCAmpl);
  }
}
