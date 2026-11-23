// JG Designs Mastering EQ \u2014 prototype interface.
//
// The look is ported from design board C13 (Ice Grey), see design/reference/.
// This file has four parts:
//   1. A bridge to the plugin (or a mock when the page is opened in a normal browser).
//   2. A tiny template engine for ui/template.html ({{holes}}, <sc-for>, <sc-if>).
//   3. renderVals(): turns the current parameter values into everything the template shows.
//   4. Mouse handling: drag knobs, click buttons, double-click to reset, scroll for fine moves.
'use strict';

// ---------------------------------------------------------------------------
// Band layout (display only; the real ranges come from the plugin's parameters)
// ---------------------------------------------------------------------------
const BANDS = [
  { name: 'SUB',      nums: ['16', '20', '25', '30', '40', '50', '60', '80', '100'], tag: 'HARMONICS' },
  { name: 'LOW',      nums: ['40', '60', '80', '100', '150', '200', '300'],          tag: 'HARMONICS' },
  { name: 'LOW MID',  nums: ['150', '200', '300', '400', '500', '700', '1k'],        tag: 'SATURATING' },
  { name: 'MID',      nums: ['500', '700', '1k', '1.4k', '2k', '2.8k', '4k'],        tag: 'SATURATING' },
  { name: 'HIGH MID', nums: ['1.5k', '2k', '2.5k', '3k', '4k', '5k', '6k'],          tag: 'SATURATING' },
  { name: 'PRESENCE', nums: ['3k', '4k', '5k', '6k', '8k', '10k', '12k'],            tag: 'SATURATING' },
  { name: 'AIR',      nums: ['6k', '8k', '10k', '12k', '14k', '16k', '18k', '20k', '24k'], tag: 'SATURATING' }
];
const REDLINE_DB = 4;

// ---------------------------------------------------------------------------
// 1. Bridge
// ---------------------------------------------------------------------------
const backend = (window.__JUCE__ && window.__JUCE__.backend) ? window.__JUCE__.backend : null;

const meta = {};        // id -> { kind, min, max, def, map, interval, choices }
const values = {};      // id -> plain value (number; bools are 0/1; choices are an index)
const meters = { in: [-120, -120], out: [-120, -120] };
const holds = { in: [{ v: -120, t: 0 }, { v: -120, t: 0 }], out: [{ v: -120, t: 0 }, { v: -120, t: 0 }] };
const ui = { active: 5, dragging: null };

function send(eventId, payload) { if (backend) backend.emitEvent(eventId, payload); }

function setParam(id, plain) {
  const m = meta[id];
  if (!m) return;
  plain = legalise(m, plain);
  if (values[id] === plain) return;
  values[id] = plain;
  send('jg_set', { id, value: plain });
  scheduleRender();
}

function legalise(m, v) {
  v = Math.min(m.max, Math.max(m.min, v));
  if (m.kind !== 'float') return Math.round(v);
  if (m.id && m.id.endsWith('_freq')) return v < 1000 ? Math.round(v) : Math.round(v / 10) * 10;
  if (m.interval > 0) return parseFloat((Math.round(v / m.interval) * m.interval).toFixed(4));
  return v;
}

const toNorm = (m, v) => m.map === 'log'
  ? Math.log(v / m.min) / Math.log(m.max / m.min)
  : (v - m.min) / (m.max - m.min);
const fromNorm = (m, n) => {
  n = Math.min(1, Math.max(0, n));
  return m.map === 'log' ? m.min * Math.pow(m.max / m.min, n) : m.min + n * (m.max - m.min);
};

function applyInit(payload) {
  for (const m of payload.meta) meta[m.id] = m;
  Object.assign(values, payload.values);
  scheduleRender();
}

function peakToHold(kind, ch, db, now) {
  const h = holds[kind][ch];
  if (db >= h.v || now - h.t > 1500) { h.v = db; h.t = now; }
}

