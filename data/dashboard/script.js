/* Wattrack dashboard script in Pico CSS 
     1. Dark/light theme toggle (Pico data-theme, persisted)
     2. Timeframe switcher
     3. SVG chart tooltip
     4. Pulse constant stepper & presets
     5. Utility rate stepper & cost calculations
     6. First-time user onboarding popup (Pico <dialog>)
     7. Baseline utility reading form + storage hooks
     8. Sync button
*/

document.addEventListener('DOMContentLoaded', () => {
  // ==========================================
  // 1. Theme toggle (Pico uses data-theme on <html>)
  // ==========================================
  var htmlElem = document.documentElement;
  var themeBtn = document.getElementById('themeToggleBtn');
  var themeText = document.getElementById('themeToggleText');
  var THEME_KEY = 'wattrack-theme';

  function updateThemeUI(isDark) {
    htmlElem.setAttribute('data-theme', isDark ? 'dark' : 'light');
    if (themeText) themeText.textContent = isDark ? 'Dark' : 'Light';
  }

  updateThemeUI(htmlElem.getAttribute('data-theme') !== 'light');
  // Restore the user's saved preference, if any
  try {
    var savedTheme = localStorage.getItem(THEME_KEY);
    if (savedTheme) updateThemeUI(savedTheme === 'dark');
  } catch (e) { }

  if (themeBtn) {
    themeBtn.addEventListener('click', () => {
      var isDark = htmlElem.getAttribute('data-theme') !== 'light';
      var next = !isDark;
      updateThemeUI(next);
      try { localStorage.setItem(THEME_KEY, next ? 'dark' : 'light'); } catch (e) { }
    });
  }

  // ==========================================
  // 2. Timeframe switcher
  // ==========================================
  var timeframeButtons = document.querySelectorAll('.timeframe-btn');
  var usedTitle = document.getElementById('usedMetricTitle');
  var usedValue = document.getElementById('usedValue');
  var usedSub = document.getElementById('usedSub');
  var dailyAvg = document.getElementById('dailyAvg');
  var projValue = document.getElementById('projValue');
  var chartMainTitle = document.getElementById('chartMainTitle');
  var chartBadge = document.getElementById('chartBadge');

  var dataByTimeframe = {
    today: {
      title: 'Used Today (Live)',
      value: '16.8',
      sub: 'Updated 10s ago • 16.8 kWh since midnight',
      avg: '1.40 kWh / hour',
      proj: '₱27.60 (Today)',
      chartTitle: "Today's Hourly Energy & Live Pulses",
      badge: 'Today, Oct 24'
    },
    week: {
      title: 'Used This Week',
      value: '118',
      sub: 'Mon - Thu • 4 days recorded',
      avg: '29.5 kWh / day',
      proj: '₱204.60 (Week)',
      chartTitle: 'Weekly Daily Load Profile & Heatmap',
      badge: 'Week 43, 2024'
    },
    month: {
      title: 'Used This Month',
      value: '285',
      sub: 'Day 18 of 30 in lease billing cycle',
      avg: '15.8 kWh / day',
      proj: '₱789.30',
      chartTitle: 'Daily Energy Usage & Heatmap Spend Profile',
      badge: 'October 2024'
    },
    billing: {
      title: 'Current Billing Cycle',
      value: '285',
      sub: 'Cycle Ends Nov 1 • 12 days remaining',
      avg: '15.8 kWh / day',
      proj: '₱789.30 EST',
      chartTitle: 'Cycle Energy Usage & Month-End Forecast',
      badge: 'Cycle #10 - Oct/Nov'
    }
  };

  function setTimeframeActive(btn) {
    timeframeButtons.forEach((b) => b.classList.remove('active'));
    btn.classList.add('active');
  }

  timeframeButtons.forEach((btn) => {
    btn.addEventListener('click', () => {
      setTimeframeActive(btn);
      var d = dataByTimeframe[btn.dataset.range];
      if (d) {
        usedTitle.textContent = d.title;
        usedValue.textContent = d.value;
        usedSub.textContent = d.sub;
        dailyAvg.textContent = d.avg;
        projValue.textContent = d.proj;
        chartMainTitle.textContent = d.chartTitle;
        chartBadge.textContent = d.badge;
      }
    });
  });

  // ==========================================
  // 3. SVG chart tooltip
  // ==========================================
  var tooltip = document.getElementById('chartTooltip');
  var tooltipDate = document.getElementById('tooltipDate');
  var tooltipBadge = document.getElementById('tooltipBadge');
  var tooltipKwhValue = document.getElementById('tooltipKwhValue');
  var tooltipCost = document.getElementById('tooltipCost');
  var tooltipImpulses = document.getElementById('tooltipImpulses');
  var interactiveElements = document.querySelectorAll('.chart-bar, .chart-point');

  interactiveElements.forEach((el) => {
    el.addEventListener('mouseenter', () => {
      if (!tooltip) return;
      var kwh = el.dataset.kwh || '16.8 kWh';
      var kwhNum = parseFloat(kwh);
      var pulses = isNaN(kwhNum) ? 16800 : Math.round(kwhNum * 1000);

      tooltipDate.textContent = el.dataset.date || 'Recorded Day';
      tooltipBadge.textContent = el.dataset.badge || (el.dataset.live ? 'LIVE' : 'RECORDED');
      tooltipKwhValue.textContent = kwh;
      tooltipCost.textContent = el.dataset.cost || '₱2.40';
      tooltipImpulses.textContent = pulses.toLocaleString() + ' imp';

      var x = 0;
      var y = 0;
      var target = el.tagName.toLowerCase() === 'circle' ? el : el.querySelector('rect');
      if (target) {
        x = parseFloat(target.getAttribute('cx') || target.getAttribute('x')) +
          parseFloat(target.getAttribute('r') ? target.getAttribute('r') : (parseFloat(target.getAttribute('width') || 0) / 2));
        y = parseFloat(target.getAttribute('cy') || target.getAttribute('y'));
      }

      var leftPos = Math.max(10, Math.min(x - 90, 700));
      var topPos = Math.max(10, y - 90);
      tooltip.style.left = leftPos + 'px';
      tooltip.style.top = topPos + 'px';
      tooltip.hidden = false;
    });
  });

  var chartWrapper = document.getElementById('chartWrapper');
  if (chartWrapper && tooltip) {
    chartWrapper.addEventListener('mouseleave', () => { tooltip.hidden = true; });
  }

  // ==========================================
  // 4. Pulse constant stepper & presets
  // ==========================================
  var pulseInput = document.getElementById('pulseConstantInput');
  var decBtn = document.getElementById('decPulseBtn');
  var incBtn = document.getElementById('incPulseBtn');
  var presetBtns = document.querySelectorAll('.preset-btn');
  var expPulseRate = document.getElementById('expPulseRate');
  var expWhPerPulse = document.getElementById('expWhPerPulse');
  var pulseWhSummary = document.getElementById('pulseWhSummary');
  var pulseRateDisplay = document.getElementById('pulseRateDisplay');

  function markActivePreset(btnList, matchFn) {
    btnList.forEach((b) => b.classList.toggle('active', matchFn(b)));
  }

  function updatePulseCalculations(val) {
    var imp = parseInt(val, 10);
    if (isNaN(imp) || imp <= 0) return;

    expPulseRate.textContent = imp.toLocaleString() + ' imp/kWh';
    expWhPerPulse.textContent = (1000 / imp).toFixed(2) + ' Wh';
    pulseWhSummary.textContent = (1000 / imp).toFixed(3) + ' Wh';

    // Blinks/min = (1,420 W * imp) / 60,000
    if (pulseRateDisplay) {
      pulseRateDisplay.textContent = ((1420 * imp) / 60000).toFixed(1) + ' blinks/min';
    }

    markActivePreset(presetBtns, (b) => parseInt(b.dataset.value, 10) === imp);

    // Rate cost depends on pulse constant — recalculate
    updateRateCalculations(rateInput.value);
  }

  presetBtns.forEach((btn) => {
    btn.addEventListener('click', () => {
      pulseInput.value = btn.dataset.value;
      updatePulseCalculations(btn.dataset.value);
    });
  });

  decBtn.addEventListener('click', () => {
    var current = parseInt(pulseInput.value, 10) || 1000;
    if (current > 100) {
      current -= 100;
      pulseInput.value = current;
      updatePulseCalculations(current);
    }
  });

  incBtn.addEventListener('click', () => {
    var current = parseInt(pulseInput.value, 10) || 1000;
    if (current < 10000) {
      current += 100;
      pulseInput.value = current;
      updatePulseCalculations(current);
    }
  });

  pulseInput.addEventListener('input', () => updatePulseCalculations(pulseInput.value));

  // ==========================================
  // 5. Utility rate stepper & cost calculations
  // ==========================================
  var rateInput = document.getElementById('rateInput');
  var decRateBtn = document.getElementById('decRateBtn');
  var incRateBtn = document.getElementById('incRateBtn');
  var ratePresetBtns = document.querySelectorAll('.rate-preset-btn');
  var rateDisplay = document.getElementById('rateDisplay');
  var costPerPulse = document.getElementById('costPerPulse');
  var costPerImpulse = document.getElementById('costPerImpulse');
  var costPerHour = document.getElementById('costPerHour');
  var projectedCost = document.getElementById('projectedCost');

  var MIN_UTIL_RATE = 0.1;
  var MAX_UTIL_RATE = 25;

  function updateRateCalculations(rateVal) {
    var rate = parseFloat(rateVal);
    if (isNaN(rate) || rate <= 0) return;

    var imp = parseInt(pulseInput.value, 10) || 1000;
    var costPerImp = rate / imp;
    var hourlyCost = rate * 1.4;      // 1.4 kW current draw
    var monthlyCost = hourlyCost * 24 * 30;

    rateDisplay.textContent = '₱' + rate.toFixed(2) + '/kWh';
    costPerPulse.textContent = '₱' + costPerImp.toFixed(6);
    costPerImpulse.textContent = '₱' + costPerImp.toFixed(6);
    costPerHour.textContent = '₱' + hourlyCost.toFixed(3);
    projectedCost.textContent = '₱' + monthlyCost.toFixed(2);

    markActivePreset(ratePresetBtns, (b) => parseFloat(b.dataset.value) === rate);
  }

  ratePresetBtns.forEach((btn) => {
    btn.addEventListener('click', () => {
      rateInput.value = btn.dataset.value;
      updateRateCalculations(btn.dataset.value);
    });
  });

  decRateBtn.addEventListener('click', () => {
    var current = parseFloat(rateInput.value) || 12;
    if (current > MIN_UTIL_RATE) {
      current = Math.max(0.01, current - 1);
      rateInput.value = current.toFixed(2);
      updateRateCalculations(current);
    }
  });

  incRateBtn.addEventListener('click', () => {
    var current = parseFloat(rateInput.value) || 0.12;
    if (current < MAX_UTIL_RATE) {
      current = Math.min(MAX_UTIL_RATE, current + 1);
      rateInput.value = current.toFixed(2);
      updateRateCalculations(current);
    }
  });

  rateInput.addEventListener('input', () => updateRateCalculations(rateInput.value));

  // Initialize rate calculations on load
  updateRateCalculations(rateInput.value);

  // ==========================================
  // 6. First-time user onboarding popup
  // ==========================================
  var dialog = document.getElementById('onboardingDialog');
  var newBtn = document.getElementById('onboardNewBtn');
  var existingBtn = document.getElementById('onboardExistingBtn');
  var closeX = document.getElementById('onboardCloseX');
  var baselineSection = document.getElementById('baselineSection');

  var SETUP_KEY = 'wattrack-onboarded';
  var PULSE_KEY = 'wattrack-pulse';
  var RATE_KEY = 'wattrack-rate';

  function showOnboarding() {
    if (dialog && typeof dialog.showModal === 'function') dialog.showModal();
  }
  function closeOnboarding() {
    if (dialog && typeof dialog.close === 'function') dialog.close();
  }

  if (newBtn) {
    newBtn.addEventListener('click', () => {
      try { localStorage.setItem(SETUP_KEY, '1'); } catch (e) { }
      closeOnboarding();
      if (baselineSection) {
        baselineSection.hidden = false;
        baselineSection.scrollIntoView({ behavior: 'smooth', block: 'center' });
      }
    });
  }

  if (existingBtn) {
    existingBtn.addEventListener('click', () => {
      try { localStorage.setItem(SETUP_KEY, '1'); } catch (e) { }
      closeOnboarding();
    });
  }

  if (closeX) {
    closeX.addEventListener('click', closeOnboarding);
  }

  // Esc also closes (native dialog behaviour) — mark seen so it
  // doesn't nag on every reload.
  if (dialog) {
    dialog.addEventListener('close', () => {
      try { localStorage.setItem(SETUP_KEY, '1'); } catch (e) { }
    });
    // First visit (or revisit before answering): show the popup
    if (!localStorage.getItem(SETUP_KEY)) {
      showOnboarding();
    }
  }

  // ==========================================
  // 7. Baseline reading form (first-time setup section)
  // ==========================================
  var baselineForm = document.getElementById('baselineForm');
  var baselineValue = document.getElementById('baselineValue');
  var baselineUnit = document.getElementById('baselineUnit');
  var baselineDate = document.getElementById('baselineDate');
  var baselineStatus = document.getElementById('baselineStatus');
  var baselineSaveBtn = document.getElementById('baselineSaveBtn');

  // Default the reading date to today
  if (baselineDate && !baselineDate.value) {
    baselineDate.value = new Date().toISOString().slice(0, 10);
  }

  if (baselineForm) {
    baselineForm.addEventListener('submit', (e) => {
      e.preventDefault();

      var reading = parseFloat(baselineValue.value);
      if (isNaN(reading) || reading < 0) return;

      var payload = {
        value: reading,
        unit: baselineUnit.value,
        date: baselineDate.value,
        // Where automation would plug in later:
        source: 'manual'
      };

      // Persist locally as an offline-safe fallback
      try { localStorage.setItem('wattrack-reading', JSON.stringify(payload)); } catch (err) { }

      // POST to the ESP32 so it can store the baseline in flash/EEPROM.
      // The firmware currently returns 404 for unknown routes — a future
      // handler can persist this instead.
      fetch('/api/reading', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      }).catch(() => { /* offline-safe: local copy already saved */ });

      // Feedback
      var original = baselineSaveBtn.textContent;
      baselineSaveBtn.disabled = true;
      baselineSaveBtn.textContent = 'Saving...';
      setTimeout(() => {
        baselineSaveBtn.disabled = false;
        baselineSaveBtn.textContent = original;
        baselineStatus.hidden = false;
        setTimeout(() => { baselineStatus.hidden = true; }, 2500);
      }, 500);
    });
  }

  // Restore saved values from a previous session (local mirror)
  try {
    var savedReading = localStorage.getItem('wattrack-reading');
    if (savedReading && baselineValue) {
      var r = JSON.parse(savedReading);
      baselineValue.value = r.value;
      baselineUnit.value = r.unit;
      baselineDate.value = r.date;
    }
  } catch (err) { }

  // Restore saved pulse constant / rate if present
  try {
    var savedPulse = localStorage.getItem(PULSE_KEY);
    if (savedPulse && pulseInput) pulseInput.value = savedPulse;
    var savedRate = localStorage.getItem(RATE_KEY);
    if (savedRate && rateInput) rateInput.value = savedRate;
  } catch (err) { }

  // ==========================================
  // 8. Save buttons — POST config to ESP32 + visual feedback
  // ==========================================
  var saveBtn = document.getElementById('saveConfigBtn');
  var saveStatus = document.getElementById('saveStatus');
  var saveRateBtn = document.getElementById('saveRateBtn');
  var rateSaveStatus = document.getElementById('rateSaveStatus');

  function flashSaveStatus(statusEl) {
    if (!statusEl) return;
    statusEl.hidden = false;
    setTimeout(() => { statusEl.hidden = true; }, 2500);
  }

  if (saveBtn) {
    saveBtn.addEventListener('click', () => {
      var originalContent = saveBtn.innerHTML;
      saveBtn.disabled = true;
      saveBtn.innerHTML = '<span class="spin">⏳</span><span>Writing EEPROM...</span>';

      fetch('/api/pulse-config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pulseConstant: parseInt(pulseInput.value, 10) || 1000 })
      }).catch(() => { });

      try { localStorage.setItem(PULSE_KEY, pulseInput.value); } catch (err) { }

      setTimeout(() => {
        saveBtn.disabled = false;
        saveBtn.innerHTML = '<span>✔ Saved!</span>';
        flashSaveStatus(saveStatus);
        setTimeout(() => {
          saveBtn.innerHTML = originalContent;
        }, 1600);
      }, 700);
    });
  }

  if (saveRateBtn) {
    saveRateBtn.addEventListener('click', () => {
      var originalContent = saveRateBtn.innerHTML;
      saveRateBtn.disabled = true;
      saveRateBtn.innerHTML = '<span class="spin">⏳</span><span>Writing EEPROM...</span>';

      fetch('/api/rate-config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ rate: parseFloat(rateInput.value) || 11.5 })
      }).catch(() => { });

      try { localStorage.setItem(RATE_KEY, rateInput.value); } catch (err) { }

      setTimeout(() => {
        saveRateBtn.disabled = false;
        saveRateBtn.innerHTML = '<span>✔ Saved!</span>';
        flashSaveStatus(rateSaveStatus);
        setTimeout(() => {
          saveRateBtn.innerHTML = originalContent;
        }, 1600);
      }, 700);
    });
  }

  // ==========================================
  // 9. Sync button (ping ESP32 controller)
  // ==========================================
  var reconnectBtn = document.getElementById('reconnectBtn');
  var syncIcon = document.getElementById('syncIcon');
  if (reconnectBtn) {
    reconnectBtn.addEventListener('click', () => {
      if (syncIcon) syncIcon.classList.add('spin');
      setTimeout(() => {
        if (syncIcon) syncIcon.classList.remove('spin');
      }, 800);
    });
  }
});

