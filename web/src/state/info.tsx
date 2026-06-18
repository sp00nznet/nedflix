import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from 'react';
import { InfoOverlay } from '../components/InfoOverlay';

// An item the "More info" overlay can describe. `kind` drives ffprobe + artwork search.
export interface InfoItem {
  id: string;
  path?: string;
  kind: 'movie' | 'tv' | 'series' | 'music' | 'book' | 'channel';
  title: string;
  subtitle?: string;
  artUrl?: string;
}

interface InfoCtx {
  item: InfoItem | null;
  open: (i: InfoItem) => void;
  close: () => void;
}
const Ctx = createContext<InfoCtx>({ item: null, open: () => {}, close: () => {} });

export const useInfo = () => useContext(Ctx);

/** Spread onto a focusable card to make it info-able (right-click, or the `i` key /
 *  controller button when focused). */
export function infoProps(item: InfoItem, open: (i: InfoItem) => void) {
  return {
    'data-info': JSON.stringify(item),
    onContextMenu: (e: React.MouseEvent) => { e.preventDefault(); open(item); },
  } as const;
}

/** Build an InfoItem from a library title-ish object. */
export function titleToInfo(t: { id: string; path?: string; type?: string; title: string; year?: number; genre?: string; posterUrl?: string }): InfoItem {
  return {
    id: t.id,
    path: t.path,
    kind: t.type === 'Series' ? 'series' : 'movie',
    title: t.title,
    subtitle: [t.year, t.genre].filter(Boolean).join(' · ') || undefined,
    artUrl: t.posterUrl,
  };
}

export function InfoProvider({ children }: { children: ReactNode }) {
  const [item, setItem] = useState<InfoItem | null>(null);
  const open = useCallback((i: InfoItem) => setItem(i), []);
  const close = useCallback(() => setItem(null), []);

  // `i` (and the controller button mapped to it) opens info for the focused card.
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key !== 'i' && e.key !== 'I') return;
      const el = document.activeElement as HTMLElement | null;
      if (!el || el.tagName === 'INPUT' || el.tagName === 'TEXTAREA') return;
      const d = el.getAttribute && el.getAttribute('data-info');
      if (!d) return;
      e.preventDefault();
      try { open(JSON.parse(d)); } catch { /* ignore */ }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [open]);

  const value = useMemo(() => ({ item, open, close }), [item, open, close]);
  return (
    <Ctx.Provider value={value}>
      {children}
      {item && <InfoOverlay item={item} onClose={close} />}
    </Ctx.Provider>
  );
}