function startBridge() {
if (backend) {
  backend.addEventListener('jg_init', applyInit);
  backend.addEventListener('jg_params', (p) => {
    for (const id in p) if (!(ui.dragging && ui.dragging.id === id)) values[id] = p[id];
    scheduleRender();
  });
  backend.addEventListener('jg_meters', (p) => {
    const now = performance.now();
    meters.in = p.in; meters.out = p.out;
    for (let c = 0; c < 2; c++) { peakToHold('in', c, p.in[c], now); peakToHold('out', c, p.out[c], now); }
    scheduleRender();
  });
  send('jg_ready', {});
} else {
  // Opened in a normal browser (for design work): fake parameters and meters.
  applyInit(mockInit());
  setInterval(() => {
    const now = performance.now();
    const t = now / 1000;
    const lvl = (k) => -10 + 4 * Math.sin(t * 2.1 + k) + 3 * Math.sin(t * 7.3 + k * 2) - Math.random() * 2;
    meters.in = [lvl(0), lvl(1)]; meters.out = [lvl(0), lvl(1)];
    for (let c = 0; c < 2; c++) { peakToHold('in', c, meters.in[c], now); peakToHold('out', c, meters.out[c], now); }
    scheduleRender();
  }, 33);
}
}

function mockInit() {
  const out = { meta: [], values: {} };
  const add = (m) => { out.meta.push(m); out.values[m.id] = m.def; };
  const f = (id, min, max, def, map = 'lin', interval = 0) => add({ id, kind: 'float', min, max, def, map, interval });
  const b = (id, def) => add({ id, kind: 'bool', min: 0, max: 1, def });
  const c = (id, choices, def) => add({ id, kind: 'choice', min: 0, max: choices.length - 1, def, choices });
  f('in_drive', 0, 10, 3, 'lin', 0.1); b('in_bypass', 0); b('in_lift', 0);
  f('seq_amount', 0, 100, 30, 'lin', 1); c('seq_range', ['\u00B11.5 dB', '\u00B13 dB'], 1);
  b('seq_cut_only', 1); b('seq_freeze', 0); b('seq_delta', 0); b('seq_bypass', 0);
  const defs = [[40, 16, 100, 1, 0.7], [100, 40, 300, 0, 0.8], [300, 150, 1000, 0, 1.2], [1000, 500, 4000, 0, 1.0],
    [3000, 1500, 6000, 0, 1.4], [6000, 3000, 12000, 0, 2.0], [16000, 6000, 24000, 2, 0.7]];
  defs.forEach((d, i) => {
    const n = i + 1;
    f(`peq_b${n}_freq`, d[1], d[2], d[0], 'log'); out.meta[out.meta.length - 1].id = `peq_b${n}_freq`;
    f(`peq_b${n}_gain`, -10, 10, 0, 'lin', 0.1);
    f(`peq_b${n}_q`, 0.3, 4, d[4], 'log', 0.1);
    c(`peq_b${n}_shape`, ['Bell', 'Low Shelf', 'High Shelf'], d[3]);
  });
  c('peq_sat', ['Tube', 'Iron', 'SS', 'Tape'], 0); b('peq_bypass', 0);
  f('out_gain', -12, 12, 0, 'lin', 0.1);
  c('out_target', ['\u221223 LUFS', '\u221216 LUFS', '\u221214 LUFS', '\u221211 LUFS', '\u22129 LUFS'], 2);
  b('glb_bypass', 0); c('glb_oversampling', ['1x', '2x', '4x'], 1);
  return out;
}

// ---------------------------------------------------------------------------
// 2. Template engine
// ---------------------------------------------------------------------------
function lookup(scope, path) {
  let v = scope;
  for (const k of path.trim().split('.')) { if (v == null) return ''; v = v[k]; }
  return v;
}
function interp(str, scope) {
  if (str.indexOf('{{') < 0) return str;
  return str.replace(/\{\{([^}]+)\}\}/g, (_, p) => { const v = lookup(scope, p); return v == null ? '' : String(v); });
}
const listPath = (attr) => attr.replace(/^\s*\{\{|\}\}\s*$/g, '');

