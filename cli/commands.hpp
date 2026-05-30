// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Breno Cunha Queiroz
#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace ket::cli {

// Each command receives the argument slice following the command word, plus the
// streams to use (injectable so the commands are unit-testable). `in` is the
// input source used when no file argument is given (or it is "-"). Returns a
// process exit code: 0 ok, 1 usage error, 2 runtime error.
int cmd_draw(const std::vector<std::string>& args, std::istream& in,
             std::ostream& out, std::ostream& err);
int cmd_run(const std::vector<std::string>& args, std::istream& in,
            std::ostream& out, std::ostream& err);
int cmd_sample(const std::vector<std::string>& args, std::istream& in,
               std::ostream& out, std::ostream& err);

void print_version(std::ostream& out);
void print_usage(std::ostream& out);

}  // namespace ket::cli
