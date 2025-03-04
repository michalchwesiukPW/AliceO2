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

///
/// \file    DatDecoderSpec.cxx
/// \author  Andrea Ferrero
///
/// \brief Implementation of a data processor to run the raw decoding
///

#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <array>
#include <functional>

#include "Framework/CallbackService.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/DataProcessorSpec.h"
#include "Framework/InputRecordWalker.h"
#include "Framework/Lifetime.h"
#include "Framework/Output.h"
#include "Framework/Task.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/DataRefUtils.h"

#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/RDHUtils.h"
#include "DPLUtils/DPLRawParser.h"
#include "CommonConstants/LHCConstants.h"
#include "DetectorsRaw/HBFUtils.h"

#include "DataFormatsMCH/Digit.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawCommon/CoDecParam.h"
#include "MCHRawDecoder/DataDecoder.h"
#include "MCHRawDecoder/ROFFinder.h"
#include "MCHRawElecMap/Mapper.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHWorkflow/DataDecoderSpec.h"
#include "CommonUtils/VerbosityConfig.h"

//#define MCH_RAW_DATADECODER_DEBUG_DIGIT_TIME 1

namespace o2
{
namespace mch
{
namespace raw
{

using namespace o2;
using namespace o2::framework;
using namespace o2::mch::mapping;
using RDH = o2::header::RDHAny;

//=======================
// Data decoder
class DataDecoderTask
{
 public:
  DataDecoderTask(std::string spec) : mInputSpec(spec) {}

  //_________________________________________________________________________________________________
  void init(framework::InitContext& ic)
  {
    SampaChannelHandler channelHandler;
    RdhHandler rdhHandler;

    auto ds2manu = ic.options().get<bool>("ds2manu");
    auto sampaBcOffset = CoDecParam::Instance().sampaBcOffset;
    mDebug = ic.options().get<bool>("mch-debug");
    mCheckROFs = ic.options().get<bool>("check-rofs");
    mDummyROFs = ic.options().get<bool>("dummy-rofs");
    auto mapCRUfile = ic.options().get<std::string>("cru-map");
    auto mapFECfile = ic.options().get<std::string>("fec-map");
    auto useDummyElecMap = ic.options().get<bool>("dummy-elecmap");
    mErrorLogFrequency = ic.options().get<int>("error-log-frequency");
    auto timeRecoModeString = ic.options().get<std::string>("time-reco-mode");

    DataDecoder::TimeRecoMode timeRecoMode = DataDecoder::TimeRecoMode::HBPackets;
    if (timeRecoModeString == "hbpackets") {
      timeRecoMode = DataDecoder::TimeRecoMode::HBPackets;
    } else if (timeRecoModeString == "bcreset") {
      timeRecoMode = DataDecoder::TimeRecoMode::BCReset;
    }

    mDecoder = new DataDecoder(channelHandler, rdhHandler, sampaBcOffset, mapCRUfile, mapFECfile, ds2manu, mDebug,
                               useDummyElecMap, timeRecoMode);

    auto stop = [this]() {
      LOG(info) << "mch-data-decoder: decoding duration = " << mTimeDecoding.count() * 1000 / mTFcount << " us / TF";
      LOG(info) << "mch-data-decoder: ROF finder duration = " << mTimeROFFinder.count() * 1000 / mTFcount << " us / TF";
    };
    ic.services().get<CallbackService>().set(CallbackService::Id::Stop, stop);
  }

