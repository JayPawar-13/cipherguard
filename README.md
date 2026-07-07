# 🔐 CipherGuard

<p align="center">

**A Secure Authentication Toolkit built with C++17, WebAssembly (WASM), HTML, CSS, and JavaScript.**

A modern authentication demo featuring password strength analysis, secure password generation, and Time-based One-Time Password (TOTP) generation, accessible through both a command-line interface and a browser powered by WebAssembly.

</p>

<p align="center">

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![WebAssembly](https://img.shields.io/badge/WebAssembly-WASM-purple.svg)
![HTML5](https://img.shields.io/badge/HTML-5-orange.svg)
![CSS3](https://img.shields.io/badge/CSS-3-blue.svg)
![JavaScript](https://img.shields.io/badge/JavaScript-ES6-yellow.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

</p>

---

## 📖 Project Overview

CipherGuard is a lightweight authentication toolkit developed as a demonstration of secure authentication concepts using modern C++ and WebAssembly.

The project combines native C++ performance with an interactive browser interface by compiling C++ code into WebAssembly using **Emscripten**. This allows the same authentication logic to be reused in both the terminal application and the web application.

The toolkit currently provides:

- 🔐 Password Strength Analysis
- 🎲 Secure Password Generation
- 🔢 Time-Based One-Time Password (TOTP) Generation
- 🌐 Browser-based interface powered by WebAssembly
- 💻 Cross-platform Command-Line Interface (CLI)

---

## ✨ Features

### 🔐 Password Strength Checker

- Checks password length
- Detects uppercase and lowercase characters
- Detects numbers
- Detects special characters
- Identifies commonly used passwords
- Calculates a password strength score (0–100)
- Provides suggestions to improve weak passwords

---

### 🎲 Password Generator

Generate secure passwords with configurable options:

- Uppercase letters
- Lowercase letters
- Numbers
- Special characters
- Custom password length

---

### 🔢 TOTP Generator

Generate Time-based One-Time Passwords using:

- SHA-1
- HMAC-SHA1
- RFC 6238 algorithm
- 30-second rotating authentication codes

---

### 🌐 WebAssembly Integration

The browser application reuses the same C++ authentication logic by compiling it into WebAssembly using **Emscripten**.

This demonstrates:

- Native C++ performance
- Code reuse between CLI and Web
- Modern browser compatibility

---

## 🚀 Technologies Used

| Technology | Purpose |
|------------|---------|
| C++17 | Core authentication logic |
| STL | Data structures and algorithms |
| WebAssembly (WASM) | Browser execution |
| Emscripten | C++ → WASM compiler |
| HTML5 | User Interface |
| CSS3 | Styling |
| JavaScript | Browser interaction |
| Git | Version Control |
| GitHub | Repository Hosting |
