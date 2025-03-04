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
#ifndef FRAMEWORK_DATASPECUTILS_H
#define FRAMEWORK_DATASPECUTILS_H

#include "Framework/OutputSpec.h"
#include "Framework/InputSpec.h"
#include "Headers/DataHeader.h"
#include "Framework/Output.h"

#include <optional>

namespace o2
{
namespace framework
{

struct DataSpecUtils {
  /// @return true if a given InputSpec @a spec matches with a @a target ConcreteDataMatcher
  static bool match(InputSpec const& spec, ConcreteDataMatcher const& target);

  /// @return true if a given InputSpec @a spec matches with a @a target ConcreteDataTypeMatcher
  static bool match(InputSpec const& spec, ConcreteDataTypeMatcher const& target);

  /// @return true if matchers of the two specs match on all three levels if both
  /// matchers are of type ConcreteDataMatcher, on the level of origin and description
  /// otherwise
  static bool match(OutputSpec const& left, OutputSpec const& right);

  /// @return true if a given OutputSpec @a spec matches with a @a target ConcreteDataMatcher
  static bool match(OutputSpec const& spec, ConcreteDataMatcher const& target);

  /// @return true if a given InputSpec @a input  matches the @a output outputspec
  static bool match(InputSpec const& input, OutputSpec const& output);

  static bool match(const InputSpec& spec,
                    const o2::header::DataOrigin& origin,
                    const o2::header::DataDescription& description,
                    const o2::header::DataHeader::SubSpecificationType& subSpec);

  static bool match(const OutputSpec& spec,
                    const o2::header::DataOrigin& origin,
                    const o2::header::DataDescription& description,
                    const o2::header::DataHeader::SubSpecificationType& subSpec);

  /// find a matching spec in the container
  /// @return std::optional with found spec or std::nullopt
  template <typename ContainerT>
  static std::optional<typename ContainerT::value_type> find(ContainerT const& container,
                                                             const o2::header::DataOrigin& origin,
                                                             const o2::header::DataDescription& description,
                                                             const o2::header::DataHeader::SubSpecificationType& subSpec)
  {
    for (auto const& spec : container) {
      if (match(spec, origin, description, subSpec)) {
        return std::make_optional<typename ContainerT::value_type>(spec);
      }
    }
    return std::nullopt;
  }

  /// @return true if the InputSpec will match at least the provided @a origin.
  static bool partialMatch(InputSpec const& spec, o2::header::DataOrigin const& origin);

  /// @return true if the OutputSpec will match at least the provided @a origin.
  static bool partialMatch(OutputSpec const& spec, o2::header::DataOrigin const& origin);

  /// @return true if the OutputSpec will match at least the provided @a description.
  static bool partialMatch(InputSpec const& spec, o2::header::DataDescription const& description);

  /// @return true if the OutputSpec will match at least the provided @a description.
  static bool partialMatch(OutputSpec const& spec, o2::header::DataDescription const& description);

  template <typename T>
  static bool match(const T& spec, const o2::header::DataHeader& header)
  {
    return DataSpecUtils::match(spec,
                                header.dataOrigin,
                                header.dataDescription,
                                header.subSpecification);
  }

  /// Describes an InputSpec. Use this to get some human readable
  /// version of the contents of the InputSpec. Notice this is not part
  /// of the InputSpec API, because there is no unique way a description should
  /// be done, so we keep this outside.
  static std::string describe(InputSpec const& spec);

  /// Describes an OutputSpec. Use this to get some human readable
  /// version of the contents of the OutputSpec.
  ///
  /// @note: Notice this is not part of the InputSpec API, because there is no
  /// unique way a description should be done, so we keep this outside.
  static std::string describe(OutputSpec const& spec);

  /// Provide a unique label for the input spec. Again this is outside because there
  /// is no standard way of doing it, so better not to pollute the API.
  static std::string label(InputSpec const& spec);

  /// Provide a unique label for the input spec. Again this is outside because there
  /// is no standard way of doing it, so better not to pollute the API.
  static std::string label(OutputSpec const& spec);

  /// Provides the to be used as suffix for any REST endpoint related
  /// to the @a spec.
  static std::string restEndpoint(InputSpec const& spec);

  /// Given an InputSpec, manipulate it so that it will match only the given
  /// subSpec.
  static void updateMatchingSubspec(InputSpec& in, header::DataHeader::SubSpecificationType subSpec);
  /// Given an OutputSpec, manipulate it so that it will match only the given
  /// subSpec.
  static void updateMatchingSubspec(OutputSpec& in, header::DataHeader::SubSpecificationType subSpec);

  /// @return true if the given InputSpec @a input is valid.
  static bool validate(InputSpec const& input);

  /// @return true if the given OutputSpec @a out is valid.
  static bool validate(OutputSpec const& output);

