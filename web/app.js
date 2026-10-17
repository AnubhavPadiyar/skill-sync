// Skill Sync frontend - talks to the C++ backend (/api/*)
const $ = (s, r = document) => r.querySelector(s);
const esc = s => String(s).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const app = $('#app');
let me = JSON.parse(localStorage.getItem('ss_user') || 'null');

async function api(path, body) {
  const opt = body ? { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: new URLSearchParams(body) } : {};
  const r = await fetch(path, opt);
  const data = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(data.error || 'Something went wrong. Try again.');
  return data;
}
function toast(msg, bad) {
  const t = $('#toast');
  t.textContent = msg; t.className = 'show' + (bad ? ' bad' : '');
  clearTimeout(toast.t); toast.t = setTimeout(() => (t.className = ''), 3200);
}
function setMe(u) { me = u; u ? localStorage.setItem('ss_user', JSON.stringify(u)) : localStorage.removeItem('ss_user'); }
const chips = (list, role) => list.length ? `<div class="chips">${list.map(s => `<span class="chip ${role}">${esc(s)}</span>`).join('')}</div>` : '<span class="note">None yet</span>';

/* ---------- tag input ---------- */
function tagInput(host, initial, role, placeholder) {
  let tags = [...initial];
  host.classList.add('tags');
  const input = document.createElement('input');
  input.placeholder = placeholder || 'Type a skill, press Enter';
  const draw = () => {
    host.querySelectorAll('.chip').forEach(n => n.remove());
    tags.forEach((t, i) => {
      const c = document.createElement('span');
      c.className = 'chip ' + role;
      c.innerHTML = esc(t) + ' <button type="button" aria-label="Remove ' + esc(t) + '">&times;</button>';
      c.querySelector('button').onclick = e => { e.stopPropagation(); tags.splice(i, 1); draw(); };
      host.insertBefore(c, input);
    });
  };
  const add = () => {
    input.value.split(',').map(s => s.trim().toLowerCase()).filter(Boolean).forEach(s => { if (!tags.includes(s)) tags.push(s); });
    input.value = ''; draw();
  };
  input.addEventListener('keydown', e => {
    if (e.key === 'Enter' || e.key === ',') { e.preventDefault(); add(); }
    else if (e.key === 'Backspace' && !input.value && tags.length) { tags.pop(); draw(); }
  });
  input.addEventListener('blur', add);
  host.addEventListener('click', () => input.focus());
  host.appendChild(input); draw();
  return () => { add(); return tags.join(','); };
}

