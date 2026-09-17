#pragma once

#include <cstddef>
#include <meta>
#include <vector>

#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/spine/members.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand::spine {

consteval bool isGiven(std::meta::info type) {
  return type == ^^tokenized::Sources || type == ^^text::StringPools;
}

consteval bool producedBefore(const std::vector<std::meta::info>& facts,
                              std::size_t index, std::meta::info type) {
  for (std::size_t earlier = 0; earlier < index; ++earlier)
    if (std::meta::return_type_of(facts[earlier]) == type)
      return true;
  return false;
}

// NOTE: Each one is declared and never defined. A consteval call to it ends
// the compile with the rule in the diagnostic. The argument is the offending
// member's position in the namespace.
namespace broken {
void aFactMustReturnAValue(std::size_t fact);
void aFactMustNotReturnAGiven(std::size_t fact);
void aFactReturnsAnUnqualifiedValue(std::size_t fact);
void twoFactsMustNotReturnOneType(std::size_t fact);
void aFactReadsThroughConstReferencesOnly(std::size_t fact);
void aFactMayOnlyReadWhatIsAlreadyOnTheBoard(std::size_t fact);
void aReportReturnsNothing(std::size_t report);
void aReportReadsExactlyOneFact(std::size_t report);
void oneFactMayHaveOneReport(std::size_t report);
} // namespace broken

consteval void validateFacts(const std::vector<std::meta::info>& facts) {
  for (std::size_t index = 0; index < facts.size(); ++index) {
    const auto result = std::meta::return_type_of(facts[index]);
    if (std::meta::is_void_type(result) ||
        std::meta::is_reference_type(result))
      broken::aFactMustReturnAValue(index);
    if (result != std::meta::remove_cv(result))
      broken::aFactReturnsAnUnqualifiedValue(index);
    if (isGiven(result))
      broken::aFactMustNotReturnAGiven(index);
    if (producedBefore(facts, index, result))
      broken::twoFactsMustNotReturnOneType(index);
    for (const auto parameter : std::meta::parameters_of(facts[index])) {
      if (!isRead(parameter))
        broken::aFactReadsThroughConstReferencesOnly(index);
      const auto read = readOf(parameter);
      if (!isGiven(read) && !producedBefore(facts, index, read))
        broken::aFactMayOnlyReadWhatIsAlreadyOnTheBoard(index);
    }
  }
}

consteval void validateReports(const std::vector<std::meta::info>& facts,
                               const std::vector<std::meta::info>& reports) {
  for (std::size_t index = 0; index < reports.size(); ++index) {
    if (!std::meta::is_void_type(std::meta::return_type_of(reports[index])))
      broken::aReportReturnsNothing(index);
    const auto parameters = std::meta::parameters_of(reports[index]);
    if (parameters.size() != 1 || !isRead(parameters[0]) ||
        !producedBefore(facts, facts.size(), readOf(parameters[0])))
      broken::aReportReadsExactlyOneFact(index);
    for (std::size_t earlier = 0; earlier < index; ++earlier)
      if (readOf(std::meta::parameters_of(reports[earlier])[0]) ==
          readOf(parameters[0]))
        broken::oneFactMayHaveOneReport(index);
  }
}

consteval std::vector<std::meta::info> validatedFacts(std::meta::info space) {
  auto facts = tagged(space, FactTag::Fact);
  validateFacts(facts);
  return facts;
}

consteval std::vector<std::meta::info>
validatedReports(std::meta::info space) {
  auto reports = tagged(space, FactTag::Report);
  validateReports(tagged(space, FactTag::Fact), reports);
  return reports;
}

template <std::meta::info Space>
inline constexpr auto kFacts = std::define_static_array(validatedFacts(Space));
template <std::meta::info Space>
inline constexpr auto kReports =
    std::define_static_array(validatedReports(Space));

} // namespace wl_stable::expand::spine
