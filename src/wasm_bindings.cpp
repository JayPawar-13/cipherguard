/*
 * wasm_bindings.cpp — CipherGuard WebAssembly bridge
 * ----------------------------------------------------
 * Exposes the exact same C++ logic from auth_core.h to JavaScript
 * using Emscripten's embind. Compile with:
 *
 *   emcc src/wasm_bindings.cpp -o web/auth.js -s WASM=1 -lembind \
 *        -s MODULARIZE=1 -s EXPORT_NAME="AuthModule"
 *
 * This produces web/auth.js and web/auth.wasm, which index.html loads.
 */

#include "auth_core.h"
#include <emscripten/bind.h>
using namespace emscripten;

// Returns a pipe-delimited string: "score|label|suggestion1|suggestion2|..."
// (kept as a plain string instead of a JS object/array to avoid extra
// embind container bindings — simplest possible bridge)
std::string checkPasswordJS(std::string pwd) {
    std::vector<std::string> feedback;
    int score = scorePassword(pwd, feedback);
    std::string result = std::to_string(score) + "|" + strengthLabel(score);
    for (auto& f : feedback) result += "|" + f;
    return result;
}

std::string generatePasswordJS(int length, bool upper, bool digits, bool special) {
    return generatePassword(length, upper, digits, special);
}

std::string generateTOTPJS(std::string secret) {
    uint32_t code = generateTOTP(secret);
    char buf[8];
    snprintf(buf, sizeof(buf), "%06u", code);
    return std::string(buf);
}

int secondsLeftJS() {
    return secondsRemainingInStep();
}

EMSCRIPTEN_BINDINGS(cipherguard_module) {
    function("checkPassword", &checkPasswordJS);
    function("generatePassword", &generatePasswordJS);
    function("generateTOTP", &generateTOTPJS);
    function("secondsLeft", &secondsLeftJS);
}
