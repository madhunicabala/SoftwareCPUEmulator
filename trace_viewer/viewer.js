'use strict';

/* ── State ───────────────────────────────────────────────────────────── */
let _trace    = [];   // cycle objects from trace.json
let _map      = null; // parsed map.json or null
let _idx      = 0;    // current cycle index (0-based)
let _pcToLine = {};   // "0x2000" → index into _map.lines

/* ── Opcode category colours ─────────────────────────────────────────── */
const CAT_COLOR = {
  data:  '#3b82f6',
  arith: '#10b981',
  logic: '#8b5cf6',
  cmp:   '#eab308',
  jump:  '#f59e0b',
  call:  '#ef4444',
  io:    '#06b6d4',
  sys:   '#6b7280',
};

const OP_CAT = {
  NOP:'sys',  HALT:'sys',
  MOV:'data', LOAD:'data', STORE:'data', PUSH:'data', POP:'data',
  LOADB:'data', MOVW:'data',
  ADD:'arith', SUB:'arith', MUL:'arith', INC:'arith', DEC:'arith',
  AND:'logic', OR:'logic', XOR:'logic', NOT:'logic', SHL:'logic', SHR:'logic',
  CMP:'cmp',
  JMP:'jump', JZ:'jump', JNZ:'jump', JL:'jump', JGE:'jump', JC:'jump',
  CALL:'call', RET:'call',
  IN:'io', OUT:'io',
};

function opColor(name) {
  return CAT_COLOR[OP_CAT[name] || 'sys'];
}

/* ── DOM helpers ─────────────────────────────────────────────────────── */
const $ = id => document.getElementById(id);

function hex16(n) {
  return '0x' + (n & 0xFFFF).toString(16).toUpperCase().padStart(4, '0');
}

