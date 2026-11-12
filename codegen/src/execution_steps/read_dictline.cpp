#include "execution_steps/steps.hpp"

#include "helpers/read.hpp"

void execution_steps::dictline::readDictline() { helpers::read<^^source::scheme::dictline>(); }
