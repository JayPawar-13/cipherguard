/* =========================================================
   CipherGuard — app.js
   All session data lives in the `session` object below only.
   Nothing is written to localStorage/sessionStorage/cookies,
   so refreshing the page fully resets the flow, as required.
   ========================================================= */

let Module = null;

// ---- In-memory session state (resets on page refresh) ----
const session = {
  loginId: null,   // the email/phone used to log in
  idType: null,    // "email" or "phone"
};

// ---- Boot the WASM engine ----
const statusDot = document.getElementById("statusDot");
const statusText = document.getElementById("statusText");

if (typeof AuthModule === "function") {
  AuthModule().then((m) => {
    Module = m;
    statusDot.classList.add("ready");
    statusText.textContent = "engine ready";
  }).catch((err) => {
    statusText.textContent = "engine failed to load";
    console.error(err);
  });
} else {
  statusText.textContent = "auth.js not found — compile with emcc first";
}

// ---- Helpers ----
function detectIdType(value) {
  const email = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  const phone = /^\d{10}$/;
  if (email.test(value)) return "email";
  if (phone.test(value)) return "phone";
  return null;
}

function requireEngine() {
  if (!Module) {
    alert("The security engine hasn't finished loading yet. Give it a second and try again.");
    return false;
  }
  return true;
}

function showScreen(id) {
  document.querySelectorAll(".screen").forEach((s) => s.classList.remove("active"));
  document.getElementById(id).classList.add("active");
}

// =========================================================
// SCREEN 1 — LOGIN
// =========================================================
const loginIdInput = document.getElementById("loginId");
const idHint = document.getElementById("idHint");
const passwordInput = document.getElementById("password");
const strengthFill = document.getElementById("strengthFill");
const strengthLabelEl = document.getElementById("strengthLabel");
const strengthScoreEl = document.getElementById("strengthScore");
const feedbackList = document.getElementById("feedbackList");
const btnCheck = document.getElementById("btnCheck");
const btnGenerate = document.getElementById("btnGenerate");
const btnLogin = document.getElementById("btnLogin");
const loginNote = document.getElementById("loginNote");

let lastScore = 0;

function colorForScore(score) {
  if (score >= 80) return "var(--green)";
  if (score >= 60) return "var(--cyan)";
  if (score >= 40) return "var(--amber)";
  return "var(--red)";
}

function refreshStrength() {
  if (!Module) return;
  const pwd = passwordInput.value;
  if (!pwd) {
    strengthFill.style.width = "0%";
    strengthLabelEl.textContent = "—";
    strengthScoreEl.textContent = "0/100";
    feedbackList.innerHTML = "";
    lastScore = 0;
    updateLoginButton();
    return;
  }
  const raw = Module.checkPassword(pwd);       // "score|label|suggestion1|suggestion2..."
  const parts = raw.split("|");
  const score = parseInt(parts[0], 10);
  const label = parts[1];
  const suggestions = parts.slice(2);

  lastScore = score;
  strengthFill.style.width = score + "%";
  strengthFill.style.background = colorForScore(score);
  strengthLabelEl.textContent = label;
  strengthLabelEl.style.color = colorForScore(score);
  strengthScoreEl.textContent = score + "/100";

  feedbackList.innerHTML = "";
  if (suggestions.length === 0 || (suggestions.length === 1 && suggestions[0] === "")) {
    const li = document.createElement("li");
    li.textContent = "No issues found — nice password.";
    feedbackList.appendChild(li);
  } else {
    suggestions.forEach((s) => {
      if (!s) return;
      const li = document.createElement("li");
      li.textContent = s;
      feedbackList.appendChild(li);
    });
  }
  updateLoginButton();
}

function updateLoginButton() {
  const idType = detectIdType(loginIdInput.value.trim());
  const idOk = idType !== null;
  idHint.textContent = loginIdInput.value.length === 0
    ? ""
    : idOk
      ? `Recognized as ${idType}`
      : "Enter a valid email or a 10-digit phone number";
  idHint.classList.toggle("ok", idOk);

  const canLogin = idOk && lastScore > 50;
  btnLogin.disabled = !canLogin;
  loginNote.textContent = canLogin
    ? "Ready to continue."
    : "Password strength must be above 50% and the ID must be valid to continue.";
}

passwordInput.addEventListener("input", refreshStrength);
loginIdInput.addEventListener("input", updateLoginButton);

btnCheck.addEventListener("click", () => {
  if (!requireEngine()) return;
  refreshStrength();
});

