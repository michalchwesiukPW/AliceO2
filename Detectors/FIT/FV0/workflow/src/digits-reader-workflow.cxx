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

/// \file digits-reader-workflow.cxx
/// \brief Implementation of FV0 digits reader
///
/// \author ruben.shahoyan@cern.ch

#include "Framework/CallbackService.h"
#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/Task.h"
#include "FV0Workflow/DigitReaderSpec.h"
#include "CommonUtils/ConfigurableParam.h"

using namespace o2::framework;

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  // option allowing to set parameters
  std::vector<o2::framework::ConfigParamSpec> options{
    {"disable-mc", o2::framework::VariantType::Bool, false, {"disable MC propagation even if available"}}};
  options.push_back(ConfigParamSpec{"disable-trigger-input", o2::framework::VariantType::Bool, false, {"Disable trigger input DPL channel"}});
  std::string keyvaluehelp("Semicolon separated key=value strings");
  options.push_back(ConfigParamSpec{"configKeyValues", VariantType::String, "", {keyvaluehelp}});
  std::swap(workflowOptions, options);
}

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(const ConfigContext& ctx)
{
  WorkflowSpec specs;
  o2::conf::ConfigurableParam::updateFromString(ctx.options().get<std::string>("configKeyValues"));
  DataProcessorSpec producer = o2::fv0::getDigitReaderSpec(ctx.options().get<bool>("disable-mc"), ctx.options().get<bool>("disable-trigger-input"));
  specs.push_back(producer);
  return specs;
}