/* ---------- swap ring (SVG) ---------- */
function ringSvg(users, skills, meId, animate) {
  const n = users.length, cx = 160, cy = 150, R = n === 2 ? 100 : 86, nr = 22;
  const pts = users.map((u, i) => {
    const a = (n === 2 ? Math.PI : -Math.PI / 2) + (2 * Math.PI * i) / n;
    return { x: cx + R * Math.cos(a), y: cy + R * Math.sin(a), a, u };
  });
  let edges = '', nodes = '';
  pts.forEach((p, i) => {
    const q = pts[(i + 1) % n];
    const dx = q.x - p.x, dy = q.y - p.y, len = Math.hypot(dx, dy);
    const k = (n === 2 ? 0.46 : 0.16) * len;
    const c = { x: (p.x + q.x) / 2 + (dy / len) * k, y: (p.y + q.y) / 2 - (dx / len) * k };
    const un = (a, b) => { const l = Math.hypot(b.x - a.x, b.y - a.y) || 1; return { x: (b.x - a.x) / l, y: (b.y - a.y) / l }; };
    const s0 = un(p, c), s1 = un(q, c);
    const sx = p.x + s0.x * (nr + 3), sy = p.y + s0.y * (nr + 3);
    const ex = q.x + s1.x * (nr + 7), ey = q.y + s1.y * (nr + 7);
    const mx = 0.25 * p.x + 0.5 * c.x + 0.25 * q.x, my = 0.25 * p.y + 0.5 * c.y + 0.25 * q.y;
    const label = skills[i], w = label.length * 6.6 + 16;
    const ang = Math.atan2(ey - c.y, ex - c.x), ah = 8;
    const hx1 = ex - ah * Math.cos(ang - 0.45), hy1 = ey - ah * Math.sin(ang - 0.45);
    const hx2 = ex - ah * Math.cos(ang + 0.45), hy2 = ey - ah * Math.sin(ang + 0.45);
    edges += `<g class="eg" style="animation-delay:${0.25 * i + 0.2}s">
      <path class="edge" pathLength="1" style="animation-delay:${0.25 * i + 0.2}s" d="M${sx.toFixed(1)} ${sy.toFixed(1)} Q${c.x.toFixed(1)} ${c.y.toFixed(1)} ${ex.toFixed(1)} ${ey.toFixed(1)}"/>
      <polygon points="${ex.toFixed(1)},${ey.toFixed(1)} ${hx1.toFixed(1)},${hy1.toFixed(1)} ${hx2.toFixed(1)},${hy2.toFixed(1)}" fill="var(--teach)"/>
      <rect class="pill" x="${(mx - w / 2).toFixed(1)}" y="${(my - 10).toFixed(1)}" width="${w.toFixed(1)}" height="20" rx="10"/>
      <text x="${mx.toFixed(1)}" y="${(my + 4).toFixed(1)}" text-anchor="middle" font-size="11.5" font-weight="600" fill="#08705f">${esc(label)}</text></g>`;
  });
  pts.forEach(p => {
    const isMe = p.u.id === meId;
    const init = esc(p.u.name.trim().slice(0, 2).toUpperCase());
    let lx = p.x + (nr + 12) * Math.cos(p.a), ly = p.y + (nr + 12) * Math.sin(p.a) + 4;
    const cs = Math.cos(p.a);
    let anchor = cs > 0.3 ? 'start' : cs < -0.3 ? 'end' : 'middle';
    if (n === 2) { lx = p.x; ly = p.y + nr + 16; anchor = 'middle'; }
    nodes += `<circle class="${isMe ? 'node-me' : 'node'}" cx="${p.x.toFixed(1)}" cy="${p.y.toFixed(1)}" r="${nr}"/>
      <text x="${p.x.toFixed(1)}" y="${(p.y + 4.5).toFixed(1)}" text-anchor="middle" font-size="13" font-weight="700" fill="${isMe ? '#fff' : 'var(--ink)'}">${init}</text>
      <text x="${lx.toFixed(1)}" y="${ly.toFixed(1)}" text-anchor="${anchor}" font-size="12.5" font-weight="${isMe ? 700 : 500}" fill="var(--ink)">${esc(isMe ? 'You' : p.u.name)}</text>`;
  });
  const desc = users.map((u, i) => `${u.name} teaches ${skills[i]} to ${users[(i + 1) % n].name}`).join(', ');
  return `<div class="ring${animate ? ' reveal' : ''}"><svg viewBox="0 0 320 300" role="img" aria-label="${esc(desc)}">${edges}${nodes}</svg></div>`;
}

function matchCard(m, opts = {}) {
  const n = m.users.length;
  const lines = m.users.map((u, i) => {
    const nx = m.users[(i + 1) % n];
    const nm = x => (x.id === (me && me.id) ? 'You' : esc(x.name));
    return `<li>${nm(u)} ${u.id === (me && me.id) ? 'teach' : 'teaches'} <span class="chip teach">${esc(m.skills[i])}</span> to ${nm(nx)}</li>`;
  }).join('');
  const meta = n === 2 ? 'Direct swap' : `Chain of ${n} people`;
  const action = opts.send ? `<button class="btn sm" data-ids="${m.users.map(u => u.id).join(',')}">Send swap request</button>` : '';
  const status = opts.status ? `<span class="status">${esc(opts.status)}</span>` : '';
  return `<article class="match">${ringSvg(m.users, m.skills, me && me.id, opts.animate)}
    <div><div class="meta">${meta} ${status}</div><ul>${lines}</ul>${action}</div></article>`;
}
function bindSend(root) {
  root.querySelectorAll('button[data-ids]').forEach(b => b.onclick = async () => {
    b.disabled = true;
    try { const r = await api('/api/requests', { ids: b.dataset.ids }); toast(`Request #${r.id} sent to ${r.match.users.length} people`); location.hash = '#/requests'; }
    catch (e) { toast(e.message, true); b.disabled = false; }
  });
}

