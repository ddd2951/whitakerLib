#pragma once

// INFO: Gen's execution steps, one namespace per phase.
//
//  A step's tags are all the runner knows about it: Gate names a step it follows; Reads and Writes name the shared
//  values it touches (storage:: and whitaker::relationship::sImage).
//
//  run.hpp discovers the steps through reflection, validates their tags at build time, and derives the execution
//  order and what may run at once. Nothing else lists the steps.
//
// NOTE: Reads and Writes are development scaffolding, not a stable part of gen: a ledger kept by hand from a static
//  trace of each step. This is the known weak point: the build checks that a tag names a real value, not that the
//  ledger is complete, and a missing one can cause a data race (a ThreadSanitizer build of gen helps find it).
// TODO: Storage records each access, and this file is generated from that record.

#include "execution_steps/tags.hpp"
#include "search/image.hpp"
#include "storage/addon_rows.hpp"
#include "storage/addons.hpp"
#include "storage/array_order.hpp"
#include "storage/back.hpp"
#include "storage/below.hpp"
#include "storage/by_stem.hpp"
#include "storage/candidate_capacity.hpp"
#include "storage/candidate_counts.hpp"
#include "storage/cell_of.hpp"
#include "storage/cells.hpp"
#include "storage/class_grammars.hpp"
#include "storage/classes.hpp"
#include "storage/compression_duplicates.hpp"
#include "storage/description_ids.hpp"
#include "storage/dictline.hpp"
#include "storage/encoded_image.hpp"
#include "storage/ending_ids.hpp"
#include "storage/ending_order.hpp"
#include "storage/ending_runs.hpp"
#include "storage/endings.hpp"
#include "storage/front.hpp"
#include "storage/image.hpp"
#include "storage/image_destination.hpp"
#include "storage/image_spellings.hpp"
#include "storage/inflects.hpp"
#include "storage/kept.hpp"
#include "storage/lines.hpp"
#include "storage/loaded_image.hpp"
#include "storage/lookup_metadata.hpp"
#include "storage/max_rows.hpp"
#include "storage/mixed.hpp"
#include "storage/order.hpp"
#include "storage/packons.hpp"
#include "storage/perfect_suffixes.hpp"
#include "storage/records.hpp"
#include "storage/result.hpp"
#include "storage/reversed.hpp"
#include "storage/root.hpp"
#include "storage/row_ids.hpp"
#include "storage/rows.hpp"
#include "storage/rules.hpp"
#include "storage/source.hpp"
#include "storage/starts.hpp"
#include "storage/states.hpp"
#include "storage/stem_runs.hpp"
#include "storage/stems.hpp"
#include "storage/suffix_base.hpp"
#include "storage/temporary_image.hpp"
#include "storage/total.hpp"
#include "storage/uniques.hpp"

namespace execution_steps {

namespace dictline {
[[= tag::Writes{^^storage::lines<^^source::scheme::dictline>}]] void readDictline();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Writes{^^storage::records<^^source::scheme::dictline>}
]] void indexDictlineRecords();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::records<^^source::scheme::dictline>}, = tag::Writes{^^storage::dictline}
]] void parseDictlineRecords();
[[= tag::Writes{^^storage::dictline}]] void addEsse();
} // namespace dictline

namespace inflects {
[[= tag::Writes{^^storage::lines<^^source::scheme::inflects>}]] void readInflects();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::inflects>},
  = tag::Writes{^^storage::records<^^source::scheme::inflects>}
]] void indexInflectsRecords();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::inflects>},
  = tag::Reads{^^storage::records<^^source::scheme::inflects>}, = tag::Writes{^^storage::inflects}
]] void parseInflectsRecords();
[[= tag::Writes{^^storage::inflects}]] void addAdverbInflections();
} // namespace inflects

namespace uniques {
[[= tag::Writes{^^storage::lines<^^source::scheme::uniques>}]] void readUniques();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Writes{^^storage::records<^^source::scheme::uniques>}
]] void indexUniquesRecords();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Reads{^^storage::records<^^source::scheme::uniques>}, = tag::Writes{^^storage::uniques}
]] void parseUniquesRecords();
} // namespace uniques

