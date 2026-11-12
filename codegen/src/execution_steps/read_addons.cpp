#include "execution_steps/steps.hpp"

#include "helpers/read.hpp"

void execution_steps::addons::readAddons() { helpers::read<^^source::scheme::addons>(); }
