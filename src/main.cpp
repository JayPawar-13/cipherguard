/*
 * main.cpp — CipherGuard CLI
 * Run this with: g++ -std=c++17 -Wall -o cipherguard src/main.cpp && ./cipherguard
 */

#include "auth_core.h"

void printFeedback(const std::vector<std::string>& feedback) {
    if (feedback.empty()) {
        std::cout << "  No issues found. Great password!\n";
        return;
    }
    std::cout << "  Suggestions:\n";
    for (const auto& f : feedback) std::cout << "   - " << f << "\n";
}

int main() {
    std::string lastSecret;

    while (true) {
        std::cout << "\n===== CipherGuard =====\n";
        std::cout << "1. Check password strength\n";
        std::cout << "2. Generate a strong password\n";
        std::cout << "3. Generate a TOTP code from a secret\n";
        std::cout << "4. Exit\n";
        std::cout << "Choose an option: ";

        int choice;
        if (!(std::cin >> choice)) break;
        std::cin.ignore();

        if (choice == 1) {
            std::cout << "Enter password to check: ";
            std::string pwd;
            std::getline(std::cin, pwd);
            std::vector<std::string> feedback;
            int score = scorePassword(pwd, feedback);
            std::cout << "Score: " << score << "/100  ->  " << strengthLabel(score) << "\n";
            printFeedback(feedback);

        } else if (choice == 2) {
            int length; std::cout << "Password length (e.g. 16): "; std::cin >> length;
            std::string pwd = generatePassword(length, true, true, true);
            std::cout << "Generated password: " << pwd << "\n";
            lastSecret = pwd;

        } else if (choice == 3) {
            std::string secret;
            std::cout << "Enter a secret key (or press Enter to reuse last generated password): ";
            std::getline(std::cin, secret);
            if (secret.empty()) {
                if (lastSecret.empty()) {
                    std::cout << "No secret available. Generate a password first or type a secret.\n";
                    continue;
                }
                secret = lastSecret;
            }
            uint32_t code = generateTOTP(secret);
            printf("TOTP Code: %06u\n", code);
            std::cout << "Valid for " << secondsRemainingInStep() << " more seconds\n";

        } else if (choice == 4) {
            std::cout << "Goodbye!\n";
            break;
        } else {
            std::cout << "Invalid option, try again.\n";
        }
    }
    return 0;
}