namespace addons {
[[= tag::Writes{^^storage::lines<^^source::scheme::addons>}]] void readAddons();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::addons>}, = tag::Writes{^^storage::records<^^source::scheme::addons>}
]] void indexAddonsRecords();
[[
  = tag::Reads{^^storage::lines<^^source::scheme::addons>}, = tag::Reads{^^storage::records<^^source::scheme::addons>},
  = tag::Writes{^^storage::addons}
]] void parseAddonsRecords();
} // namespace addons

namespace candidates {
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::inflects},
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::lines<^^source::scheme::inflects>}, = tag::Writes{^^storage::kept}
]] void makeCandidates();
} // namespace candidates

namespace expansion {
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::inflects}, = tag::Reads{^^storage::uniques},
  = tag::Reads{^^storage::lines<^^source::scheme::uniques>}, = tag::Writes{^^storage::kept}
]] void prepareForms();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::inflects}, = tag::Reads{^^storage::uniques},
  = tag::Writes{^^storage::kept}
]] void makeReadings();
[[
  = tag::Reads{^^storage::inflects}, = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::lines<^^source::scheme::inflects>}, = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Writes{^^storage::kept}
]] void sweepReadings();
[[ = tag::Reads{^^storage::kept}, = tag::Writes{^^storage::starts},
   = tag::Writes{^^storage::total} ]] void indexResults();
} // namespace expansion

namespace image_destination {
[[= tag::Writes{^^storage::destination}]] void prepareImageDestination();
} // namespace image_destination

namespace image_data {
[[ = tag::Reads{^^storage::addons}, = tag::Writes{^^storage::addonRows} ]] void classifyAddons();
[[
  = tag::Reads{^^storage::addons}, = tag::Reads{^^storage::addonRows},
  = tag::Reads{^^storage::lines<^^source::scheme::addons>}, = tag::Writes{^^storage::image},
  = tag::Writes{^^storage::suffixBase}
]] void storeAddons();
[[ = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::uniques},
   = tag::Writes{^^storage::image} ]] void storeDictionaryWords();
[[
  = tag::Reads{^^storage::addonRows}, = tag::Reads{^^storage::addons},
  = tag::Reads{^^storage::lines<^^source::scheme::addons>}, = tag::Writes{^^storage::packons}
]] void numberPackons();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::packons}, = tag::Writes{^^storage::classGrammars}, = tag::Writes{^^storage::image}
]] void buildDictionaryClasses();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::kept}, = tag::Reads{^^storage::starts},
  = tag::Reads{^^storage::total},
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::lines<^^source::scheme::inflects>}, = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Reads{^^storage::records<^^source::scheme::dictline>},
  = tag::Reads{^^storage::records<^^source::scheme::inflects>},
  = tag::Reads{^^storage::records<^^source::scheme::uniques>}, = tag::Writes{^^storage::image},
  = tag::Writes{^^storage::descriptionIds}, = tag::Writes{^^storage::source}
]] void collectReadings();
[[
  = tag::Reads{^^storage::kept}, = tag::Reads{^^storage::starts}, = tag::Reads{^^storage::total},
  = tag::Reads{^^storage::lines<^^source::scheme::uniques>}, = tag::Writes{^^storage::source},
  = tag::Writes{^^storage::image}
]] void groupSpellings();
[[
  = tag::Reads{^^storage::inflects}, = tag::Reads{^^storage::lines<^^source::scheme::inflects>},
  = tag::Writes{^^storage::endingOrder}, = tag::Writes{^^storage::image}
]] void orderInflections();
[[
  = tag::Reads{^^storage::classGrammars}, = tag::Reads{^^storage::endingOrder}, = tag::Reads{^^storage::inflects},
  = tag::Writes{^^storage::image}, = tag::Writes{^^storage::descriptionIds}
]] void licenseDictionaryClasses();
[[
  = tag::Reads{^^storage::addonRows}, = tag::Reads{^^storage::addons}, = tag::Reads{^^storage::endingOrder},
  = tag::Reads{^^storage::suffixBase}, = tag::Reads{^^storage::inflects}, = tag::Writes{^^storage::image},
  = tag::Writes{^^storage::descriptionIds}
]] void licenseSuffixes();
} // namespace image_data