function renderInto(tplParent, scope, out) {
  for (const node of tplParent.childNodes) {
    if (node.nodeType === 3) { out.appendChild(document.createTextNode(interp(node.nodeValue, scope))); continue; }
    if (node.nodeType !== 1) continue;
    const tag = node.localName;
    if (tag === 'sc-for') {
      const list = lookup(scope, listPath(node.getAttribute('list'))) || [];
      const as = node.getAttribute('as');
      for (const item of list) { const s = Object.create(scope); s[as] = item; renderInto(node, s, out); }
      continue;
    }
    if (tag === 'sc-if') {
      if (lookup(scope, listPath(node.getAttribute('value')))) renderInto(node, scope, out);
      continue;
    }
    const el = document.createElementNS(node.namespaceURI, node.tagName);
    for (const a of node.attributes) el.setAttribute(a.name, interp(a.value, scope));
    renderInto(node, scope, el);
    out.appendChild(el);
  }
}

// Patch the live DOM to match a freshly rendered tree, touching only what changed.
function morph(live, next, keepAttrs) {
  if (live.nodeType === 1 && !keepAttrs) {
    for (const a of next.attributes) if (live.getAttribute(a.name) !== a.value) live.setAttribute(a.name, a.value);
    for (const a of [...live.attributes]) if (!next.hasAttribute(a.name)) live.removeAttribute(a.name);
  }
  const nextKids = [...next.childNodes];
  for (let i = 0; i < nextKids.length; i++) {
    const l = live.childNodes[i];
    const n = nextKids[i];
    if (!l) live.appendChild(n);
    else if (l.nodeType !== n.nodeType || l.nodeName !== n.nodeName) live.replaceChild(n, l);
    else if (l.nodeType === 3) { if (l.nodeValue !== n.nodeValue) l.nodeValue = n.nodeValue; }
    else morph(l, n);
  }
  while (live.childNodes.length > nextKids.length) live.removeChild(live.lastChild);
}

let tpl = null;
let renderQueued = false;
function scheduleRender() {
  if (renderQueued) return;
  renderQueued = true;
  requestAnimationFrame(() => { renderQueued = false; render(); });
}
function render() {
  if (!tpl || !Object.keys(meta).length) return;
  const vals = renderVals();
  const frag = document.createElement('div');
  renderInto(tpl, vals, frag);
  morph(document.getElementById('root'), frag, true);
}

// ---------------------------------------------------------------------------
// 3. renderVals: parameter values -> display values (ported from board C13)
// ---------------------------------------------------------------------------
const pt = (cx, cy, r, deg) => { const a = deg * Math.PI / 180; return [cx + r * Math.sin(a), cy - r * Math.cos(a)]; };
const seg = (c, r1, r2, deg) => { const p = pt(c, c, r1, deg), q = pt(c, c, r2, deg); return 'M' + p[0].toFixed(2) + ' ' + p[1].toFixed(2) + ' L' + q[0].toFixed(2) + ' ' + q[1].toFixed(2) + ' '; };
const arc = (c, r, a0, a1) => {
  const p = pt(c, c, r, a0), q = pt(c, c, r, a1);
  const large = a1 - a0 > 180 ? 1 : 0;
  return 'M' + p[0].toFixed(2) + ' ' + p[1].toFixed(2) + ' A' + r + ' ' + r + ' 0 ' + large + ' 1 ' + q[0].toFixed(2) + ' ' + q[1].toFixed(2);
};
const parseHz = (t) => t.endsWith('k') ? parseFloat(t) * 1000 : parseFloat(t);
const minus = (s) => s.replace('-', '\u2212');
const fmtHz = (f) => f < 1000 ? Math.round(f) + ' Hz' : (f / 1000).toFixed(2) + ' kHz';
const fmtDb = (g) => (g > 0.05 ? '+' : g < -0.05 ? '\u2212' : '') + Math.abs(g).toFixed(1) + ' dB';
const fmtQ = (q) => q.toFixed(1);
const SHAPES = ['bell', 'low', 'high'];