/* ---------- views ---------- */
async function landing() {
  const demo = { users: [{ id: 1, name: 'Aarav' }, { id: 2, name: 'Diya' }, { id: 3, name: 'Kabir' }], skills: ['python', 'sql', 'photoshop'] };
  const pair = { users: [{ id: 1, name: 'Aarav' }, { id: 2, name: 'Diya' }], skills: ['python', 'react'] };
  app.innerHTML = `
  <section class="hero">
    <div>
      <h1>Teach what you know. Learn what you want.</h1>
      <p class="lead">List the skills you can teach and the ones you want to learn. Skill Sync finds students to swap with, even when it takes three or four people to make the trade work.</p>
      <div class="row">${me ? '<a class="btn" href="#/dashboard">Open dashboard</a>' : '<a class="btn" href="#/auth?new">Create your profile</a><a class="btn ghost" href="#/auth">Sign in</a>'}</div>
      <p class="livebar" id="live"></p>
    </div>
    <div>${ringSvg(demo.users, demo.skills, 0, true)}</div>
  </section>
  <section class="how">
    <div>${ringSvg(pair.users, pair.skills, 0)}<h3>Direct swap</h3><p>Two students who each teach what the other wants to learn.</p></div>
    <div>${ringSvg(demo.users, demo.skills, 0)}<h3>Chain swap</h3><p>When no direct match exists, we look for a loop through three or more people so everyone still gets a skill.</p></div>
    <div><h3>Request and notify</h3><p style="margin-bottom:1rem">Pick a swap and send one request. Everyone in the loop gets a notification.</p>${chips(['python', 'sql', 'photoshop'], 'teach')}<p class="note" style="margin-top:.8rem">Teal means you can teach it, amber means you want it.</p></div>
  </section>`;
  try {
    const s = await api('/api/stats');
    $('#live').textContent = `${s.users} students listed, ${s.edges} ways someone can teach someone else right now.`;
  } catch { /* backend offline: skip */ }
}

function auth() {
  const startNew = location.hash.includes('new');
  app.innerHTML = `<div class="panel narrow">
    <div class="tabs" role="tablist"><button id="t-in" role="tab">Sign in</button><button id="t-up" role="tab">Create account</button></div>
    <form id="f"><div id="fields"></div><p class="error" id="err" role="alert"></p><button class="btn" id="go" style="margin-top:.5rem"></button></form></div>`;
  let mode = startNew ? 'up' : 'in', getTeach, getLearn;
  const draw = () => {
    $('#t-in').className = mode === 'in' ? 'on' : ''; $('#t-up').className = mode === 'up' ? 'on' : '';
    $('#go').textContent = mode === 'in' ? 'Sign in' : 'Create account';
    $('#err').textContent = '';
    $('#fields').innerHTML = `<label for="name">Your name</label><input type="text" id="name" autocomplete="name" required>` +
      (mode === 'up' ? `<label>Skills you can teach</label><div id="teach"></div><label>Skills you want to learn</label><div id="learn"></div><p class="hint">Press Enter after each skill. You can edit these later.</p>` : '');
    if (mode === 'up') { getTeach = tagInput($('#teach'), [], 'teach'); getLearn = tagInput($('#learn'), [], 'learn'); }
    $('#name').focus();
  };
  $('#t-in').onclick = () => { mode = 'in'; draw(); };
  $('#t-up').onclick = () => { mode = 'up'; draw(); };
  $('#f').onsubmit = async e => {
    e.preventDefault();
    try {
      const body = { name: $('#name').value };
      if (mode === 'up') { body.teach = getTeach(); body.learn = getLearn(); }
      setMe(await api(mode === 'up' ? '/api/register' : '/api/login', body));
      location.hash = '#/dashboard';
    } catch (er) { $('#err').textContent = er.message; }
  };
  draw();
}