namespace lookup_tables {
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::inflects},
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::lines<^^source::scheme::inflects>}, = tag::Writes{^^storage::byStem},
  = tag::Writes{^^storage::candidateCounts}, = tag::Writes{^^storage::mixed},
  = tag::Writes{^^storage::compressionDuplicates}, = tag::Writes{^^storage::rows}, = tag::Writes{^^storage::rules}
]] void makeLookupRules();
[[
  = tag::Reads{^^storage::uniques}, = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Writes{^^storage::candidateCounts}, = tag::Writes{^^storage::byStem}, = tag::Writes{^^storage::rows},
  = tag::Writes{^^storage::rules}
]] void addUniqueRules();
[[ = tag::Reads{^^storage::byStem}, = tag::Reads{^^storage::rows},
   = tag::Writes{^^storage::perfectSuffixes} ]] void collectPerfectSuffixes();
[[ = tag::Writes{^^storage::candidateCapacity},
   = tag::Writes{^^storage::candidateCounts} ]] void measureCandidateCapacity();
[[
  = tag::Reads{^^storage::byStem}, = tag::Reads{^^storage::rows}, = tag::Reads{^^storage::dictline},
  = tag::Reads{^^storage::inflects}, = tag::Reads{^^storage::uniques},
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>}, = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Writes{^^storage::classes}, = tag::Writes{^^storage::stems}
]] void buildLookupClasses();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Writes{^^storage::stems}
]] void addAffixStems();
[[= tag::Writes{^^storage::stems}]] void sortLookupStems();
[[ = tag::Reads{^^storage::rows}, = tag::Writes{^^storage::order},
   = tag::Writes{^^storage::arrayOrder} ]] void rankLookupRules();
[[
  = tag::Reads{^^storage::stems}, = tag::Reads{^^storage::classes}, = tag::Reads{^^storage::rows},
  = tag::Writes{^^storage::stemRuns}, = tag::Writes{^^storage::endingRuns}
]] void groupStemSpellings();
[[
  = tag::Writes{^^storage::endingRuns}, = tag::Writes{^^storage::endings}, = tag::Writes{^^storage::endingIds},
  = tag::Writes{^^storage::reversed}
]] void numberEndings();
[[ = tag::Reads{^^storage::stemRuns}, = tag::Writes{^^storage::front} ]] void buildStemTrie();
[[ = tag::Reads{^^storage::reversed}, = tag::Writes{^^storage::back} ]] void buildEndingTrie();
[[
  = tag::Reads{^^storage::classes}, = tag::Reads{^^storage::stems}, = tag::Reads{^^storage::endingIds},
  = tag::Reads{^^storage::endings}, = tag::Reads{^^storage::order}, = tag::Reads{^^storage::rows},
  = tag::Writes{^^storage::cells}, = tag::Writes{^^storage::cellOf}, = tag::Writes{^^storage::rowIds},
  = tag::Writes{^^storage::maxRows}
]] void buildLookupCells();
[[
  = tag::Reads{^^storage::front}, = tag::Reads{^^storage::back}, = tag::Reads{^^storage::maxRows},
  = tag::Reads{^^storage::cells}, = tag::Reads{^^storage::rowIds}, = tag::Reads{^^storage::rows},
  = tag::Reads{^^storage::classes}, = tag::Reads{^^storage::mixed}, = tag::Reads{^^storage::compressionDuplicates},
  = tag::Writes{^^storage::out}
]] void deriveLookupBounds();
[[
  = tag::Reads{^^storage::perfectSuffixes}, = tag::Reads{^^storage::candidateCapacity},
  = tag::Reads{^^storage::endings}, = tag::Writes{^^storage::out}
]] void buildPerfectSuffixTrie();
[[
  = tag::Reads{^^storage::front}, = tag::Writes{^^storage::states}, = tag::Writes{^^storage::below},
  = tag::Writes{^^storage::root}
]] void canonicalizeStemTrie();
[[ = tag::Reads{^^storage::front}, = tag::Reads{^^storage::stems},
   = tag::Writes{^^storage::result} ]] void numberStemRuns();
[[
  = tag::Reads{^^storage::states}, = tag::Reads{^^storage::below}, = tag::Reads{^^storage::root},
  = tag::Writes{^^storage::result}
]] void packSharedStemSlots();
[[ = tag::Reads{^^storage::result}, = tag::Reads{^^storage::back},
   = tag::Writes{^^storage::out} ]] void buildLookupHeads();
