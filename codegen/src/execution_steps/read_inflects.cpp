#include "execution_steps/steps.hpp"

#include "helpers/read.hpp"

void execution_steps::inflects::readInflects() { helpers::read<^^source::scheme::inflects>(); }
