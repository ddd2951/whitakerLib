// INFO: Gen: every step of execution_steps/steps.hpp, in the order its gates allow (execution_steps/run.hpp).
#include "execution_steps/run.hpp"

int main() {
  runner::runParallel<^^execution_steps>();
  return 0;
}