  //_________________________________________________________________________________________________
  // the decodeTF() function processes the messages generated by the (sub)TimeFrame builder
  void decodeTF(framework::ProcessingContext& pc)
  {
    const auto* dh = DataRefUtils::getHeader<o2::header::DataHeader*>(pc.inputs().getFirstValid(true));
    mFirstTForbit = dh->firstTForbit;

    mDecoder->setFirstOrbitInTF(mFirstTForbit);

    if (mDebug) {
      LOG(info) << "[DataDecoderSpec::run] first TF orbit is " << mFirstTForbit;
    }

    // get the input buffer
    auto& inputs = pc.inputs();
    DPLRawParser parser(inputs, o2::framework::select(mInputSpec.c_str()));
    bool abort{false};
    for (auto it = parser.begin(), end = parser.end(); it != end && abort == false; ++it) {
      auto const* raw = it.raw();
      if (!raw) {
        continue;
      }
      size_t payloadSize = it.size();

      gsl::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(raw), sizeof(RDH) + payloadSize);
      bool ok = mDecoder->decodeBuffer(buffer);
      if (!ok) {
        LOG(alarm) << "critical decoding error : aborting this TF decoding\n";
        abort = true;
      }
    }
  }

  //_________________________________________________________________________________________________
  // the decodeReadout() function processes the messages generated by o2-mch-cru-page-reader-workflow
  void decodeReadout(const o2::framework::DataRef& input)
  {
    static int nFrame = 1;
    // get the input buffer
    if (input.spec->binding != "readout") {
      return;
    }

    auto const* raw = input.payload;
    // size of payload
    size_t payloadSize = DataRefUtils::getPayloadSize(input);

    if (mDebug) {
      std::cout << nFrame << "  payloadSize=" << payloadSize << std::endl;
    }
    nFrame += 1;
    if (payloadSize == 0) {
      return;
    }

    const RDH* rdh = reinterpret_cast<const RDH*>(raw);
    mFirstTForbit = o2::raw::RDHUtils::getHeartBeatOrbit(rdh);
    mDecoder->setFirstOrbitInTF(mFirstTForbit);

    gsl::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(raw), payloadSize);
    mDecoder->decodeBuffer(buffer);
  }

  void sendEmptyOutput(framework::DataAllocator& output)
  {
    std::vector<Digit> digits;
    std::vector<ROFRecord> rofs;
    std::vector<OrbitInfo> orbits;
    output.snapshot(Output{header::gDataOriginMCH, "DIGITS", 0}, digits);
    output.snapshot(Output{header::gDataOriginMCH, "DIGITROFS", 0}, rofs);
    output.snapshot(Output{header::gDataOriginMCH, "ORBITS", 0}, orbits);
  }

  bool isDroppedTF(framework::ProcessingContext& pc)
  {
    /// If we see requested data type input
    /// with 0xDEADBEEF subspec and 0 payload this means that the
    /// "delayed message" mechanism created it in absence of real data
    /// from upstream, i.e. the TF was dropped.
    constexpr auto origin = header::gDataOriginMCH;
    static size_t contDeadBeef = 0; // number of times 0xDEADBEEF was seen continuously
    o2::framework::InputSpec dummy{"dummy",
                                   framework::ConcreteDataMatcher{origin,
                                                                  header::gDataDescriptionRawData,
                                                                  0xDEADBEEF}};
    for (const auto& ref : o2::framework::InputRecordWalker(pc.inputs(), {dummy})) {
      const auto dh = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
      auto payloadSize = DataRefUtils::getPayloadSize(ref);
      if (payloadSize == 0) {
        auto maxWarn = o2::conf::VerbosityConfig::Instance().maxWarnDeadBeef;
        if (++contDeadBeef <= maxWarn) {
          LOGP(warning, "Found input [{}/{}/{:#x}] TF#{} 1st_orbit:{} Payload {} : assuming no payload for all links in this TF{}",
               dh->dataOrigin.str, dh->dataDescription.str, dh->subSpecification, dh->tfCounter, dh->firstTForbit, payloadSize,
               contDeadBeef == maxWarn ? fmt::format(". {} such inputs in row received, stopping reporting", contDeadBeef) : "");
        }
        return true;
      }
    }
    contDeadBeef = 0; // if good data, reset the counter
    return false;
  }

