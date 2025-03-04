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

#include "WorkflowSerializationHelpers.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/DataDescriptorQueryBuilder.h"
#include "Framework/DataSpecUtils.h"
#include "Framework/VariantJSONHelpers.h"
#include "Framework/DataDescriptorMatcher.h"
#include "Framework/Logger.h"

#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <iostream>
#include <algorithm>
#include <memory>

namespace o2::framework
{

using namespace rapidjson;
using namespace o2::framework::data_matcher;

struct WorkflowImporter : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, WorkflowImporter> {
  enum struct State {
    IN_START,
    IN_EXECUTION,
    IN_WORKFLOW,
    IN_METADATA,
    IN_COMMAND,
    IN_DATAPROCESSORS,
    IN_DATAPROCESSOR,
    IN_DATAPROCESSOR_NAME,
    IN_DATAPROCESSOR_RANK,
    IN_DATAPROCESSOR_N_SLOTS,
    IN_DATAPROCESSOR_TIMESLICE_ID,
    IN_DATAPROCESSOR_MAX_TIMESLICES,
    IN_INPUTS,
    IN_OUTPUTS,
    IN_OPTIONS,
    IN_LABELS,
    IN_WORKFLOW_OPTIONS,
    IN_INPUT,
    IN_INPUT_BINDING,
    IN_INPUT_ORIGIN,
    IN_INPUT_DESCRIPTION,
    IN_INPUT_SUBSPEC,
    IN_INPUT_LIFETIME,
    IN_INPUT_MATCHER,
    IN_INPUT_MATCHER_OPERATION,
    IN_INPUT_LEFT_MATCHER,
    IN_INPUT_RIGHT_MATCHER,
    IN_INPUT_OPTIONS,
    IN_OUTPUT,
    IN_OUTPUT_BINDING,
    IN_OUTPUT_ORIGIN,
    IN_OUTPUT_DESCRIPTION,
    IN_OUTPUT_SUBSPEC,
    IN_OUTPUT_LIFETIME,
    IN_OUTPUT_OPTIONS,
    IN_OPTION,
    IN_OPTION_NAME,
    IN_OPTION_TYPE,
    IN_OPTION_DEFAULT,
    IN_OPTION_HELP,
    IN_OPTION_KIND,
    IN_LABEL,
    IN_METADATUM,
    IN_METADATUM_NAME,
    IN_METADATUM_EXECUTABLE,
    IN_METADATUM_ARGS,
    IN_METADATUM_ARG,
    IN_METADATUM_CHANNELS,
    IN_METADATUM_CHANNEL,
    IN_ERROR
  };

