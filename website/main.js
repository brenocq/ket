// SPDX-License-Identifier: MIT
// Progressive enhancement only — the page is fully functional without this.
(() => {
  "use strict";
  const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

  // Reduced motion: hold the demo video on its poster frame instead of looping.
  if (reduce) {
    for (const v of document.querySelectorAll("video")) {
      v.removeAttribute("autoplay");
      v.pause();
    }
  }

  // Cursor spotlight: feed the mouse position to each card's radial highlight.
  if (!reduce) {
    for (const card of document.querySelectorAll(".card")) {
      card.addEventListener("pointermove", (e) => {
        const r = card.getBoundingClientRect();
        card.style.setProperty("--mx", `${e.clientX - r.left}px`);
        card.style.setProperty("--my", `${e.clientY - r.top}px`);
      });
    }
  }

  // Copy button: copy the currently visible code panel.
  const copyBtn = document.querySelector(".copy");
  if (copyBtn) {
    copyBtn.addEventListener("click", async () => {
      const panel = document.querySelector(".code .panel:not([style*='none'])");
      const visible = [...document.querySelectorAll(".code .panel")].find(
        (p) => getComputedStyle(p).display !== "none",
      );
      const text = (visible || panel)?.innerText ?? "";
      try {
        await navigator.clipboard.writeText(text);
        const old = copyBtn.textContent;
        copyBtn.textContent = "Copied";
        setTimeout(() => (copyBtn.textContent = old), 1200);
      } catch {
        /* clipboard blocked — ignore */
      }
    });
  }

  // Install command: copy the command text (without the prompt).
  const installBtn = document.querySelector(".install .copy2");
  if (installBtn) {
    installBtn.addEventListener("click", async () => {
      const cmd = document.querySelector(".install .cmd")?.innerText ?? "";
      try {
        await navigator.clipboard.writeText(cmd.trim());
        installBtn.textContent = "Copied";
        setTimeout(() => (installBtn.textContent = "Copy"), 1200);
      } catch {
        /* ignore */
      }
    });
  }

  // Scroll reveal: add .in when an element scrolls into view (once).
  const reveals = document.querySelectorAll(".reveal");
  if (reduce || !("IntersectionObserver" in window)) {
    reveals.forEach((el) => el.classList.add("in"));
  } else {
    const io = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) {
          if (entry.isIntersecting) {
            entry.target.classList.add("in");
            io.unobserve(entry.target);
          }
        }
      },
      { threshold: 0.12 },
    );
    reveals.forEach((el) => io.observe(el));
  }
})();
