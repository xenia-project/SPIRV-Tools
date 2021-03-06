// Copyright (c) 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pass_manager.h"

#include <iostream>
#include <vector>

#include "ir_context.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {

namespace opt {

Pass::Status PassManager::Run(ir::IRContext* context) {
  auto status = Pass::Status::SuccessWithoutChange;

  // If print_all_stream_ is not null, prints the disassembly of the module
  // to that stream, with the given preamble and optionally the pass name.
  auto print_disassembly = [&context, this](const char* preamble, Pass* pass) {
    if (print_all_stream_) {
      std::vector<uint32_t> binary;
      context->module()->ToBinary(&binary, false);
      SpirvTools t(SPV_ENV_UNIVERSAL_1_2);
      std::string disassembly;
      t.Disassemble(binary, &disassembly, 0);
      *print_all_stream_ << preamble << (pass ? pass->name() : "") << "\n"
                         << disassembly << std::endl;
    }
  };

  for (const auto& pass : passes_) {
    print_disassembly("; IR before pass ", pass.get());
    const auto one_status = pass->Run(context);
    if (one_status == Pass::Status::Failure) return one_status;
    if (one_status == Pass::Status::SuccessWithChange) status = one_status;
  }
  print_disassembly("; IR after last pass", nullptr);

  // Set the Id bound in the header in case a pass forgot to do so.
  //
  // TODO(dnovillo): This should be unnecessary and automatically maintained by
  // the IRContext.
  if (status == Pass::Status::SuccessWithChange) {
    context->module()->SetIdBound(context->module()->ComputeIdBound());
  }
  passes_.clear();
  return status;
}

}  // namespace opt
}  // namespace spvtools
