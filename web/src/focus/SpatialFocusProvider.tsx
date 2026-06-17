import { useEffect, type ReactNode } from 'react';
import { useLocation, useNavigate } from 'react-router-dom';
import { moveFocus, focusFirst, type Dir } from './spatialNav';
import { startGamepad } from './gamepad';

/**
 * Global key/controller handler for the 10-foot UI.
 *  arrows → moveFocus · Enter → click focused · Esc/Backspace → back ·
 *  Space → toggle play (on /watch). Resets focus to the first non-rail element on
 *  every route change (double-rAF). Gamepad polling feeds the same key path.
 */
export function SpatialFocusProvider({ children }: { children: ReactNode }) {
  const navigate = useNavigate();
  const location = useLocation();

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      const k = e.key;
      // Don't hijack typing in inputs/textareas.
      const target = e.target as HTMLElement | null;
      const typing =
        target &&
        (target.tagName === 'INPUT' ||
          target.tagName === 'TEXTAREA' ||
          target.isContentEditable);

      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(k)) {
        if (typing) return;
        const dir = k.replace('Arrow', '').toLowerCase() as Dir;
        if (moveFocus(dir)) e.preventDefault();
      } else if (k === 'Enter') {
        const el = document.activeElement as HTMLElement | null;
        if (!typing && el && el.getAttribute('data-focusable') !== null) {
          e.preventDefault();
          el.click();
        }
      } else if (k === 'Backspace' || k === 'Escape') {
        if (typing && k === 'Backspace') return;
        e.preventDefault();
        if (location.pathname !== '/') navigate(-1);
      } else if (k === ' ' && location.pathname.startsWith('/watch')) {
        e.preventDefault();
        window.dispatchEvent(new CustomEvent('marquee:toggleplay'));
      }
    };
    window.addEventListener('keydown', onKey);
    const stopPad = startGamepad();
    return () => {
      window.removeEventListener('keydown', onKey);
      stopPad();
    };
  }, [navigate, location.pathname]);

  // Reset focus to first non-rail focusable on every navigation.
  useEffect(() => {
    const r1 = requestAnimationFrame(() => {
      const r2 = requestAnimationFrame(() => focusFirst());
      (focusFirst as unknown as { _r2?: number })._r2 = r2;
    });
    return () => cancelAnimationFrame(r1);
  }, [location.pathname, location.search]);

  return <>{children}</>;
}