  //_________________________________________________________________________________________________
  void run(framework::ProcessingContext& pc)
  {
    if (isDroppedTF(pc)) {
      sendEmptyOutput(pc.outputs());
      return;
    }

    static int32_t deltaMax = 0;

    auto createBuffer = [&](auto& vec, size_t& size) {
      size = vec.empty() ? 0 : sizeof(*(vec.begin())) * vec.size();
      char* buf = nullptr;
      if (size > 0) {
        buf = (char*)malloc(size);
        if (buf) {
          char* p = buf;
          size_t sizeofElement = sizeof(*(vec.begin()));
          for (auto& element : vec) {
            memcpy(p, &element, sizeofElement);
            p += sizeofElement;
          }
        }
      }
      return buf;
    };

    auto tStart = std::chrono::high_resolution_clock::now();
    mDecoder->reset();
    for (auto&& input : pc.inputs()) {
      if (input.spec->binding == "readout") {
        decodeReadout(input);
      }
      if (input.spec->binding == "TF") {
        decodeTF(pc);
      }
    }
    mDecoder->computeDigitsTime();
    int minDigitOrbitAccepted = CoDecParam::Instance().minDigitOrbitAccepted;
    int maxDigitOrbitAccepted = CoDecParam::Instance().maxDigitOrbitAccepted;
    mDecoder->checkDigitsTime(minDigitOrbitAccepted, maxDigitOrbitAccepted);
    auto tEnd = std::chrono::high_resolution_clock::now();
    mTimeDecoding += tEnd - tStart;

    auto& digits = mDecoder->getDigits();
    auto& orbits = mDecoder->getOrbits();
    auto& errors = mDecoder->getErrors();

#ifdef MCH_RAW_DATADECODER_DEBUG_DIGIT_TIME
    constexpr int BCINORBIT = o2::constants::lhc::LHCMaxBunches;
    int32_t bcMax = mNHBPerTF * 3564 - 1;
    for (auto d : digits) {
      if (d.getTime() < 0 || d.getTime() > bcMax) {
        int32_t delta = d.getTime() - bcMax;
        if (delta > deltaMax) {
          deltaMax = delta;
          std::cout << "Max digit time exceeded: TF ORBIT " << mDecoder->getFirstOrbitInTF().value() << "  DE# " << d.getDetID() << " PadId " << d.getPadID() << " ADC " << d.getADC()
                    << " time " << d.getTime() << " (" << bcMax << ", delta=" << delta << ")" << std::endl;
        }
      }
    }
#endif

    tStart = std::chrono::high_resolution_clock::now();
    ROFFinder rofFinder(digits, mFirstTForbit);
    rofFinder.process(mDummyROFs);
    tEnd = std::chrono::high_resolution_clock::now();
    mTimeROFFinder += tEnd - tStart;

    if (mDebug) {
      rofFinder.dumpOutputDigits();
      rofFinder.dumpOutputROFs();
    }

    if (mCheckROFs) {
      rofFinder.isRofTimeMonotonic();
      rofFinder.isDigitsTimeAligned();
    }

    // send the output buffer via DPL
    size_t digitsSize, rofsSize, orbitsSize, errorsSize;
    char* digitsBuffer = rofFinder.saveDigitsToBuffer(digitsSize);
    char* rofsBuffer = rofFinder.saveROFRsToBuffer(rofsSize);
    char* orbitsBuffer = createBuffer(orbits, orbitsSize);
    char* errorsBuffer = createBuffer(errors, errorsSize);

    if (mDebug) {
      LOGP(info, "digitsSize {}  rofsSize {}  orbitsSize {}  errorsSize {}", digitsSize, rofsSize, orbitsSize, errorsSize);
    }

    // create the output message
    auto freefct = [](void* data, void*) { free(data); };
    pc.outputs().adoptChunk(Output{header::gDataOriginMCH, "DIGITS", 0}, digitsBuffer, digitsSize, freefct, nullptr);
    pc.outputs().adoptChunk(Output{header::gDataOriginMCH, "DIGITROFS", 0}, rofsBuffer, rofsSize, freefct, nullptr);
    pc.outputs().adoptChunk(Output{header::gDataOriginMCH, "ORBITS", 0}, orbitsBuffer, orbitsSize, freefct, nullptr);
    pc.outputs().adoptChunk(Output{header::gDataOriginMCH, "ERRORS", 0}, errorsBuffer, errorsSize, freefct, nullptr);

    mTFcount += 1;
    if (mErrorLogFrequency) {
      if (mTFcount == 1 || mTFcount % mErrorLogFrequency == 0) {
        mDecoder->logErrorMap(mTFcount);
      }
    }
  }

