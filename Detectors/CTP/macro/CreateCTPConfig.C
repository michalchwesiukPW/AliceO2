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

/// \file CreateCTPConfig.C
/// \brief create CTP config, test it and add to database
/// \author Roman Lietava

#if !defined(__CLING__) || defined(__ROOTCLING__)

#include "FairLogger.h"
#include "CCDB/CcdbApi.h"
#include "CCDB/BasicCCDBManager.h"
#include "DataFormatsCTP/Configuration.h"
#include <string>
#include <map>
#include <iostream>
#endif
using namespace o2::ctp;
void CreateCTPConfig(long tmin = 0, long tmax = -1, std::string ccdbHost = "http://ccdb-test.cern.ch:8080")
{
  /// Demo configuration
  CTPConfiguration ctpcfg;
  std::string cfgstr = "PARTITION: TEST \n";
  cfgstr += "VERSION:0 \n";
  cfgstr += "INPUTS: \n";
  cfgstr += "MFV0MB FV0 M 0x1 \n";
  cfgstr += "MFV0MBInner FV0 M 0x2 \n";
  cfgstr += "MFV0MBOuter FV0 M 0x4 \n";
  cfgstr += "MFV0HM FV0 M 0x8 \n";
  cfgstr += "MFT0A FT0 M 0x10 \n";
  cfgstr += "MFT0B FT0 M 0x20 \n";
  cfgstr += "MFT0Vertex FT0 M 0x40 \n";
  cfgstr += "MFT0Cent FT0 M 0x80 \n";
  cfgstr += "MFT0SemiCent FT0 M 0x100 \n";
  cfgstr += "DESCRIPTORS: \n";
  cfgstr += "DV0MB MFV0MB \n";
  cfgstr += "DV0MBInner MFV0MBInner \n";
  cfgstr += "DV0MBOuter MFV0MBOuter \n";
  cfgstr += "DT0AND MFT0A MFT0B \n";
  cfgstr += "DT0A MFT0A \n";
  cfgstr += "DT0B MFT0B \n";
  cfgstr += "DINTV0T0 MFV0MB MFT0Vertex \n";
  cfgstr += "DINT4 MFV0MB MFT0A MFT0B \n";
  cfgstr += "DV0HM MFV0HM \n";
  cfgstr += "DT0HM MFT0Cent \n";
  cfgstr += "DHM MFV0HM MFT0Cent \n";
  cfgstr += "CLUSTERS: ALL\n";
  cfgstr += "ALL FV0 FT0 TPC \n";
  cfgstr += "CLASSES:\n";
  cfgstr += "CMBV0 0 DV0MB ALL \n";
  cfgstr += "CMBT0 1 DT0AND ALL \n";
  cfgstr += "CINT4 2 DINT4 ALL \n";
  cfgstr += "CINTV0T0 3 DINTV0T0 ALL \n";
  cfgstr += "CT0A 4 DT0A ALL \n";
  cfgstr += "CT0B 62 DT0B ALL \n";
  cfgstr += "CINTHM 63 DHM ALL \n";
  //
  // run3 config
  //
  std::string cfgRun3str =
    "bcm TOF 100 1288 2476 \n \
bcm PHYS 1226 \n\
bcd10 1khz \n\
bcd20 0 \n\
bcd2m 45khz \n\
#  \n\
LTG tof  \n\
trig  \n\
bcm TOF e \n\
#   \n\
LTG mft \n\
ferst 1 \n\
# \n\
LTG mch \n\
ferst 1 \n\
# 3 clusters for CRU, TRD and oldTTC detectors: \n\
0 cluster clu1 fv0 ft0 fdd its mft mid mch tpc zdc tst tof \n\
0 cl_ph PHYS \n\
# \n\
1 cluster clu2 trd \n\
1 cl_45khz bcd2m \n\
2 cluster clu3 hmp phs \n\
2 cl_1khz bcd10 \n \
3 cluster clu4 emc cpv \n \
4 cl_5khz bcd20 \n";
  // ctpcfg.loadConfiguration(cfgstr);
  ctpcfg.loadConfigurationRun3(cfgRun3str);
  ctpcfg.printStream(std::cout);
  std::cout << "Going write to db" << std::endl;
  // return;
  ///
  /// add to database
  o2::ccdb::CcdbApi api;
  map<string, string> metadata; // can be empty
  api.init(ccdbHost.c_str());   // or http://localhost:8080 for a local installation
  // store abitrary user object in strongly typed manner
  api.storeAsTFileAny(&ctpcfg, o2::ctp::CCDBPathCTPConfig, metadata, tmin, tmax);
  std::cout << "CTP config in database" << std::endl;
  /// get frp, database
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL(ccdbHost);
  auto ctpconfigdb = mgr.get<CTPConfiguration>(CCDBPathCTPConfig);
  ctpconfigdb->printStream(std::cout);
}
