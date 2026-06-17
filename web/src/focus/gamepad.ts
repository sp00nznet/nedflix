// Gamepad → keyboard bridge. Polls navigator.getGamepads() and synthesizes the same
// arrow/Enter/Escape KeyboardEvents the SpatialFocusProvider already listens for, so
// remote, keyboard, and controller all share one code path.
//   D-pad / left stick → Arrow keys · A (0) → Enter · B (1) → Escape

const REPEAT_MS = 160; // debounce held directions

type Btn = { key: string; pressed: boolean; last: number };

export function startGamepad(): () => void {
  if (typeof navigator === 'undefined' || !('getGamepads' in navigator)) {
    return () => {};
  }
  let raf = 0;
  const state: Record<string, Btn> = {
    up: { key: 'ArrowUp', pressed: false, last: 0 },
    down: { key: 'ArrowDown', pressed: false, last: 0 },
    left: { key: 'ArrowLeft', pressed: false, last: 0 },
    right: { key: 'ArrowRight', pressed: false, last: 0 },
    a: { key: 'Enter', pressed: false, last: 0 },
    b: { key: 'Escape', pressed: false, last: 0 },
  };

  const fire = (key: string) => {
    window.dispatchEvent(new KeyboardEvent('keydown', { key, bubbles: true }));
  };

  const edge = (name: string, isDown: boolean, now: number, repeat: boolean) => {
    const b = state[name];
    if (isDown) {
      const due = !b.pressed || (repeat && now - b.last > REPEAT_MS);
      if (due) {
        fire(b.key);
        b.last = now;
      }
      b.pressed = true;
    } else {
      b.pressed = false;
    }
  };

  const poll = () => {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    const now = performance.now();
    for (const gp of pads) {
      if (!gp) continue;
      const ax = gp.axes[0] ?? 0;
      const ay = gp.axes[1] ?? 0;
      const dpadUp = gp.buttons[12]?.pressed || ay < -0.5;
      const dpadDown = gp.buttons[13]?.pressed || ay > 0.5;
      const dpadLeft = gp.buttons[14]?.pressed || ax < -0.5;
      const dpadRight = gp.buttons[15]?.pressed || ax > 0.5;
      edge('up', !!dpadUp, now, true);
      edge('down', !!dpadDown, now, true);
      edge('left', !!dpadLeft, now, true);
      edge('right', !!dpadRight, now, true);
      edge('a', !!gp.buttons[0]?.pressed, now, false);
      edge('b', !!gp.buttons[1]?.pressed, now, false);
      break; // first connected pad wins
    }
    raf = requestAnimationFrame(poll);
  };
  raf = requestAnimationFrame(poll);
  return () => cancelAnimationFrame(raf);
}
