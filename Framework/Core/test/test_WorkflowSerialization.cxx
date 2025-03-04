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
#define BOOST_TEST_MODULE Test Framework WorkflowSerializationHelpers
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include "Framework/WorkflowSpec.h"
#include "../src/WorkflowSerializationHelpers.h"
#include <boost/test/unit_test.hpp>

using namespace o2::framework;

BOOST_AUTO_TEST_CASE(TestVerifyWorkflow)
{
  using namespace o2::framework;
  WorkflowSpec w0{                       //
                  DataProcessorSpec{"A", //
                                    {InputSpec{"foo", "A", "COLLISIONCONTEXT", 1, Lifetime::Condition, {
                                                                                                         ConfigParamSpec{"aUrl", VariantType::String, "foo/bar", {"A InputSpec option"}},       //
                                                                                                         ConfigParamSpec{"bUrl", VariantType::String, "foo/foo", {"Another InputSpec option"}}, //
                                                                                                       }}},                                                                                     //
                                    {OutputSpec{{"bar"}, "C", "D", 2, Lifetime::Timeframe}},                                                                                                    //
                                    AlgorithmSpec{[](ProcessingContext& ctx) {}},                                                                                                               //
                                    {                                                                                                                                                           //
                                     ConfigParamSpec{"aInt", VariantType::Int, 0, {"An Int"}},                                                                                                  //
                                     ConfigParamSpec{"aFloat", VariantType::Float, 1.3, {"A Float"}},                                                                                           //
                                     ConfigParamSpec{"aBool", VariantType::Bool, true, {"A Bool"}},                                                                                             //
                                     ConfigParamSpec{"aString", VariantType::String, "some string", {"A String"}}}},                                                                            //                                                                                                    //
                  DataProcessorSpec{"B",                                                                                                                                                        //
                                    {InputSpec{"foo", "C", "D"}},                                                                                                                               //
                                    {                                                                                                                                                           //
                                     OutputSpec{{"bar1"}, "E", "F", 0},                                                                                                                         //
                                     OutputSpec{{"bar2"}, "E", "F", 1}},                                                                                                                        //
                                    AlgorithmSpec{[](ProcessingContext& ctx) {}},                                                                                                               //
                                    {}},                                                                                                                                                        //
                  DataProcessorSpec{"C", {},                                                                                                                                                    //
                                    {                                                                                                                                                           //
                                     OutputSpec{{"bar"}, "G", "H"}},                                                                                                                            //
                                    AlgorithmSpec{[](ProcessingContext& ctx) {}},                                                                                                               //
                                    {}},                                                                                                                                                        //
                  DataProcessorSpec{"D", {InputSpec{"foo", {"C", "D"}}},                                                                                                                        //
                                    {OutputSpec{{"bar"}, {"I", "L"}}},                                                                                                                          //
                                    AlgorithmSpec{[](ProcessingContext& ctx) {}},                                                                                                               //
                                    {},                                                                                                                                                         //
                                    CommonServices::defaultServices(),                                                                                                                          //
                                    {{"label a"}, {"label \"b\""}}}};

  std::vector<DataProcessorInfo> metadataOut{
    {"A", "test_Framework_test_SerializationWorkflow", {"foo"}, {ConfigParamSpec{"aBool", VariantType::Bool, true, {"A Bool"}}}},
    {"B", "test_Framework_test_SerializationWorkflow", {"b-bar", "bfoof", "fbdbfaso"}},
    {"C", "test_Framework_test_SerializationWorkflow", {}},
    {"D", "test_Framework_test_SerializationWorkflow", {}},
  };

  CommandInfo commandInfoOut{"o2-dpl-workflow -b --option 1 --option 2"};

  std::vector<DataProcessorInfo> metadataIn{};
  CommandInfo commandInfoIn;

  std::ostringstream firstDump;
  WorkflowSerializationHelpers::dump(firstDump, w0, metadataOut, commandInfoOut);
  std::istringstream is;
  is.str(firstDump.str());
  WorkflowSpec w1;
  WorkflowSerializationHelpers::import(is, w1, metadataIn, commandInfoIn);

  std::ostringstream secondDump;
  WorkflowSerializationHelpers::dump(secondDump, w1, metadataIn, commandInfoIn);

  BOOST_REQUIRE_EQUAL(w0.size(), 4);
  BOOST_REQUIRE_EQUAL(w0.size(), w1.size());
  BOOST_CHECK_EQUAL(firstDump.str(), secondDump.str());
  BOOST_CHECK_EQUAL(commandInfoIn.command, commandInfoOut.command);

  // also check if the conversion to ConcreteDataMatcher is working at import
  BOOST_CHECK(std::get_if<ConcreteDataMatcher>(&w1[0].inputs[0].matcher) != nullptr);
}