  friend std::ostream& operator<<(std::ostream& s, State state)
  {
    switch (state) {
      case State::IN_START:
        s << "IN_START";
        break;
      case State::IN_EXECUTION:
        s << "IN_EXECUTION";
        break;
      case State::IN_WORKFLOW:
        s << "IN_WORKFLOW";
        break;
      case State::IN_COMMAND:
        s << "IN_COMMAND";
        break;
      case State::IN_DATAPROCESSORS:
        s << "IN_DATAPROCESSORS";
        break;
      case State::IN_DATAPROCESSOR:
        s << "IN_DATAPROCESSOR";
        break;
      case State::IN_DATAPROCESSOR_NAME:
        s << "IN_DATAPROCESSOR_NAME";
        break;
      case State::IN_DATAPROCESSOR_RANK:
        s << "IN_DATAPROCESSOR_RANK";
        break;
      case State::IN_DATAPROCESSOR_N_SLOTS:
        s << "IN_DATAPROCESSOR_N_SLOTS";
        break;
      case State::IN_DATAPROCESSOR_TIMESLICE_ID:
        s << "IN_DATAPROCESSOR_TIMESLICE_ID";
        break;
      case State::IN_DATAPROCESSOR_MAX_TIMESLICES:
        s << "IN_DATAPROCESSOR_MAX_TIMESLICES";
        break;
      case State::IN_INPUTS:
        s << "IN_INPUTS";
        break;
      case State::IN_OUTPUTS:
        s << "IN_OUTPUTS";
        break;
      case State::IN_OPTIONS:
        s << "IN_OPTIONS";
        break;
      case State::IN_LABELS:
        s << "IN_LABELS";
        break;
      case State::IN_WORKFLOW_OPTIONS:
        s << "IN_WORKFLOW_OPTIONS";
        break;
      case State::IN_INPUT:
        s << "IN_INPUT";
        break;
      case State::IN_INPUT_BINDING:
        s << "IN_INPUT_BINDING";
        break;
      case State::IN_INPUT_ORIGIN:
        s << "IN_INPUT_ORIGIN";
        break;
      case State::IN_INPUT_DESCRIPTION:
        s << "IN_INPUT_DESCRIPTION";
        break;
      case State::IN_INPUT_SUBSPEC:
        s << "IN_INPUT_SUBSPEC";
        break;
      case State::IN_INPUT_MATCHER:
        s << "IN_INPUT_MATCHER";
        break;
      case State::IN_INPUT_MATCHER_OPERATION:
        s << "IN_INPUT_MATCHER_OPERATION";
        break;
      case State::IN_INPUT_LEFT_MATCHER:
        s << "IN_INPUT_LEFT_MATCHER";
        break;
      case State::IN_INPUT_RIGHT_MATCHER:
        s << "IN_INPUT_RIGHT_MATCHER";
        break;
      case State::IN_INPUT_LIFETIME:
        s << "IN_INPUT_LIFETIME";
        break;
      case State::IN_INPUT_OPTIONS:
        s << "IN_INPUT_OPTIONS";
        break;
      case State::IN_OUTPUT:
        s << "IN_OUTPUT";
        break;
      case State::IN_OUTPUT_BINDING:
        s << "IN_OUTPUT_BINDING";
        break;
      case State::IN_OUTPUT_ORIGIN:
        s << "IN_OUTPUT_ORIGIN";
        break;
      case State::IN_OUTPUT_DESCRIPTION:
        s << "IN_OUTPUT_DESCRIPTION";
        break;
      case State::IN_OUTPUT_SUBSPEC:
        s << "IN_OUTPUT_SUBSPEC";
        break;
      case State::IN_OUTPUT_LIFETIME:
        s << "IN_OUTPUT_LIFETIME";
        break;
      case State::IN_OUTPUT_OPTIONS:
        s << "IN_OUTPUT_OPTIONS";
        break;
      case State::IN_OPTION:
        s << "IN_OPTION";
        break;
      case State::IN_OPTION_NAME:
        s << "IN_OPTION_NAME";
        break;
      case State::IN_OPTION_TYPE:
        s << "IN_OPTION_TYPE";
        break;
      case State::IN_OPTION_DEFAULT:
        s << "IN_OPTION_DEFAULT";
        break;
      case State::IN_OPTION_HELP:
        s << "IN_OPTION_HELP";
        break;
      case State::IN_OPTION_KIND:
        s << "IN_OPTION_KIND";
        break;
      case State::IN_LABEL:
        s << "IN_LABEL";
        break;
      case State::IN_ERROR:
        s << "IN_ERROR";
        break;
      case State::IN_METADATA:
        s << "IN_METADATA";
        break;
      case State::IN_METADATUM:
        s << "IN_METADATUM";
        break;
      case State::IN_METADATUM_NAME:
        s << "IN_METADATUM_NAME";
        break;
      case State::IN_METADATUM_EXECUTABLE:
        s << "IN_METADATUM_EXECUTABLE";
        break;
      case State::IN_METADATUM_ARGS:
        s << "IN_METADATUM_ARGS";
        break;
      case State::IN_METADATUM_ARG:
        s << "IN_METADATUM_ARG";
        break;
      case State::IN_METADATUM_CHANNELS:
        s << "IN_METADATUM_CHANNELS";
        break;
      case State::IN_METADATUM_CHANNEL:
        s << "IN_METADATUM_CHANNEL";
        break;
    }
    return s;
  }

  WorkflowImporter(std::vector<DataProcessorSpec>& o,
                   std::vector<DataProcessorInfo>& m,
                   CommandInfo& c)
    : states{},
      dataProcessors{o},
      metadata{m},
      command{c}
  {
    push(State::IN_START);
  }

  bool StartObject()
  {
    enter("START_OBJECT");
    if (in(State::IN_START)) {
      push(State::IN_EXECUTION);
    } else if (in(State::IN_DATAPROCESSORS)) {
      push(State::IN_DATAPROCESSOR);
      dataProcessors.push_back(DataProcessorSpec{});
    } else if (in(State::IN_DATAPROCESSOR)) {
      dataProcessors.push_back(DataProcessorSpec{});
    } else if (in(State::IN_INPUTS)) {
      push(State::IN_INPUT);
      inputMatcherNodes.clear();
    } else if (in(State::IN_INPUT_MATCHER)) {
      assert(0); // to be implemented
    } else if (in(State::IN_INPUT_LEFT_MATCHER)) {
      assert(0); // to be implemented
    } else if (in(State::IN_INPUT_RIGHT_MATCHER)) {
      assert(0); // to be implemented
    } else if (in(State::IN_OUTPUTS)) {
      push(State::IN_OUTPUT);
      outputHasSubSpec = false;
    } else if (in(State::IN_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_INPUT_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_OUTPUT_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_WORKFLOW_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_METADATA)) {
      push(State::IN_METADATUM);
      metadata.push_back(DataProcessorInfo{});
    } else if (in(State::IN_METADATUM)) {
      metadata.push_back(DataProcessorInfo{});
    } else if (in(State::IN_COMMAND)) {
      command = CommandInfo{};
    }
    return true;
  }