// ==========================================
// 10. fetch the metrics from ESP32
// ==========================================

async function updateMetrics() {
  try {
    const response = await fetch('/api/metrics');

    if (!response.ok) {
      throw new Error(`Failed to fetch ${response.status}`);
    }

    const metrics = await response.json();

    const kwhElement = document.getElementById('powerNowValue');
    const wattElement = document.getElementById('powerNowWatts');

    const pulseElement = document.getElementById('pulseDisplay');

    if (kwhElement) {
      kwhElement.textContent = Number(metrics.kwh).toFixed(2);
    }

    if (wattElement) {
      wattElement.textContent = Number(metrics.kwh * 1000);
    }

    if (pulseElement) {
      pulseElement.textContent = `${metrics.pulses} pulses`;
    }
  } catch (error) {
    console.error(error);
  }
}

updateMetrics();
setInterval(updateMetrics, 750);

// ==========================================
// 11. fetch IP address of esp32 
// ==========================================

function updateIPAddress() {
  fetch('/api/ip_address')
    .then(response => {
      if (!response.ok) {
        throw new Error(`failed to fetch /api/ip_address`);
      }
      return response.text();
    })
    .then(data =>{
      document.getElementById("ip_address").textContent = data;
    })
    .catch(error => {
      console.error(error);
      document.getElementById("ip_address").textContent = 'Error';
    });
};
updateIPAddress();