function escapeHtml(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

/* ── File loading ────────────────────────────────────────────────────── */
function loadTrace(file) {
  const reader = new FileReader();
  reader.onload = e => {
    try {
      _trace = JSON.parse(e.target.result);
      $('status-trace').textContent = `✓ ${file.name} (${_trace.length} cycles)`;
      $('zone-trace').classList.add('loaded');
      updateOpenBtn();
    } catch {
      $('status-trace').textContent = 'Parse error';
    }
  };
  reader.readAsText(file);
}

function loadMap(file) {
  const reader = new FileReader();
  reader.onload = e => {
    try {
      _map = JSON.parse(e.target.result);
      $('status-map').textContent = `✓ ${file.name} (${_map.lines.length} lines)`;
      $('zone-map').classList.add('loaded');
    } catch {
      $('status-map').textContent = 'Parse error';
    }
  };
  reader.readAsText(file);
}

function updateOpenBtn() {
  $('btn-open').disabled = (_trace.length === 0);
}

/* ── Build static DOM ────────────────────────────────────────────────── */
function buildSource() {
  _pcToLine = {};
  const container = $('source-lines');
  container.innerHTML = '';

  if (!_map) {
    container.innerHTML = '<div class="src-no-map">No source map loaded — drop map.json on the load screen to enable source tracking.</div>';
    return;
  }

  $('src-fname').textContent = _map.source.split('/').pop();

  _map.lines.forEach((ml, i) => { _pcToLine[ml.addr] = i; });

  const frag = document.createDocumentFragment();
  _map.lines.forEach(ml => {
    const row = document.createElement('div');
    row.className = 'src-row';
    row.innerHTML =
      `<span class="src-lnum">${ml.line}</span>` +
      `<span class="src-text">${escapeHtml(ml.text)}</span>`;
    frag.appendChild(row);
  });
  container.appendChild(frag);
}

function buildLog() {
  const container = $('log-rows');
  container.innerHTML = '';
  const frag = document.createDocumentFragment();

  _trace.forEach((cy, i) => {
    const color = opColor(cy.op);

    let srcText = '';
    if (_map && _pcToLine[cy.pc] !== undefined) {
      srcText = _map.lines[_pcToLine[cy.pc]].text
        .trim().replace(/;.*$/, '').trim();
    }

    const row = document.createElement('div');
    row.className = 'log-row';
    row.dataset.idx = i;
    row.innerHTML =
      `<span class="log-cyc">${cy.cycle}</span>` +
      `<span class="log-op" style="color:${color}">${cy.op}</span>` +
      `<span class="log-pc">${cy.pc}</span>` +
      `<span class="log-src">${escapeHtml(srcText)}</span>`;
    row.addEventListener('click', () => goTo(i));
    frag.appendChild(row);
  });

  container.appendChild(frag);
}

/* ── Open viewer ─────────────────────────────────────────────────────── */
function openViewer() {
  if (_trace.length === 0) return;

  buildSource();   // must precede buildLog — populates _pcToLine
  buildLog();

  $('tb-ctot').textContent = _trace.length;

  if (_map) {
    $('tb-src').textContent = _map.source.split('/').pop();
  }

  $('load-screen').hidden = true;
  $('viewer').hidden = false;

  _idx = 0;
  render();
  drawTimeline();
}

/* ── Navigation ──────────────────────────────────────────────────────── */
function goTo(n) {
  const prev = _idx;
  _idx = Math.max(0, Math.min(_trace.length - 1, n));
  render(prev);
}

/* ── Render ──────────────────────────────────────────────────────────── */
function render(prevIdx) {
  const cur = _trace[_idx];
  const prv = (prevIdx !== undefined) ? _trace[prevIdx] : null;

  renderTopbar(cur);
  renderSource(cur);
  renderCPU(cur, prv);
  renderLog();
  drawTimeline();

  $('ctl-pos').textContent = `cycle ${cur.cycle} / ${_trace.length}`;
}

function renderTopbar(cur) {
  $('tb-op').textContent  = cur.op;
  $('tb-op').style.color  = opColor(cur.op);
  $('tb-pc').textContent  = cur.pc;
  $('tb-cnum').textContent = cur.cycle;
}

function renderSource(cur) {
  const old = $('source-lines').querySelector('.src-row.active');
  if (old) old.classList.remove('active');

  if (!_map) return;
  const lineIdx = _pcToLine[cur.pc];
  if (lineIdx === undefined) return;

  const rows = $('source-lines').children;
  const row  = rows[lineIdx];
  if (!row) return;

  row.classList.add('active');
  row.scrollIntoView({ block: 'nearest', behavior: 'smooth' });
}

function renderCPU(cur, prv) {
  const MODES = ['REG', 'IMM', 'DIRECT', 'INDIRECT'];

  for (let i = 0; i < 4; i++) {
    const val = cur.reg[i];
    $(`rval-${i}`).textContent = hex16(val);
    $(`rdec-${i}`).textContent = val;

    const row = $(`rrow-${i}`);
    const changed = prv && prv.reg[i] !== val;
    if (changed) {
      row.classList.remove('changed');
      void row.offsetWidth;   // retrigger CSS animation
      row.classList.add('changed');
    } else {
      row.classList.remove('changed');
    }
  }

  $('sv-pc').textContent = cur.pc;
  $('sv-sp').textContent = cur.sp;

  setFlag('fl-Z', cur.flags.Z);
  setFlag('fl-N', cur.flags.N);
  setFlag('fl-C', cur.flags.C);
  setFlag('fl-V', cur.flags.V);

  $('ic-op').textContent    = cur.op;
  $('ic-op').style.color    = opColor(cur.op);
  $('icf-mode').textContent = `mode: ${MODES[cur.mode] ?? cur.mode}`;
  $('icf-dst').textContent  = `dst: R${cur.dst}`;
  $('icf-src').textContent  = `src: ${cur.src}`;
  $('ic-raw').textContent   = cur.instr;
}

function setFlag(id, val) {
  $(id).classList.toggle('flag-on', !!val);
}

function renderLog() {
  const rows = $('log-rows').children;
  const old  = $('log-rows').querySelector('.log-row.active');
  if (old) old.classList.remove('active');

  const row = rows[_idx];
  if (!row) return;

  row.classList.add('active');
  row.scrollIntoView({ block: 'nearest', behavior: 'smooth' });
}

/* ── Timeline ────────────────────────────────────────────────────────── */
function drawTimeline() {
  const canvas = $('timeline-canvas');
  const wrap   = $('timeline-wrap');
  const dpr    = window.devicePixelRatio || 1;
  const W      = wrap.clientWidth;
  const H      = wrap.clientHeight;

  canvas.width        = W * dpr;
  canvas.height       = H * dpr;
  canvas.style.width  = W + 'px';
  canvas.style.height = H + 'px';

  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);

  const n    = _trace.length;
  if (n === 0) return;

  const barW = W / n;
  const barH = H * 0.5;
  const barY = (H - barH) / 2;

  _trace.forEach((cy, i) => {
    ctx.fillStyle   = opColor(cy.op);
    ctx.globalAlpha = (i === _idx) ? 1.0 : 0.5;
    const x = i * barW;
    ctx.fillRect(x, barY, Math.max(barW - 0.5, 0.5), barH);
  });

  ctx.globalAlpha = 1.0;

  // Amber cursor
  const cx = (_idx + 0.5) * barW;
  ctx.strokeStyle = '#f59e0b';
  ctx.lineWidth   = 1.5;
  ctx.beginPath();
  ctx.moveTo(cx, 0);
  ctx.lineTo(cx, H);
  ctx.stroke();

  // Tick mark at top
  ctx.fillStyle = '#f59e0b';
  ctx.beginPath();
  ctx.moveTo(cx - 4, 0);
  ctx.lineTo(cx + 4, 0);
  ctx.lineTo(cx, 6);
  ctx.closePath();
  ctx.fill();
}