  bool EndObject(SizeType memberCount)
  {
    enter("END_OBJECT");
    if (in(State::IN_INPUT)) {
      auto buildMatcher = [](auto& nodes) -> std::unique_ptr<DataDescriptorMatcher> {
        auto lastMatcher =
          std::make_unique<DataDescriptorMatcher>(DataDescriptorMatcher::Op::Just,
                                                  StartTimeValueMatcher(ContextRef{ContextPos::STARTTIME_POS}));
        for (size_t ni = 0, ne = nodes.size(); ni < ne; ++ni) {
          auto& node = nodes[nodes.size() - 1 - ni];
          auto tmp = std::make_unique<DataDescriptorMatcher>(DataDescriptorMatcher::Op::And,
                                                             std::move(node),
                                                             std::move(lastMatcher));
          assert(lastMatcher.get() == nullptr);
          lastMatcher = std::move(tmp);
        }
        return lastMatcher;
      };

      auto matcher = buildMatcher(inputMatcherNodes);
      auto concrete = DataSpecUtils::optionalConcreteDataMatcherFrom(*matcher);
      if (concrete.has_value()) {
        // the matcher is fully qualified with unique parameters so we add ConcreteDataMatcher
        dataProcessors.back().inputs.push_back(InputSpec({binding}, (*concrete).origin, (*concrete).description, (*concrete).subSpec, lifetime, inputOptions));
      } else {
        dataProcessors.back().inputs.push_back(InputSpec({binding}, std::move(*matcher), lifetime, inputOptions));
      }
      inputMatcherNodes.clear();
      inputOptions.clear();

    } else if (in(State::IN_OUTPUT)) {
      if (outputHasSubSpec) {
        dataProcessors.back().outputs.push_back(OutputSpec({binding}, origin, description, subspec, lifetime));
      } else {
        dataProcessors.back().outputs.push_back(OutputSpec({binding}, {origin, description}, lifetime));
      }
      outputHasSubSpec = false;
    } else if (in(State::IN_OPTION)) {
      std::unique_ptr<ConfigParamSpec> opt{nullptr};

      using HelpString = ConfigParamSpec::HelpString;
      std::stringstream is;
      is.str(optionDefault);
      switch (optionType) {
        case VariantType::String:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, optionDefault.c_str(), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Int:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, std::stoi(optionDefault, nullptr), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::UInt8:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, static_cast<uint8_t>(std::stoul(optionDefault, nullptr)), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::UInt16:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, static_cast<uint16_t>(std::stoul(optionDefault, nullptr)), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::UInt32:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, static_cast<uint32_t>(std::stoul(optionDefault, nullptr)), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::UInt64:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, std::stoul(optionDefault, nullptr), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Int64:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, std::stol(optionDefault, nullptr), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Float:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, std::stof(optionDefault, nullptr), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Double:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, std::stod(optionDefault, nullptr), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Bool:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, (bool)std::stoi(optionDefault, nullptr), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::ArrayInt:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::ArrayInt>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::ArrayFloat:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::ArrayFloat>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::ArrayDouble:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::ArrayDouble>(is), HelpString{optionHelp}, optionKind);
          break;
          //        case VariantType::ArrayBool:
          //          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::ArrayBool>(is), HelpString{optionHelp}, optionKind);
          //          break;
        case VariantType::ArrayString:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::ArrayString>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Array2DInt:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::Array2DInt>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Array2DFloat:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::Array2DFloat>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Array2DDouble:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::Array2DDouble>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::LabeledArrayInt:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::LabeledArrayInt>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::LabeledArrayFloat:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::LabeledArrayFloat>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::LabeledArrayDouble:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, VariantJSONHelpers::read<VariantType::LabeledArrayDouble>(is), HelpString{optionHelp}, optionKind);
          break;
        case VariantType::Dict:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, emptyDict(), HelpString{optionHelp}, optionKind);
          break;
        default:
          opt = std::make_unique<ConfigParamSpec>(optionName, optionType, optionDefault, HelpString{optionHelp}, optionKind);
      }
      // Depending on the previous state, push options to the right place.
      if (previousIs(State::IN_OPTIONS)) {
        dataProcessors.back().options.push_back(*opt);
      } else if (previousIs(State::IN_WORKFLOW_OPTIONS)) {
        metadata.back().workflowOptions.push_back(*opt);
      } else if (previousIs(State::IN_INPUT_OPTIONS)) {
        inputOptions.push_back(*opt);
      } else if (previousIs(State::IN_OUTPUT_OPTIONS)) {
        outputOptions.push_back(*opt);
      } else {
        assert(false);
      }
    }
    pop();
    return true;
  }

  bool StartArray()
  {
    enter("START_ARRAY");
    if (in(State::IN_WORKFLOW)) {
      push(State::IN_DATAPROCESSORS);
    } else if (in(State::IN_INPUTS)) {
      push(State::IN_INPUT);
    } else if (in(State::IN_INPUT_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_OUTPUT_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_OUTPUTS)) {
      push(State::IN_OUTPUT);
      outputHasSubSpec = false;
    } else if (in(State::IN_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_WORKFLOW_OPTIONS)) {
      push(State::IN_OPTION);
    } else if (in(State::IN_LABELS)) {
      push(State::IN_LABEL);
    } else if (in(State::IN_METADATA)) {
      push(State::IN_METADATUM);
    } else if (in(State::IN_METADATUM_ARGS)) {
      push(State::IN_METADATUM_ARG);
    } else if (in(State::IN_METADATUM_CHANNELS)) {
      push(State::IN_METADATUM_CHANNEL);
    }
    return true;
  }

  bool EndArray(SizeType count)
  {
    enter("END_ARRAY");
    // Handle the case in which inputs / options / outputs are
    // empty.
    if (in(State::IN_INPUT) || in(State::IN_OUTPUT) || in(State::IN_OPTION) || in(State::IN_LABEL) || in(State::IN_METADATUM) || in(State::IN_METADATUM_ARG) || in(State::IN_METADATUM_CHANNEL) || in(State::IN_DATAPROCESSORS)) {
      pop();
    }
    pop();
    return true;
  }

  bool Key(const Ch* str, SizeType length, bool copy)
  {
    enter("KEY");
    enter(str);
    if (in(State::IN_INPUT) && strncmp(str, "binding", length) == 0) {
      push(State::IN_INPUT_BINDING);
    } else if (in(State::IN_INPUT) && strncmp(str, "origin", length) == 0) {
      push(State::IN_INPUT_ORIGIN);
    } else if (in(State::IN_INPUT) && strncmp(str, "description", length) == 0) {
      push(State::IN_INPUT_DESCRIPTION);
    } else if (in(State::IN_INPUT) && strncmp(str, "subspec", length) == 0) {
      push(State::IN_INPUT_SUBSPEC);
    } else if (in(State::IN_INPUT) && strncmp(str, "matcher", length) == 0) {
      assert(0);
      // the outermost matcher is starting here
      // we create a placeholder which is being updated later
      inputMatcherNodes.push_back(std::make_unique<DataDescriptorMatcher>(DataDescriptorMatcher::Op::And, ConstantValueMatcher{false}));
      push(State::IN_INPUT_MATCHER);
    } else if (in(State::IN_INPUT_MATCHER) && strncmp(str, "matcher", length) == 0) {
      // recursive matchers, can maybe combine with above
      push(State::IN_INPUT_MATCHER);
    } else if (in(State::IN_INPUT_MATCHER) && strncmp(str, "operation", length) == 0) {
      push(State::IN_INPUT_MATCHER_OPERATION);
    } else if (in(State::IN_INPUT_MATCHER) && strncmp(str, "left", length) == 0) {
      push(State::IN_INPUT_LEFT_MATCHER);
    } else if (in(State::IN_INPUT_MATCHER) && strncmp(str, "right", length) == 0) {
      push(State::IN_INPUT_RIGHT_MATCHER);
    } else if (in(State::IN_INPUT_LEFT_MATCHER) && strncmp(str, "origin", length) == 0) {
      push(State::IN_INPUT_ORIGIN);
    } else if (in(State::IN_INPUT_RIGHT_MATCHER) && strncmp(str, "origin", length) == 0) {
      push(State::IN_INPUT_ORIGIN);
    } else if (in(State::IN_INPUT_LEFT_MATCHER) && strncmp(str, "description", length) == 0) {
      push(State::IN_INPUT_DESCRIPTION);
    } else if (in(State::IN_INPUT_RIGHT_MATCHER) && strncmp(str, "description", length) == 0) {
      push(State::IN_INPUT_DESCRIPTION);
    } else if (in(State::IN_INPUT_LEFT_MATCHER) && strncmp(str, "subspec", length) == 0) {
      push(State::IN_INPUT_SUBSPEC);
    } else if (in(State::IN_INPUT_RIGHT_MATCHER) && strncmp(str, "subspec", length) == 0) {
      push(State::IN_INPUT_SUBSPEC);
    } else if (in(State::IN_INPUT) && strncmp(str, "lifetime", length) == 0) {
      push(State::IN_INPUT_LIFETIME);
    } else if (in(State::IN_INPUT) && strncmp(str, "metadata", length) == 0) {
      push(State::IN_INPUT_OPTIONS);
    } else if (in(State::IN_OUTPUT) && strncmp(str, "binding", length) == 0) {
      push(State::IN_OUTPUT_BINDING);
    } else if (in(State::IN_OUTPUT) && strncmp(str, "origin", length) == 0) {
      push(State::IN_OUTPUT_ORIGIN);
    } else if (in(State::IN_OUTPUT) && strncmp(str, "description", length) == 0) {
      push(State::IN_OUTPUT_DESCRIPTION);
    } else if (in(State::IN_OUTPUT) && strncmp(str, "subspec", length) == 0) {
      push(State::IN_OUTPUT_SUBSPEC);
      outputHasSubSpec = true;
    } else if (in(State::IN_OUTPUT) && strncmp(str, "lifetime", length) == 0) {
      push(State::IN_OUTPUT_LIFETIME);
    } else if (in(State::IN_OUTPUT) && strncmp(str, "metadata", length) == 0) {
      push(State::IN_OUTPUT_OPTIONS);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "name", length) == 0) {
      push(State::IN_DATAPROCESSOR_NAME);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "ranks", length) == 0) {
      push(State::IN_DATAPROCESSOR_RANK);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "nSlots", length) == 0) {
      push(State::IN_DATAPROCESSOR_N_SLOTS);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "inputTimeSliceId", length) == 0) {
      push(State::IN_DATAPROCESSOR_TIMESLICE_ID);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "maxInputTimeslices", length) == 0) {
      push(State::IN_DATAPROCESSOR_MAX_TIMESLICES);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "inputs", length) == 0) {
      push(State::IN_INPUTS);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "outputs", length) == 0) {
      push(State::IN_OUTPUTS);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "options", length) == 0) {
      push(State::IN_OPTIONS);
    } else if (in(State::IN_DATAPROCESSOR) && strncmp(str, "labels", length) == 0) {
      push(State::IN_LABELS);
    } else if (in(State::IN_EXECUTION) && strncmp(str, "workflow", length) == 0) {
      push(State::IN_WORKFLOW);
    } else if (in(State::IN_EXECUTION) && strncmp(str, "metadata", length) == 0) {
      push(State::IN_METADATA);
    } else if (in(State::IN_OPTION) && strncmp(str, "name", length) == 0) {
      push(State::IN_OPTION_NAME);
    } else if (in(State::IN_OPTION) && strncmp(str, "type", length) == 0) {
      push(State::IN_OPTION_TYPE);
    } else if (in(State::IN_OPTION) && strncmp(str, "defaultValue", length) == 0) {
      push(State::IN_OPTION_DEFAULT);
    } else if (in(State::IN_OPTION) && strncmp(str, "help", length) == 0) {
      push(State::IN_OPTION_HELP);
    } else if (in(State::IN_OPTION) && strncmp(str, "kind", length) == 0) {
      push(State::IN_OPTION_KIND);
    } else if (in(State::IN_METADATUM) && strncmp(str, "name", length) == 0) {
      push(State::IN_METADATUM_NAME);
    } else if (in(State::IN_METADATUM) && strncmp(str, "executable", length) == 0) {
      push(State::IN_METADATUM_EXECUTABLE);
    } else if (in(State::IN_METADATUM) && strncmp(str, "cmdLineArgs", length) == 0) {
      push(State::IN_METADATUM_ARGS);
    } else if (in(State::IN_METADATUM) && strncmp(str, "workflowOptions", length) == 0) {
      push(State::IN_WORKFLOW_OPTIONS);
    } else if (in(State::IN_METADATUM) && strncmp(str, "channels", length) == 0) {
      push(State::IN_METADATUM_CHANNELS);
    } else if (in(State::IN_EXECUTION) && strncmp(str, "command", length) == 0) {
      push(State::IN_COMMAND);
    }
    return true;
  }

  bool String(const Ch* str, SizeType length, bool copy)
  {
    enter("STRING");
    enter(str);
    auto s = std::string(str, length);
    if (in(State::IN_DATAPROCESSOR_NAME)) {
      assert(dataProcessors.size());
      dataProcessors.back().name = s;
    } else if (in(State::IN_METADATUM_NAME)) {
      assert(metadata.size());
      metadata.back().name = s;
    } else if (in(State::IN_METADATUM_EXECUTABLE)) {
      assert(metadata.size());
      metadata.back().executable = s;
    } else if (in(State::IN_INPUT_BINDING)) {
      binding = s;
    } else if (in(State::IN_INPUT_ORIGIN)) {
      origin.runtimeInit(s.c_str(), std::min(s.size(), 4UL));
      std::string v(s.c_str(), std::min(s.size(), 4UL));
      inputMatcherNodes.push_back(OriginValueMatcher{v});
    } else if (in(State::IN_INPUT_DESCRIPTION)) {
      description.runtimeInit(s.c_str(), std::min(s.size(), 16UL));
      std::string v(s.c_str(), std::min(s.size(), 16UL));
      inputMatcherNodes.push_back(DescriptionValueMatcher{v});
    } else if (in(State::IN_INPUT_MATCHER_OPERATION)) {
      // FIXME: read operation
    } else if (in(State::IN_OUTPUT_BINDING)) {
      binding = s;
    } else if (in(State::IN_OUTPUT_ORIGIN)) {
      origin.runtimeInit(s.c_str(), std::min(s.size(), 4UL));
    } else if (in(State::IN_OUTPUT_DESCRIPTION)) {
      description.runtimeInit(s.c_str(), std::min(s.size(), 16UL));
    } else if (in(State::IN_OPTION_NAME)) {
      optionName = s;
    } else if (in(State::IN_OPTION_TYPE)) {
      optionType = (VariantType)std::stoi(s, nullptr);
    } else if (in(State::IN_OPTION_KIND)) {
      optionKind = (ConfigParamKind)std::stoi(s, nullptr);
    } else if (in(State::IN_OPTION_DEFAULT)) {
      optionDefault = s;
    } else if (in(State::IN_OPTION_HELP)) {
      optionHelp = s;
    } else if (in(State::IN_LABEL)) {
      dataProcessors.back().labels.push_back({s});
      // This is in an array, so we do not actually want to
      // exit from the state.
      push(State::IN_LABEL);
    } else if (in(State::IN_METADATUM_ARG)) {
      metadata.back().cmdLineArgs.push_back(s);
      // This is in an array, so we do not actually want to
      // exit from the state.
      push(State::IN_METADATUM_ARG);
    } else if (in(State::IN_METADATUM_CHANNEL)) {
      metadata.back().channels.push_back(s);
      // This is in an array, so we do not actually want to
      // exit from the state.
      push(State::IN_METADATUM_CHANNEL);
    } else if (in(State::IN_COMMAND)) {
      command.merge({s});
    }
    pop();
    return true;
  }

  bool Uint(unsigned i)
  {
    debug << "Uint(" << i << ")" << std::endl;
    if (in(State::IN_INPUT_SUBSPEC)) {
      subspec = i;
      inputMatcherNodes.push_back(SubSpecificationTypeValueMatcher{i});
    } else if (in(State::IN_OUTPUT_SUBSPEC)) {
      subspec = i;
    } else if (in(State::IN_INPUT_LIFETIME)) {
      lifetime = (Lifetime)i;
    } else if (in(State::IN_OUTPUT_LIFETIME)) {
      lifetime = (Lifetime)i;
    } else if (in(State::IN_DATAPROCESSOR_RANK)) {
      dataProcessors.back().rank = i;
    } else if (in(State::IN_DATAPROCESSOR_N_SLOTS)) {
      dataProcessors.back().nSlots = i;
    } else if (in(State::IN_DATAPROCESSOR_TIMESLICE_ID)) {
      dataProcessors.back().inputTimeSliceId = i;
    } else if (in(State::IN_DATAPROCESSOR_MAX_TIMESLICES)) {
      dataProcessors.back().maxInputTimeslices = i;
    }
    pop();
    return true;
  }

  bool Int(int i)
  {
    debug << "Int(" << i << ")" << std::endl;
    return true;
  }
  bool Uint64(uint64_t u)
  {
    debug << "Uint64(" << u << ")" << std::endl;
    return true;
  }
  bool Double(double d)
  {
    debug << "Double(" << d << ")" << std::endl;
    return true;
  }

  void enter(char const* what)
  {
    debug << "ENTER: " << what << std::endl;
  }
  void push(State state)
  {
    debug << "PUSH: " << state << std::endl;
    states.push_back(state);
  }

  State pop()
  {
    if (states.empty()) {
      states.push_back(State::IN_ERROR);
      return State::IN_ERROR;
    }
    auto result = states.back();
    states.pop_back();
    debug << "POP: " << result;
    if (!states.empty()) {
      debug << " now in " << states.back();
    }
    debug << std::endl;
    return result;
  }
  bool in(State o)
  {
    return states.back() == o;
  }

  bool previousIs(State o)
  {
    assert(states.size() > 1);
    return states[states.size() - 2] == o;
  }

  std::ostringstream debug;
  std::vector<State> states;
  std::string spec;
  std::vector<DataProcessorSpec>& dataProcessors;
  std::vector<DataProcessorInfo>& metadata;
  CommandInfo& command;
  std::vector<ConfigParamSpec> inputOptions;
  std::vector<ConfigParamSpec> outputOptions;
  std::string binding;
  header::DataOrigin origin;
  header::DataDescription description;
  size_t subspec;
  Lifetime lifetime;
  std::string optionName;
  VariantType optionType;
  std::string optionDefault;
  std::string optionHelp;
  ConfigParamKind optionKind;
  bool outputHasSubSpec;
  std::vector<data_matcher::Node> inputMatcherNodes;
};

