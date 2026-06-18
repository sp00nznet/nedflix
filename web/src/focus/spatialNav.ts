// Spatial focus engine — geometry scoring ported verbatim from the prototype `moveFocus`.
// Scores every visible focusable by primary-axis distance + 1.8x cross-axis penalty,
// inside a ~50px tolerance cone, and returns the best candidate in `dir`.

export type Dir = 'up' | 'down' | 'left' | 'right';

export function focusables(): HTMLElement[] {
  // If a modal sets [data-focus-trap], confine navigation to within it.
  const trap = document.querySelector<HTMLElement>('[data-focus-trap]');
  const root: ParentNode = trap ?? document;
  return Array.prototype.slice
    .call(root.querySelectorAll<HTMLElement>('[data-focusable]'))
    .filter((el: HTMLElement) => el.offsetParent !== null);
}

/** Focus the first non-rail focusable (rail = inside <nav>). */
export function focusFirst(): void {
  const f = focusables();
  if (!f.length) return;
  const main = f.find((el) => !el.closest('nav'));
  (main || f[0]).focus();
}

/** Move focus in `dir`. Returns true if focus moved. */
export function moveFocus(dir: Dir): boolean {
  const list = focusables();
  if (!list.length) return false;
  const cur = document.activeElement as HTMLElement | null;
  if (!cur || cur.getAttribute('data-focusable') === null) {
    list[0].focus();
    return true;
  }
  const r = cur.getBoundingClientRect();
  const cx = r.left + r.width / 2;
  const cy = r.top + r.height / 2;
  let best: HTMLElement | null = null;
  let bestScore = Infinity;
  for (const el of list) {
    if (el === cur) continue;
    const b = el.getBoundingClientRect();
    const bx = b.left + b.width / 2;
    const by = b.top + b.height / 2;
    const dx = bx - cx;
    const dy = by - cy;
    let ok = false;
    let primary = 0;
    if (dir === 'left') {
      ok = dx < -6 && Math.abs(dy) <= Math.abs(dx) + 50;
      primary = -dx;
    } else if (dir === 'right') {
      ok = dx > 6 && Math.abs(dy) <= Math.abs(dx) + 50;
      primary = dx;
    } else if (dir === 'up') {
      ok = dy < -6 && Math.abs(dx) <= Math.abs(dy) + 50;
      primary = -dy;
    } else if (dir === 'down') {
      ok = dy > 6 && Math.abs(dx) <= Math.abs(dy) + 50;
      primary = dy;
    }
    if (!ok) continue;
    const cross = dir === 'left' || dir === 'right' ? Math.abs(dy) : Math.abs(dx);
    const score = primary + cross * 1.8;
    if (score < bestScore) {
      bestScore = score;
      best = el;
    }
  }
  if (best) {
    best.focus();
    // keep the newly focused element in view (rails scroll horizontally, grids vertically)
    best.scrollIntoView({ block: 'nearest', inline: 'nearest' });
    return true;
  }
  return false;
}