/* ── Drop zone helper ────────────────────────────────────────────────── */
function setupDropZone(zone, handler) {
  zone.addEventListener('dragover', e => {
    e.preventDefault();
    zone.classList.add('drag-over');
  });
  zone.addEventListener('dragleave', () => zone.classList.remove('drag-over'));
  zone.addEventListener('drop', e => {
    e.preventDefault();
    zone.classList.remove('drag-over');
    const file = e.dataTransfer.files[0];
    if (file) handler(file);
  });
}

/* ── Wire up load screen ─────────────────────────────────────────────── */
$('input-trace').addEventListener('change', e => {
  if (e.target.files[0]) loadTrace(e.target.files[0]);
});
$('input-map').addEventListener('change', e => {
  if (e.target.files[0]) loadMap(e.target.files[0]);
});

$('btn-open').addEventListener('click', openViewer);

setupDropZone($('zone-trace'), file => {
  if (file.name.endsWith('.json')) loadTrace(file);
});
setupDropZone($('zone-map'), file => {
  if (file.name.endsWith('.json')) loadMap(file);
});

/* ── Wire up viewer controls ─────────────────────────────────────────── */
$('btn-first').addEventListener('click', () => goTo(0));
$('btn-prev') .addEventListener('click', () => goTo(_idx - 1));
$('btn-next') .addEventListener('click', () => goTo(_idx + 1));
$('btn-last') .addEventListener('click', () => goTo(_trace.length - 1));

document.addEventListener('keydown', e => {
  if ($('viewer').hidden) return;
  if (e.target.tagName === 'INPUT') return;
  switch (e.key) {
    case 'ArrowLeft':  goTo(_idx - 1);            e.preventDefault(); break;
    case 'ArrowRight': goTo(_idx + 1);            e.preventDefault(); break;
    case 'Home':       goTo(0);                   e.preventDefault(); break;
    case 'End':        goTo(_trace.length - 1);   e.preventDefault(); break;
  }
});

$('jump-input').addEventListener('keydown', e => {
  if (e.key !== 'Enter') return;
  const n = parseInt($('jump-input').value, 10);
  if (!isNaN(n)) goTo(n - 1);   // UI is 1-based, index is 0-based
  $('jump-input').value = '';
  $('jump-input').blur();
});

$('timeline-canvas').addEventListener('click', e => {
  const rect = $('timeline-canvas').getBoundingClientRect();
  const frac = (e.clientX - rect.left) / rect.width;
  goTo(Math.floor(frac * _trace.length));
});

window.addEventListener('resize', () => {
  if (!$('viewer').hidden) drawTimeline();
});
