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
#ifndef O2_FRAMEWORK_SIMPLERAWDEVICESERVICE_H_
#define O2_FRAMEWORK_SIMPLERAWDEVICESERVICE_H_

#include "Framework/RawDeviceService.h"
#include "Framework/DeviceSpec.h"

namespace o2::framework
{

/// Fairly unsophisticated service which simply stores and returns the
/// requested FairMQDevice and DeviceSpec
class SimpleRawDeviceService : public RawDeviceService
{
 public:
  SimpleRawDeviceService(FairMQDevice* device, DeviceSpec const& spec)
    : mDevice(device), mSpec(spec)
  {
  }

  FairMQDevice* device() final
  {
    return mDevice;
  }

  void setDevice(FairMQDevice* device) final
  {
    mDevice = device;
  }

  DeviceSpec const& spec() const final
  {
    return mSpec;
  }

  void waitFor(unsigned int ms) final;

 private:
  FairMQDevice* mDevice;
  DeviceSpec const& mSpec;
};

} // namespace o2::framework
#endif // O2_FRAMEWORK_SIMPLERAWDEVICESERVICE_H__