async function dashboard() {
  const q = t => api(`/api/matches?user=${me.id}&type=${t}`);
  const [direct, multi, shortest, reqs] = await Promise.all([q('direct'), q('multi'), q('shortest'), api(`/api/requests?user=${me.id}`)]);
  const best = direct[0] || shortest[0];
  app.innerHTML = `
    <div class="pagehead"><h1 style="font-size:clamp(1.8rem,4vw,2.6rem)">Hi, ${esc(me.name)}</h1></div>
    <div class="stats">
      <div><b>${direct.length}</b><span>direct swaps</span></div>
      <div><b>${multi.length}</b><span>chain swaps</span></div>
      <div><b>${reqs.length}</b><span>requests sent or received</span></div>
    </div>
    <section><h2 style="margin-bottom:1rem">Your closest swap</h2>
      ${best ? matchCard(best, { send: true, animate: true }) : `<div class="empty">No swaps yet. Add more skills you can teach or want to learn on your <a href="#/profile">profile</a>, or <a href="#/search">browse what others teach</a>.</div>`}
    </section>
    <section class="cols">
      <div><h3 style="margin-bottom:.6rem">You can teach</h3>${chips(me.teach, 'teach')}</div>
      <div><h3 style="margin-bottom:.6rem">You want to learn</h3>${chips(me.learn, 'learn')}</div>
    </section>`;
  bindSend(app);
}

async function search() {
  app.innerHTML = `<div class="pagehead"><h1 style="font-size:clamp(1.8rem,4vw,2.6rem)">Find skills</h1><p>Type the start of a skill name, or leave it empty to browse everything students teach.</p></div>
    <div class="panel"><input type="text" id="q" placeholder="e.g. py" aria-label="Search skills" autocomplete="off"><div id="res" style="margin-top:.6rem"></div></div>`;
  const run = async () => {
    const list = await api('/api/search?q=' + encodeURIComponent($('#q').value));
    $('#res').innerHTML = list.length ? list.map(r => {
      const want = me.learn.includes(r.skill), mine = me.teach.includes(r.skill);
      return `<div class="skillrow"><span class="chip teach">${esc(r.skill)}</span>
        ${want ? '<span class="chip learn">On your learn list</span>' : ''}${mine ? '<span class="note">You teach this</span>' : ''}
        <div class="who">${r.teachers.map(t => `<span>${esc(t.name)}</span>`).join('')}</div></div>`;
    }).join('') : '<div class="empty">No skill starts with that. Try fewer letters.</div>';
  };
  let timer; $('#q').oninput = () => { clearTimeout(timer); timer = setTimeout(run, 180); };
  $('#q').focus(); run();
}

async function matches() {
  app.innerHTML = `<div class="pagehead"><h1 style="font-size:clamp(1.8rem,4vw,2.6rem)">Matches</h1><p>Every swap below works for everyone in it: each person teaches something the next person wants.</p></div>
    <div class="tabs"><button data-t="direct">Direct</button><button data-t="multi">Chains</button><button data-t="shortest">Shortest loop</button></div><div id="list"></div>`;
  const load = async t => {
    app.querySelectorAll('.tabs button').forEach(b => b.className = b.dataset.t === t ? 'on' : '');
    const ms = await api(`/api/matches?user=${me.id}&type=${t}`);
    $('#list').innerHTML = ms.length ? ms.map(m => matchCard(m, { send: true })).join('') :
      `<div class="empty">${t === 'direct' ? 'No one is a direct match yet.' : 'No loops found.'} Add skills on your <a href="#/profile">profile</a> to open up more swaps.</div>`;
    bindSend($('#list'));
  };
  app.querySelectorAll('.tabs button').forEach(b => b.onclick = () => load(b.dataset.t));
  load('direct');
}

