#include "word.hpp"

#include "execution_steps/steps.hpp"
#include "storage/addons.hpp"
#include "storage/dictline.hpp"
#include "storage/inflects.hpp"
#include "storage/uniques.hpp"

namespace {

using storage::addons;
using storage::dictline;
using storage::inflects;
using storage::uniques;

} // namespace

const word::DictlineEntry& word::entry(const DictlineRow row) { return dictline[row.value]; }
const word::InflectsEntry& word::entry(const InflectsRow row) { return inflects[row.value]; }
const word::UniquesEntry& word::entry(const UniquesRow row) { return uniques[row.value]; }
const word::AddonsEntry& word::entry(const source::AddonsIndex row) { return addons[row.value]; }