// Where a frequency sits on a dial's printed scale (fractional index between numerals).
function dialPosition(nums, hz) {
  const v = nums.map(parseHz);
  if (hz <= v[0]) return 0;
  for (let k = 0; k < v.length - 1; k++) {
    if (hz <= v[k + 1]) return k + Math.log(hz / v[k]) / Math.log(v[k + 1] / v[k]);
  }
  return v.length - 1;
}

function dial(o) {
  const s = o.s, f = s - 16, c = f / 2, n = o.nums.length, step = 270 / (n - 1);
  let minor = '';
  for (let a = -135; a <= 135.01; a += step / 5) minor += seg(c, c - 8, c - 3, a);
  const angles = o.nums.map((t, i) => -135 + i * step);
  const R = c - o.inset;
  const nums = o.nums.map((t, i) => { const p = pt(c, c, R, angles[i]); return { x: p[0].toFixed(1), y: p[1].toFixed(1), t }; });
  const rot = -135 + o.at * step;
  const kr = Math.round(f * 0.15);
  const g = o.gain;
  const hot = g > REDLINE_DB;
  // Right = boost, left = cut (brief: "Turn left to cut, right to boost").
  const gAng = g / 10 * 135;
  let gArc = 'M0 0';
  if (g > 0.05) gArc = arc(c, kr + 9, 0, gAng);
  else if (g < -0.05) gArc = arc(c, kr + 9, gAng, 0);
  const cap = kr * 2 - 12, nLen = c - 12;
  const idx = angles.map((a) => { const p = pt(c, c, c - 9, a); return { x: p[0].toFixed(2), y: p[1].toFixed(2), r: a }; });
  const valueTop = Math.round(c + f * 0.29);
  return {
    i: o.i, n: o.i + 1, idx, s, f, vb: '0 0 ' + f + ' ' + f, left: o.cx - s / 2, top: o.cy - s / 2, z: o.z, chamfer: s - 8,
    minor, nums, numSize: o.numSize, name: o.name, value: o.value, rot,
    labelTop: Math.round(c + f * 0.21), valueTop, gainTop: valueTop + 15,
    gainText: fmtDb(g), gainColor: hot ? '#FFC2C3' : (g < -0.05 ? '#BFF4FF' : '#A9ACAF'),
    nLeft: c - 2.5, nTop: c - nLen, nLen,
    skirtLeft: c - kr - 6, skirtSize: (kr + 6) * 2,
    knobLeft: c - kr, knobSize: kr * 2, capSize: cap,
    gRot: gAng, gPtrLeft: cap / 2 - 1.5, gPtrH: Math.round(cap * 0.32),
    gPtr: hot ? '#FF3B3F' : '#F2F3F4', gPtrGlow: hot ? '0 0 6px rgba(255,59,63,0.9)' : 'none',
    gArc, gArcColor: hot ? '#FF3B3F' : (g < 0 ? '#5CE1FF' : '#F2F3F4'),
    gArcGlow: hot ? 'drop-shadow(0 0 3px rgba(255,59,63,0.95))' : (g < 0 ? 'drop-shadow(0 0 3px rgba(92,225,255,0.8))' : 'none')
  };
}