  /// Same as the other describe, but uses a buffer to reduce memory churn.
  static void describe(char* buffer, size_t size, InputSpec const& spec);

  /// Same as the other describe, but uses a buffer to reduce memory churn.
  static void describe(char* buffer, size_t size, OutputSpec const& spec);

  /// If possible extract the ConcreteDataMatcher from an InputSpec. This
  /// can be done either if the InputSpec is defined in terms for a ConcreteDataMatcher
  /// or if the query can be uniquely assigned to a ConcreteDataMatcher.
  static ConcreteDataMatcher asConcreteDataMatcher(InputSpec const& input);

  /// If possible extract the ConcreteDataMatcher from an OutputSpec.
  /// For the moment this is trivial as the OutputSpec does not allow
  /// for wildcards.
  static ConcreteDataMatcher asConcreteDataMatcher(OutputSpec const& spec);

  /// If possible extract the ConcreteTypeDataMatcher from an OutputSpec.
  /// This will always be possible, but implementation will have to
  /// take into account the fact that the OutputSpec might have a wildcard on
  /// the subSpec.
  static ConcreteDataTypeMatcher asConcreteDataTypeMatcher(OutputSpec const& spec);

  /// If possible extract the ConcreteTypeDataMatcher from an InputSpec.
  /// This will not always be possible, depending on how complex of
  /// a query the InputSpec does, however in most cases it should be ok
  /// and we can add corner cases as we go.
  static ConcreteDataTypeMatcher asConcreteDataTypeMatcher(InputSpec const& spec);

  /// If possible extract the DataOrigin from an InputSpec.
  /// This will not always be possible, depending on how complex of
  /// a query the InputSpec does, however in most cases it should be ok
  /// and we can add corner cases as we go.
  static header::DataOrigin asConcreteOrigin(InputSpec const& spec);

  /// If possible extract the DataDescription from an InputSpec.
  /// This will not always be possible, depending on how complex of
  /// a query the InputSpec does, however in most cases it should be ok
  /// and we can add corner cases as we go.
  static header::DataDescription asConcreteDataDescription(InputSpec const& spec);

  /// If possible extract either ConcreteTypeDataMatcher or ConcreteDataMatcher
  /// from an InputSpec and assign it to the matcher of the OutputSpec together
  /// with binding and lifetime
  /// If InputSpec holds a ConcreteDataMatcher, this will be used directly, if it
  /// is a DataDescriptorMatcher, depending on its complexity the ConcreteDataMatcher
  /// or ConcreteTypeDataMatcher will be created
  static OutputSpec asOutputSpec(InputSpec const& spec);

  /// Create an InputSpec which is able to match all the outputs of the given
  /// OutputSpec
  static InputSpec matchingInput(OutputSpec const& spec);

  /// Create an InputSpec from metadata string
  static InputSpec fromMetadataString(std::string s);

  /// Get the origin, if available
  static std::optional<header::DataOrigin> getOptionalOrigin(InputSpec const& spec);

  /// Get the description, if available
  static std::optional<header::DataDescription> getOptionalDescription(InputSpec const& spec);

  /// Get the subspec, if available
  static std::optional<header::DataHeader::SubSpecificationType> getOptionalSubSpec(OutputSpec const& spec);

  /// Get the subspec, if available
  static std::optional<header::DataHeader::SubSpecificationType> getOptionalSubSpec(InputSpec const& spec);

  /// Build a DataDescriptMatcher which does not care about the subSpec.
  static data_matcher::DataDescriptorMatcher dataDescriptorMatcherFrom(ConcreteDataMatcher const& concrete);

  /// Build a DataDescriptMatcher which does not care about the subSpec.
  static data_matcher::DataDescriptorMatcher dataDescriptorMatcherFrom(ConcreteDataTypeMatcher const& dataType);

  /// Build a DataDescriptMatcher which does not care about the subSpec and description.
  static data_matcher::DataDescriptorMatcher dataDescriptorMatcherFrom(header::DataOrigin const& origin);

  /// Build a DataDescriptMatcher which does not care about the subSpec and origin.
  static data_matcher::DataDescriptorMatcher dataDescriptorMatcherFrom(header::DataDescription const& origin);

  /// return fully qualified ConcreteDataMatcher if DataMatcher is connecting unique properties
  /// via 'and' operation
  static std::optional<framework::ConcreteDataMatcher> optionalConcreteDataMatcherFrom(data_matcher::DataDescriptorMatcher const& matcher);

  /// Checks if left includes right (or is equal to)
  static bool includes(const InputSpec& left, const InputSpec& right);

  /// Updates list of InputSpecs by merging metadata
  static void updateInputList(std::vector<InputSpec>& list, InputSpec&& input);
};

} // namespace framework
} // namespace o2
#endif // FRAMEWORK_DATASPECUTILS_H