bool WorkflowSerializationHelpers::import(std::istream& s,
                                          std::vector<DataProcessorSpec>& workflow,
                                          std::vector<DataProcessorInfo>& metadata,
                                          CommandInfo& command)
{
  // Skip any line which does not start with '{'
  // If we do not find a starting {, we simply assume that no workflow
  // was actually passed on the PIPE.
  // FIXME: not particularly resilient, but works for now.
  // FIXME: this will fail if { is found at char 1024.
  char buf[1024];
  bool hasFatalImportError = false;
  while (s.peek() != '{') {
    if (s.eof()) {
      return !hasFatalImportError;
    }
    if (s.fail() || s.bad()) {
      throw std::runtime_error("Malformatted input workflow");
    }
    s.getline(buf, 1024, '\n');
    // FairLogger messages (starting with [) simply get forwarded.
    // Other messages we consider them as ERRORs since they
    // were printed out without FairLogger.
    if (buf[0] == '[') {
      if (strncmp(buf, "[ERROR] invalid workflow in", strlen("[ERROR] invalid workflow in")) == 0 ||
          strncmp(buf, "[ERROR] error while setting up workflow", strlen("[ERROR] error while setting up workflow")) == 0 ||
          strncmp(buf, "[ERROR] error parsing options of", strlen("[ERROR] error parsing options of")) == 0) {
        hasFatalImportError = true;
      }
      std::cout << buf << std::endl;
    } else {
      LOG(error) << buf;
    }
  }
  if (hasFatalImportError) {
    return false;
  }
  rapidjson::Reader reader;
  rapidjson::IStreamWrapper isw(s);
  WorkflowImporter importer{workflow, metadata, command};
  bool ok = reader.Parse(isw, importer);
  if (ok == false) {
    throw std::runtime_error("Error while parsing serialised workflow");
  }
  return true;
}