function renderVals() {
  const V = (id) => values[id];
  const paint = 'linear-gradient(180deg, #76828E 0%, #606B77 16%, #67737F 48%, #515B66 80%, #727E8A 100%)';
  const sizes = [196, 184, 174, 166, 174, 184, 196];
  const centersX = [141, 317, 482, 638, 794, 959, 1135];
  const insets = [28, 27, 25, 24, 25, 27, 28];
  const zs = [1, 2, 3, 4, 3, 2, 1];
  const dialOffsets = [0, -14, -26, -36, -26, -14, 0];
  const maxS = 196, rowTop = 44;
  const cy = rowTop + maxS / 2;

  const band = BANDS.map((b, i) => {
    const n = i + 1;
    const hz = V(`peq_b${n}_freq`);
    return { ...b, hz, gain: V(`peq_b${n}_gain`), q: V(`peq_b${n}_q`), type: SHAPES[V(`peq_b${n}_shape`)] || 'bell' };
  });

  const dials = band.map((b, i) => dial({
    i, s: sizes[i], cx: centersX[i], cy: cy + dialOffsets[i], z: zs[i], name: b.name, value: fmtHz(b.hz), nums: b.nums,
    at: dialPosition(b.nums, b.hz), inset: insets[i], numSize: i === 0 || i === 6 ? 12 : 11, gain: b.gain
  }));
  const rows = [{ hasLabel: false, dials }];

  // Q window
  const win = (() => {
    const left = 40, top = 252, w = 1196, h = 76, iw = w - 10, ih = h - 10, off = left + 5, base = 34, amp = 22;
    const cxs = centersX.map((x) => x - off);
    const edges = [0];
    for (let i = 0; i < cxs.length - 1; i++) edges.push((cxs[i] + cxs[i + 1]) / 2);
    edges.push(iw);
    const zones = band.map((b, i) => ({ i, n: i + 1, name: b.name, x: edges[i].toFixed(1), w: (edges[i + 1] - edges[i]).toFixed(1) }));
    const glyph = (b, gx, gy, gw, gh) => {
      const pts = [];
      for (let k = 0; k <= 24; k++) {
        const u = k / 24;
        let v;
        if (b.type === 'bell') v = Math.exp(-Math.pow((u - 0.5) / (0.22 / b.q), 2));
        else if (b.type === 'low') v = 1 / (1 + Math.exp((u - 0.5) / (0.08 / b.q)));
        else v = 1 / (1 + Math.exp(-(u - 0.5) / (0.08 / b.q)));
        pts.push((gx + u * gw).toFixed(1) + ' ' + (gy + gh - v * gh).toFixed(1));
      }
      return 'M' + pts.join(' L') + ' ';
    };
    const ai = ui.active;
    let icons = '', hotIcons = '';
    const labels = [];
    band.forEach((b, i) => {
      if (i === ai) return;
      const hot = b.gain > REDLINE_DB;
      const g = glyph(b, cxs[i] - 42, 20, 44, 26);
      if (hot) hotIcons += g; else icons += g;
      labels.push({ x: cxs[i] + 12, q: fmtQ(b.q), color: hot ? '#FFC2C3' : '#E8FBFF' });
    });
    const a = band[ai];
    const aHot = a.gain > REDLINE_DB;
    const ac = cxs[ai];
    const sig = 30 / a.q;
    const ax0 = edges[ai] + 6, ax1 = edges[ai + 1] - 6;
    const sign = a.gain < -0.05 ? -1 : 1; // draw cuts as dips
    const shapeFn = (x) => {
      if (a.type === 'bell') return Math.exp(-Math.pow((x - ac) / sig, 2));
      if (a.type === 'low') return 1 / (1 + Math.exp((x - ac) / (9 / a.q)));
      return 1 / (1 + Math.exp(-(x - ac) / (9 / a.q)));
    };
    const depth = 22, mid = sign > 0 ? 42 : 22;
    const apts = [];
    for (let x = ax0; x <= ax1; x += 2) apts.push([x, mid - sign * depth * shapeFn(x)]);
    const acurve = 'M' + apts.map((p) => p[0].toFixed(1) + ' ' + p[1].toFixed(1)).join(' L');
    let handles, bar = 'M0 0';
    if (a.type === 'bell') {
      const adx = 0.589 * sig, ahy = mid - sign * depth * 0.707;
      handles = [{ x: (ac - adx).toFixed(1), y: ahy.toFixed(1) }, { x: (ac + adx).toFixed(1), y: ahy.toFixed(1) }];
      bar = 'M' + (ac - adx).toFixed(1) + ' ' + ahy.toFixed(1) + ' L' + (ac + adx).toFixed(1) + ' ' + ahy.toFixed(1);
    } else {
      handles = [{ x: ac.toFixed(1), y: (mid - sign * depth * 0.5).toFixed(1) }];
    }
    const active = {
      n: ai + 1, x: edges[ai] + 3, w: edges[ai + 1] - edges[ai] - 6, q: fmtQ(a.q),
      shape: a.type === 'bell' ? 'BELL' : (a.type === 'low' ? 'LO SHELF' : 'HI SHELF'),
      curve: acurve, fill: acurve + ' L' + ax1.toFixed(1) + ' ' + mid + ' L' + ax0.toFixed(1) + ' ' + mid + ' Z',
      bar, handles, textX: edges[ai] + 12, chipX: edges[ai + 1] - 10,
      color: aHot ? '#FF5A5D' : '#5CE1FF', glow: aHot ? 'rgba(255,59,63,0.9)' : 'rgba(92,225,255,0.9)',
      fillColor: aHot ? 'rgba(255,59,63,0.18)' : 'rgba(92,225,255,0.16)', handle: aHot ? '#FFB3B4' : '#BFF4FF',
      border: aHot ? 'rgba(255,59,63,0.5)' : 'rgba(92,225,255,0.5)', bg: aHot ? 'rgba(255,59,63,0.07)' : 'rgba(92,225,255,0.06)',
      halo: aHot ? 'rgba(255,59,63,0.25)' : 'rgba(92,225,255,0.2)', text: aHot ? '#FFC2C3' : '#E8FBFF'
    };
    let stems = '', hotStems = '';
    band.forEach((b, i) => {
      const dialBottom = cy + dialOffsets[i] + sizes[i] / 2 + 3;
      const s = 'M' + centersX[i] + ' ' + dialBottom + ' L' + centersX[i] + ' ' + top + ' ';
      if (b.gain > REDLINE_DB) hotStems += s; else stems += s;
    });
    return { icons: icons || 'M0 0', hotIcons: hotIcons || 'M0 0', labels, active, left, top, w, h, iw, ih, vb: '0 0 ' + iw + ' ' + ih, zones,
      stems: stems || 'M0 0', hotStems: hotStems || 'M0 0' };
  })();

  // Meters
  const lvl = (db) => {
    const p = [[0, 1], [-6, 0.85], [-12, 0.7], [-24, 0.45], [-48, 0.1]];
    if (db >= 0) return 1;
    for (let k = 0; k < p.length - 1; k++) if (db <= p[k][0] && db >= p[k + 1][0]) return p[k + 1][1] + (p[k][1] - p[k + 1][1]) * (db - p[k + 1][0]) / (p[k][0] - p[k + 1][0]);
    return db > -90 ? 0.05 : 0;
  };
  const pc = (db) => (lvl(db) * 100).toFixed(1);
  const bars = (k) => [0, 1].map((c) => ({ pct: pc(meters[k][c]), peak: pc(holds[k][c].v), peakColor: holds[k][c].v > -1 ? '#FF3B3F' : '#E8FBFF' }));
  const pk = (k) => [0, 1].map((c) => {
    const v = holds[k][c].v;
    return { t: v <= -90 ? '\u2212\u221E' : minus(v.toFixed(1)), color: v > -1 ? '#FFC2C3' : '#E8FBFF' };
  });

  // Buttons
  const pillRed = (on) => on
    ? { border: '#FF3B3F', bg: 'linear-gradient(180deg, #3A1A22 0%, #160A0E 100%)', glow: '0 0 10px rgba(255,59,63,0.45), inset 0 1px 0 rgba(255,255,255,0.15)', color: '#FFE1E2', dot: '#FF3B3F', dotGlow: '0 0 6px #FF3B3F' }
    : { border: '#393C3E', bg: '#0C0C0D', glow: 'none', color: '#A9ACAF', dot: '#393C3E', dotGlow: 'none' };
  const pOn = { border: 'rgba(92,225,255,0.8)', bg: 'rgba(92,225,255,0.14)', glow: '0 0 10px rgba(92,225,255,0.4)', color: '#E8FBFF' };
  const pOff = { border: '#393C3E', bg: '#0C0C0D', glow: 'none', color: '#A9ACAF' };
  const cOn = { color: '#FFE1E2', bg: 'rgba(255,59,63,0.22)', glow: '0 0 8px rgba(255,59,63,0.5)' };
  const cOff = { color: '#A9ACAF', bg: 'transparent', glow: 'none' };
  const badge = (on, cyan) => on
    ? (cyan ? { border: '#5CE1FF', color: '#5CE1FF', glow: '0 0 6px rgba(92,225,255,0.5)' }
            : { border: '#9AA0A6', color: '#DADDE0', glow: 'inset 1px 1px 1px rgba(0,0,0,0.8), 0 1px 0 rgba(255,255,255,0.12)' })
    : { border: '#3E4245', color: '#5E6267', glow: 'none' };
  const DIM = 'saturate(0.3) brightness(0.62)';

  const liftOn = !!V('in_lift');
  const inByp = !!V('in_bypass'), seqByp = !!V('seq_bypass'), peqByp = !!V('peq_bypass');
  const drive = V('in_drive'), dm = meta.in_drive;
  const amount = V('seq_amount');
  const satNames = ['TUBE', 'IRON', 'SS', 'TAPE'];

  return {
    paint, rows, win,
    inBars: bars('in'), outBars: bars('out'), inPk: pk('in'), outPk: pk('out'),
    lufsInt: '\u2212\u2212.\u2212',
    lufsRows: [
      { label: 'MOMENTARY', value: '\u2212\u2212.\u2212', color: '#8F9294' },
      { label: 'SHORT-TERM', value: '\u2212\u2212.\u2212', color: '#8F9294' },
      { label: 'TRUE PK', value: '\u2212\u2212.\u2212 dBTP', color: '#8F9294' }
    ],
    target: (meta.out_target.choices[V('out_target')] || ''),
    smartNodes: [[297, 114, '\u22121.2 dB'], [610, 124, '\u22122.0 dB']].map((n) => ({
      x: (n[0] * 564 / 816).toFixed(1), y: (n[1] * 104 / 200).toFixed(1), lx: (n[0] * 564 / 816).toFixed(1), ly: (n[1] * 104 / 200 + 10).toFixed(1), t: n[2]
    })),
    screws: [{ x: 24, y: 24, r: 38 }, { x: 1312, y: 24, r: -22 }, { x: 15, y: 392, r: 74 }, { x: 1321, y: 392, r: 12 }, { x: 24, y: 614, r: -55 }, { x: 1312, y: 614, r: 61 }],
    amount: { track: arc(35, 31, -135, 135), arc: amount > 0.5 ? arc(35, 31, -135, -135 + amount / 100 * 270) : 'M0 0', text: Math.round(amount) + '%' },
    smartPills: [
      { id: 'seq_range', t: 'MAX ' + (meta.seq_range.choices[V('seq_range')] || ''), ...pOn },
      { id: 'seq_cut_only', t: 'CUT ONLY', ...(V('seq_cut_only') ? pOn : pOff) },
      { id: 'seq_freeze', t: 'FREEZE', ...(V('seq_freeze') ? pOn : pOff) },
      { id: 'seq_delta', t: '\u0394 LISTEN', ...(V('seq_delta') ? pOn : pOff) }
    ],
    charPills: satNames.map((t, v) => ({ t, v, ...(V('peq_sat') === v ? cOn : cOff) })),
    drive: { rot: (-135 + toNorm(dm, drive) * 270).toFixed(1), text: drive.toFixed(1) },
    lift: { ...pillRed(liftOn), boxOpacity: liftOn ? 1 : 0.35 },
    inOn: pillRed(!inByp), inOff: pillRed(inByp),
    badge1: badge(!inByp, false), badge2: badge(!seqByp, true), badge3: badge(!peqByp, false),
    inDim: inByp ? DIM : 'none', seqDim: seqByp ? DIM : 'none', peqDim: peqByp ? DIM : 'none'
  };
}

