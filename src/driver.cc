#include <cstdio>

#include "codegen.hpp"
#include "driver.hpp"

int main() {
  yy::Driver driver{};

  InitializeTargets();
  CreateJIT();
  InitializeModuleAndManagers();

  while (true) {
    fprintf(stderr, "ready> ");
    auto p = driver.parse();
    if (p == 2)
      break;
    if (!p)
      fprintf(stderr, "cannot parse\n");
  }
}