async function requests() {
  const [reqs, notes] = await Promise.all([api(`/api/requests?user=${me.id}`), api(`/api/notifications?user=${me.id}`)]);
  app.innerHTML = `<div class="pagehead"><h1 style="font-size:clamp(1.8rem,4vw,2.6rem)">Requests</h1></div>
    <section><h2 style="margin-bottom:1rem">Swap requests</h2>${reqs.length ? reqs.slice().reverse().map(r => matchCard(r.match, { status: `Request #${r.id}, ${r.status.toLowerCase()}` })).join('') : '<div class="empty">No requests yet. Pick a swap on the <a href="#/matches">Matches</a> page.</div>'}</section>
    <section><h2 style="margin-bottom:1rem">Notifications</h2>${notes.length ? `<ul class="notes">${notes.slice().reverse().map(n => `<li>${esc(n)}</li>`).join('')}</ul>` : '<div class="empty">Nothing new.</div>'}</section>`;
}

function profile() {
  app.innerHTML = `<div class="pagehead"><h1 style="font-size:clamp(1.8rem,4vw,2.6rem)">Your profile</h1><p>${esc(me.name)}</p></div>
    <form class="panel" id="f" style="max-width:640px"><label>Skills you can teach</label><div id="teach"></div>
    <label>Skills you want to learn</label><div id="learn"></div><p class="hint">Changes update your matches straight away.</p>
    <p class="error" id="err" role="alert"></p><button class="btn">Save changes</button></form>`;
  const gt = tagInput($('#teach'), me.teach, 'teach'), gl = tagInput($('#learn'), me.learn, 'learn');
  $('#f').onsubmit = async e => {
    e.preventDefault();
    try { setMe(await api('/api/profile', { id: me.id, teach: gt(), learn: gl() })); toast('Changes saved'); }
    catch (er) { $('#err').textContent = er.message; }
  };
}

/* ---------- router + nav ---------- */
const routes = { '/': landing, '/auth': auth, '/dashboard': dashboard, '/search': search, '/matches': matches, '/requests': requests, '/profile': profile };
const secured = new Set(['/dashboard', '/search', '/matches', '/requests', '/profile']);

async function renderNav(path) {
  const nav = $('#nav');
  if (!me) { nav.innerHTML = `<a href="#/auth" class="${path === '/auth' ? 'on' : ''}">Sign in</a>`; return; }
  const links = [['/dashboard', 'Dashboard'], ['/search', 'Find skills'], ['/matches', 'Matches'], ['/requests', 'Requests'], ['/profile', 'Profile']];
  nav.innerHTML = links.map(([p, t]) => `<a href="#${p}" class="${p === path ? 'on' : ''}">${t}${p === '/requests' ? '<span class="badge" id="bd" hidden></span>' : ''}</a>`).join('') +
    `<a href="#/" id="out">Sign out</a>`;
  $('#out').onclick = () => setMe(null);
  try { const n = await api(`/api/notifications?user=${me.id}`); if (n.length) { const b = $('#bd'); b.textContent = n.length; b.hidden = false; } } catch { }
}

async function route() {
  const path = (location.hash.slice(1) || '/').split('?')[0];
  if (me) { // keep the saved profile in sync with the server
    try { const all = await api('/api/users'); const fresh = all.find(u => u.id === me.id); fresh ? setMe(fresh) : setMe(null); } catch { }
  }
  if (secured.has(path) && !me) { location.hash = '#/auth'; return; }
  if (path === '/auth' && me) { location.hash = '#/dashboard'; return; }
  renderNav(path);
  try { await (routes[path] || landing)(); }
  catch (e) { app.innerHTML = `<div class="empty">Could not reach the server. Make sure <b>./server</b> is running, then refresh.</div>`; }
  window.scrollTo(0, 0);
}
addEventListener('hashchange', route);
route();