// ---------------------------------------------------------------------------
// 4. Mouse handling
// ---------------------------------------------------------------------------
const DRAG_PIXELS = 220;   // pixels of vertical drag for the full range
const FINE = 0.1;          // hold Shift (or Alt) for fine control

function bandOf(el) { const b = el.getAttribute('data-band'); return b == null ? null : parseInt(b, 10); }

function onPointerDown(e) {
  const el = e.target.closest('[data-ctl]');
  if (!el || e.button !== 0) return;
  const id = el.getAttribute('data-ctl');
  const m = meta[id];
  if (!m) return;
  e.preventDefault();
  const b = bandOf(el);
  if (b != null && b !== ui.active) { ui.active = b; scheduleRender(); }
  ui.dragging = { id, lastY: e.clientY, norm: toNorm(m, values[id]) };
  send('jg_gesture', { id, begin: true });
  document.body.classList.add('dragging');
}

function onPointerMove(e) {
  const d = ui.dragging;
  if (!d) return;
  const m = meta[d.id];
  const dy = d.lastY - e.clientY;
  d.lastY = e.clientY;
  d.norm = Math.min(1, Math.max(0, d.norm + (dy / DRAG_PIXELS) * ((e.shiftKey || e.altKey) ? FINE : 1)));
  setParam(d.id, fromNorm(m, d.norm));
}