btnGenerate.addEventListener("click", () => {
  if (!requireEngine()) return;
  const pwd = Module.generatePassword(16, true, true, true);
  passwordInput.value = pwd;
  refreshStrength();
});

btnLogin.addEventListener("click", () => {
  const idValue = loginIdInput.value.trim();
  const idType = detectIdType(idValue);
  if (!idType || lastScore <= 50) return;

  session.loginId = idValue;
  session.idType = idType;

  document.getElementById("confirmId").value = "";
  document.getElementById("confirmHint").textContent = "";
  document.getElementById("totpPanel").hidden = true;
  document.getElementById("otpInput").value = "";
  document.getElementById("verifyError").textContent = "";

  showScreen("totpScreen");
});

// =========================================================
// SCREEN 2 — TOTP VERIFICATION
// =========================================================
const confirmIdInput = document.getElementById("confirmId");
const confirmHint = document.getElementById("confirmHint");
const btnSendCode = document.getElementById("btnSendCode");
const totpPanel = document.getElementById("totpPanel");
const deliveryNote = document.getElementById("deliveryNote");
const ringProgress = document.getElementById("ringProgress");
const ringSeconds = document.getElementById("ringSeconds");
const otpInput = document.getElementById("otpInput");
const btnVerify = document.getElementById("btnVerify");
const verifyError = document.getElementById("verifyError");

const RING_CIRCUMFERENCE = 2 * Math.PI * 52; // r=52, matches SVG
let countdownTimer = null;
let currentCode = null;

function generateAndShowCode() {
  if (!requireEngine()) return;
  currentCode = Module.generateTOTP(session.loginId);

  const icon = session.idType === "email" ? "📧" : "📱";
  const destination = session.idType === "email"
    ? session.loginId
    : "+91-" + session.loginId;

  deliveryNote.innerHTML =
    `${icon} Demo code sent to <strong>${destination}</strong>: <span class="code">${currentCode}</span>`;

  totpPanel.hidden = false;
  startCountdown();
}

function startCountdown() {
  if (countdownTimer) clearInterval(countdownTimer);

  function tick() {
    const secondsLeft = Module.secondsLeft();
    ringSeconds.textContent = secondsLeft;
    const offset = RING_CIRCUMFERENCE * (1 - secondsLeft / 30);
    ringProgress.style.strokeDashoffset = offset;
    ringProgress.style.stroke = secondsLeft <= 5 ? "var(--red)" : "var(--cyan)";

    // Code rolls over to a new one at the start of each 30s window —
    // refresh the displayed (demo) code so it always matches what
    // verification will check against.
    const freshCode = Module.generateTOTP(session.loginId);
    if (freshCode !== currentCode) {
      currentCode = freshCode;
      const icon = session.idType === "email" ? "📧" : "📱";
      const destination = session.idType === "email"
        ? session.loginId
        : "+91-" + session.loginId;
      deliveryNote.innerHTML =
        `${icon} Demo code sent to <strong>${destination}</strong>: <span class="code">${currentCode}</span>`;
    }
  }

  tick();
  countdownTimer = setInterval(tick, 1000);
}

btnSendCode.addEventListener("click", () => {
  const value = confirmIdInput.value.trim();
  if (!value) {
    confirmHint.textContent = "Enter your login ID.";
    confirmHint.classList.remove("ok");
    return;
  }
  if (value !== session.loginId) {
    confirmHint.textContent = "This ID doesn't match your recent login.";
    confirmHint.classList.remove("ok");
    totpPanel.hidden = true;
    if (countdownTimer) clearInterval(countdownTimer);
    return;
  }
  confirmHint.textContent = "Match confirmed.";
  confirmHint.classList.add("ok");
  generateAndShowCode();
});

btnVerify.addEventListener("click", () => {
  if (!requireEngine()) return;
  const entered = otpInput.value.trim();
  const expected = Module.generateTOTP(session.loginId); // recompute fresh, matches real TOTP verification style

  if (entered.length !== 6) {
    verifyError.textContent = "Enter the full 6-digit code.";
    return;
  }
  if (entered !== expected) {
    verifyError.textContent = "Invalid or expired code — try again.";
    return;
  }

  verifyError.textContent = "";
  if (countdownTimer) clearInterval(countdownTimer);
  document.getElementById("successId").textContent = session.loginId;
  showScreen("successScreen");
});

// =========================================================
// SCREEN 3 — RESTART
// =========================================================
document.getElementById("btnRestart").addEventListener("click", () => {
  session.loginId = null;
  session.idType = null;
  loginIdInput.value = "";
  passwordInput.value = "";
  refreshStrength();
  updateLoginButton();
  showScreen("loginScreen");
});