void WorkflowSerializationHelpers::dump(std::ostream& out,
                                        std::vector<DataProcessorSpec> const& workflow,
                                        std::vector<DataProcessorInfo> const& metadata,
                                        CommandInfo const& commandInfo)
{
  rapidjson::OStreamWrapper osw(out);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> w(osw);

  w.StartObject();
  w.Key("workflow");
  w.StartArray();

  for (auto& processor : workflow) {
    if (processor.name.rfind("internal-dpl", 0) == 0) {
      continue;
    }
    w.StartObject();
    w.Key("name");
    w.String(processor.name.c_str());

    w.Key("inputs");
    w.StartArray();
    for (auto& input : processor.inputs) {
      /// FIXME: this only works for a selected set of InputSpecs...
      ///        a proper way to fully serialize an InputSpec with
      ///        a DataDescriptorMatcher is needed.
      w.StartObject();
      w.Key("binding");
      w.String(input.binding.c_str());
      auto origin = DataSpecUtils::getOptionalOrigin(input);
      if (origin.has_value()) {
        w.Key("origin");
        w.String(origin->str, strnlen(origin->str, 4));
      }
      auto description = DataSpecUtils::getOptionalDescription(input);
      if (description.has_value()) {
        w.Key("description");
        w.String(description->str, strnlen(description->str, 16));
      }
      auto subSpec = DataSpecUtils::getOptionalSubSpec(input);
      if (subSpec.has_value()) {
        w.Key("subspec");
        w.Uint64(*subSpec);
      }
      w.Key("lifetime");
      w.Uint((int)input.lifetime);
      if (input.metadata.empty() == false) {
        w.Key("metadata");
        w.StartArray();
        for (auto& metadata : input.metadata) {
          w.StartObject();
          w.Key("name");
          w.String(metadata.name.c_str());
          auto s = std::to_string(int(metadata.type));
          w.Key("type");
          w.String(s.c_str());
          std::ostringstream oss;
          oss << metadata.defaultValue;
          w.Key("defaultValue");
          w.String(oss.str().c_str());
          w.Key("help");
          w.String(metadata.help.c_str());
          w.EndObject();
        }
        w.EndArray();
      }
      w.EndObject();
    }
    w.EndArray();

    w.Key("outputs");
    w.StartArray();
    for (auto& output : processor.outputs) {
      w.StartObject();
      w.Key("binding");
      if (output.binding.value.empty()) {
        auto autogenerated = DataSpecUtils::describe(output);
        w.String(autogenerated.c_str());
      } else {
        w.String(output.binding.value.c_str());
      }
      ConcreteDataTypeMatcher dataType = DataSpecUtils::asConcreteDataTypeMatcher(output);
      w.Key("origin");
      w.String(dataType.origin.str, strnlen(dataType.origin.str, 4));
      w.Key("description");
      w.String(dataType.description.str, strnlen(dataType.description.str, 16));
      // FIXME: this will have to change once we introduce wildcards for
      //        OutputSpec
      auto subSpec = DataSpecUtils::getOptionalSubSpec(output);
      if (subSpec.has_value()) {
        w.Key("subspec");
        w.Uint64(*subSpec);
      }
      w.Key("lifetime");
      w.Uint((int)output.lifetime);
      if (output.metadata.empty() == false) {
        w.Key("metadata");
        w.StartArray();
        for (auto& metadata : output.metadata) {
          w.StartObject();
          w.Key("name");
          w.String(metadata.name.c_str());
          auto s = std::to_string(int(metadata.type));
          w.Key("type");
          w.String(s.c_str());
          std::ostringstream oss;
          oss << metadata.defaultValue;
          w.Key("defaultValue");
          w.String(oss.str().c_str());
          w.Key("help");
          w.String(metadata.help.c_str());
          w.EndObject();
        }
        w.EndArray();
      }
      w.EndObject();
    }
    w.EndArray();

    w.Key("options");
    w.StartArray();
    for (auto& option : processor.options) {
      if (option.name == "start-value-enumeration" || option.name == "end-value-enumeration" || option.name == "step-value-enumeration" || option.name == "orbit-offset-enumeration" || option.name == "orbit-multiplier-enumeration") {
        continue;
      }
      w.StartObject();
      w.Key("name");
      w.String(option.name.c_str());
      auto s = std::to_string(int(option.type));
      w.Key("type");
      w.String(s.c_str());
      std::ostringstream oss;
      switch (option.type) {
        case VariantType::ArrayInt:
        case VariantType::ArrayFloat:
        case VariantType::ArrayDouble:
        case VariantType::ArrayBool:
        case VariantType::ArrayString:
        case VariantType::Array2DInt:
        case VariantType::Array2DFloat:
        case VariantType::Array2DDouble:
        case VariantType::LabeledArrayInt:
        case VariantType::LabeledArrayFloat:
        case VariantType::LabeledArrayDouble:
        case VariantType::Dict:
          VariantJSONHelpers::write(oss, option.defaultValue);
          break;
        default:
          oss << option.defaultValue;
          break;
      }
      w.Key("defaultValue");
      w.String(oss.str().c_str());
      w.Key("help");
      w.String(option.help.c_str());
      w.Key("kind");
      w.String(std::to_string((int)option.kind).c_str());
      w.EndObject();
    }
    w.EndArray();
    w.Key("labels");
    w.StartArray();
    for (auto& label : processor.labels) {
      w.String(label.value.c_str());
    }
    w.EndArray();
    w.Key("rank");
    w.Int(processor.rank);
    w.Key("nSlots");
    w.Int(processor.nSlots);
    w.Key("inputTimeSliceId");
    w.Int(processor.inputTimeSliceId);
    w.Key("maxInputTimeslices");
    w.Int(processor.maxInputTimeslices);

    w.EndObject();
  }
  w.EndArray();

  w.Key("metadata");
  w.StartArray();
  for (auto& info : metadata) {
    w.StartObject();
    w.Key("name");
    w.String(info.name.c_str());
    w.Key("executable");
    w.String(info.executable.c_str());
    w.Key("cmdLineArgs");
    w.StartArray();
    for (auto& arg : info.cmdLineArgs) {
      w.String(arg.c_str());
    }
    w.EndArray();
    w.Key("workflowOptions");
    w.StartArray();
    for (auto& option : info.workflowOptions) {
      w.StartObject();
      w.Key("name");
      w.String(option.name.c_str());
      auto s = std::to_string(int(option.type));
      w.Key("type");
      w.String(s.c_str());
      std::ostringstream oss;
      oss << option.defaultValue;
      w.Key("defaultValue");
      w.String(oss.str().c_str());
      w.Key("help");
      w.String(option.help.c_str());
      w.EndObject();
    }
    w.EndArray();
    w.Key("channels");
    w.StartArray();
    for (auto& channel : info.channels) {
      w.String(channel.c_str());
    }
    w.EndArray();
    w.EndObject();
  }
  w.EndArray();

  w.Key("command");
  w.String(commandInfo.command.c_str());

  w.EndObject();
}

} // namespace o2::framework