function onPointerUp() {
  const d = ui.dragging;
  if (!d) return;
  send('jg_gesture', { id: d.id, begin: false });
  ui.dragging = null;
  document.body.classList.remove('dragging');
}

function onDoubleClick(e) {
  const el = e.target.closest('[data-ctl]');
  if (!el) return;
  const id = el.getAttribute('data-ctl');
  const m = meta[id];
  if (!m) return;
  send('jg_gesture', { id, begin: true });
  setParam(id, m.def);
  send('jg_gesture', { id, begin: false });
}

let wheelTimer = null;
function onWheel(e) {
  const el = e.target.closest('[data-ctl]');
  if (!el) return;
  const id = el.getAttribute('data-ctl');
  const m = meta[id];
  if (!m) return;
  e.preventDefault();
  if (!wheelTimer) send('jg_gesture', { id, begin: true });
  clearTimeout(wheelTimer);
  wheelTimer = setTimeout(() => { send('jg_gesture', { id, begin: false }); wheelTimer = null; }, 300);
  const step = (e.shiftKey ? 0.002 : 0.01) * Math.sign(-e.deltaY || e.deltaX);
  setParam(id, fromNorm(m, toNorm(m, values[id]) + step));
}

function onClick(e) {
  const t = e.target.closest('[data-toggle],[data-set],[data-cycle]');
  if (!t) return;
  let id, v;
  if (t.hasAttribute('data-toggle')) {
    id = t.getAttribute('data-toggle');
    const m = meta[id];
    if (!m) return;
    v = m.kind === 'choice' ? (values[id] + 1) % (m.max + 1) : (values[id] ? 0 : 1);
  } else if (t.hasAttribute('data-set')) {
    id = t.getAttribute('data-set');
    v = parseFloat(t.getAttribute('data-value'));
  } else {
    id = t.getAttribute('data-cycle');
    const m = meta[id];
    if (!m) return;
    v = (values[id] + 1) % (m.max + 1);
  }
  send('jg_gesture', { id, begin: true });
  setParam(id, v);
  send('jg_gesture', { id, begin: false });
}

// Scale the fixed 1360 x 662 design to fit the window.
function fit() {
  const s = Math.min(window.innerWidth / 1360, window.innerHeight / 662);
  const r = document.getElementById('root');
  r.style.transform = `scale(${s})`;
  r.style.left = ((window.innerWidth - 1360 * s) / 2) + 'px';
  r.style.top = ((window.innerHeight - 662 * s) / 2) + 'px';
}

window.addEventListener('DOMContentLoaded', () => {
  tpl = document.getElementById('tpl').content;
  const root = document.getElementById('root');
  root.addEventListener('pointerdown', onPointerDown);
  window.addEventListener('pointermove', onPointerMove);
  window.addEventListener('pointerup', onPointerUp);
  window.addEventListener('pointercancel', onPointerUp);
  root.addEventListener('dblclick', onDoubleClick);
  root.addEventListener('wheel', onWheel, { passive: false });
  root.addEventListener('click', onClick);
  window.addEventListener('resize', fit);
  fit();
  startBridge();
  render();
});