 private:
  std::string mInputSpec;            /// selection string for the input data
  bool mDebug = {false};             /// flag to enable verbose output
  bool mCheckROFs = {false};         /// flag to enable consistency checks on the output ROFs
  bool mDummyROFs = {false};         /// flag to disable the ROFs finding
  uint32_t mFirstTForbit{0};         /// first orbit of the time frame being processed
  DataDecoder* mDecoder = {nullptr}; /// pointer to the data decoder instance

  uint32_t mTFcount{0};
  uint32_t mErrorLogFrequency; /// error map is logged at that frequency (use 0 to disable) (in TF unit)

  std::chrono::duration<double, std::milli> mTimeDecoding{};  ///< timer
  std::chrono::duration<double, std::milli> mTimeROFFinder{}; ///< timer
};

//_________________________________________________________________________________________________
o2::framework::DataProcessorSpec getDecodingSpec(const char* specName, std::string inputSpec,
                                                 bool askSTFDist)
{
  auto inputs = o2::framework::select(inputSpec.c_str());
  for (auto& inp : inputs) {
    // mark input as optional in order not to block the workflow
    // if our raw data happen to be missing in some TFs
    inp.lifetime = Lifetime::Optional;
  }
  if (askSTFDist) {
    // request the input FLP/DISTSUBTIMEFRAME/0 that is _guaranteed_
    // to be present, even if none of our raw data is present.
    inputs.emplace_back("stfDist", "FLP", "DISTSUBTIMEFRAME", 0, o2::framework::Lifetime::Timeframe);
  }
  o2::mch::raw::DataDecoderTask task(inputSpec);
  return DataProcessorSpec{
    specName,
    inputs,
    Outputs{OutputSpec{header::gDataOriginMCH, "DIGITS", 0, Lifetime::Timeframe},
            OutputSpec{header::gDataOriginMCH, "DIGITROFS", 0, Lifetime::Timeframe},
            OutputSpec{header::gDataOriginMCH, "ORBITS", 0, Lifetime::Timeframe},
            OutputSpec{header::gDataOriginMCH, "ERRORS", 0, Lifetime::Timeframe}},
    AlgorithmSpec{adaptFromTask<DataDecoderTask>(std::move(task))},
    Options{{"mch-debug", VariantType::Bool, false, {"enable verbose output"}},
            {"cru-map", VariantType::String, "", {"custom CRU mapping"}},
            {"fec-map", VariantType::String, "", {"custom FEC mapping"}},
            {"dummy-elecmap", VariantType::Bool, false, {"use dummy electronic mapping (for debug, temporary)"}},
            {"ds2manu", VariantType::Bool, false, {"convert channel numbering from Run3 to Run1-2 order"}},
            {"time-reco-mode", VariantType::String, "bcreset", {"digit time reconstruction method [hbpackets, bcreset]"}},
            {"check-rofs", VariantType::Bool, false, {"perform consistency checks on the output ROFs"}},
            {"dummy-rofs", VariantType::Bool, false, {"disable the ROFs finding algorithm"}},
            {"error-log-frequency", VariantType::Int, 6000, {"log the error map at this frequency (in TF unit) (first TF is always logged, unless frequency is zero)"}}}};
}

} // namespace raw
} // namespace mch
} // end namespace o2