[[
  = tag::Reads{^^storage::back}, = tag::Reads{^^storage::stems}, = tag::Writes{^^storage::result},
  = tag::Writes{^^storage::out}
]] void packLookupStems();
[[
  = tag::Reads{^^storage::rows}, = tag::Reads{^^storage::order}, = tag::Reads{^^storage::arrayOrder},
  = tag::Writes{^^storage::result}, = tag::Writes{^^storage::out}
]] void packLookupRules();
[[
  = tag::Reads{^^storage::cells}, = tag::Reads{^^storage::classes}, = tag::Reads{^^storage::endings},
  = tag::Reads{^^storage::cellOf}, = tag::Reads{^^storage::rowIds}, = tag::Reads{^^storage::out},
  = tag::Writes{^^storage::result}
]] void compressLookupCells();
} // namespace lookup_tables

namespace finish_image_data {
[[ = tag::Reads{^^storage::result}, = tag::Writes{^^storage::image} ]] void attachLookupTables();
[[= tag::Writes{^^storage::image}]] void copyLookupStems();
[[= tag::Writes{^^storage::image}]] void groupDictionaryStems();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::uniques},
  = tag::Reads{^^storage::lines<^^source::scheme::dictline>}, = tag::Reads{^^storage::lines<^^source::scheme::uniques>},
  = tag::Writes{^^storage::image}
]] void markStemLetters();
[[ = tag::Writes{^^storage::image}, = tag::Writes{^^storage::descriptionIds} ]] void assignRuleDescriptions();
[[= tag::Writes{^^storage::image}]] void assignRuleLabels();
[[= tag::Writes{^^storage::image}]] void buildAffixTables();
} // namespace finish_image_data

namespace encoding {
[[ = tag::Reads{^^storage::image}, = tag::Writes{^^storage::encodedImage} ]] void encodeImage();
} // namespace encoding

namespace image_file {
[[
  = tag::Reads{^^storage::destination}, = tag::Reads{^^storage::encodedImage}, = tag::Writes{^^storage::temporaryImage}
]] void writeTemporaryImage();
[[
  = tag::Reads{^^storage::temporaryImage}, = tag::Writes{^^storage::loadedImage},
  = tag::Writes{^^whitaker::relationship::sImage}
]] void loadTemporaryImage();
[[
  = tag::Reads{^^storage::loadedImage}, = tag::Reads{^^whitaker::relationship::sImage},
  = tag::Writes{^^storage::imageSpellings}
]] void enumerateImageSpellings();
[[
  = tag::Reads{^^storage::imageSpellings}, = tag::Reads{^^storage::kept}, = tag::Reads{^^storage::starts},
  = tag::Reads{^^storage::total}, = tag::Reads{^^storage::lines<^^source::scheme::dictline>},
  = tag::Reads{^^storage::lines<^^source::scheme::uniques>}, = tag::Reads{^^storage::loadedImage},
  = tag::Reads{^^whitaker::relationship::sImage}
]] void checkImageReadings();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::uniques}, = tag::Reads{^^storage::loadedImage},
  = tag::Reads{^^whitaker::relationship::sImage}
]] void checkDictionaryWords();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::uniques}, = tag::Reads{^^storage::loadedImage},
  = tag::Reads{^^whitaker::relationship::sImage}
]] void checkDictionaryWordStems();
[[
  = tag::Reads{^^storage::dictline}, = tag::Reads{^^storage::uniques}, = tag::Reads{^^storage::loadedImage},
  = tag::Reads{^^whitaker::relationship::sImage}
]] void checkDictionaryForms();
[[ = tag::Reads{^^storage::loadedImage}, = tag::Reads{^^whitaker::relationship::sImage} ]] void checkAddonTables();
// INFO: publishImage replaces the old image, so it waits for every check by name.
[[
  = tag::Gate{^^image_file::checkImageReadings}, = tag::Gate{^^image_file::checkDictionaryWords},
  = tag::Gate{^^image_file::checkDictionaryWordStems}, = tag::Gate{^^image_file::checkAddonTables},
  = tag::Gate{^^image_file::checkDictionaryForms},
  = tag::Reads{^^storage::destination}, = tag::Reads{^^storage::temporaryImage}
]] void publishImage();
} // namespace image_file

} // namespace execution_steps
