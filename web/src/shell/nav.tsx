// Shared nav model — used by both NavRail (desktop) and BottomTabBar (mobile).
import type { ReactNode } from 'react';

export type NavId =
  | 'home'
  | 'films'
  | 'series'
  | 'music'
  | 'books'
  | 'podcasts'
  | 'live'
  | 'search'
  | 'settings';

export interface NavItemDef {
  id: NavId;
  label: string;
  path: string;
  icon: ReactNode;
}

const I = (d: ReactNode) => (
  <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round">
    {d}
  </svg>
);

export const PRIMARY_NAV: NavItemDef[] = [
  { id: 'home', label: 'HOME', path: '/', icon: I(<><path d="M3 11 L12 3 L21 11" /><path d="M5 10 V21 H19 V10" /></>) },
  { id: 'films', label: 'FILMS', path: '/films', icon: I(<><rect x="3" y="4" width="18" height="16" rx="2" /><path d="M3 9 H21 M8 4 V20 M16 4 V20" /></>) },
  { id: 'series', label: 'SERIES', path: '/series', icon: I(<><rect x="3" y="6" width="18" height="13" rx="2" /><path d="M8 3 L12 6 L16 3" /></>) },
  { id: 'music', label: 'MUSIC', path: '/music', icon: I(<><circle cx="7" cy="17" r="3" /><circle cx="18" cy="15" r="3" /><path d="M10 17 V6 L21 4 V15" /></>) },
  { id: 'books', label: 'BOOKS', path: '/audiobooks', icon: I(<><path d="M4 5 a2 2 0 0 1 2-2 h6 v16 H6 a2 2 0 0 0-2 2 Z" /><path d="M20 5 a2 2 0 0 0-2-2 h-6 v16 h6 a2 2 0 0 1 2 2 Z" /></>) },
  { id: 'podcasts', label: 'PODS', path: '/podcasts', icon: I(<><rect x="9" y="3" width="6" height="11" rx="3" /><path d="M5 11 a7 7 0 0 0 14 0 M12 18 v3" /></>) },
  { id: 'live', label: 'LIVE', path: '/live', icon: I(<><rect x="2" y="6" width="20" height="13" rx="2" /><path d="M8 3 L12 6 L16 3" /></>) },
];

export const FOOTER_NAV: NavItemDef[] = [
  { id: 'search', label: 'SEARCH', path: '/search', icon: I(<><circle cx="11" cy="11" r="7" /><path d="M21 21 L16 16" /></>) },
  { id: 'settings', label: 'SETUP', path: '/settings', icon: I(<><circle cx="12" cy="12" r="3" /><path d="M12 2 v3 M12 19 v3 M2 12 h3 M19 12 h3 M5 5 l2 2 M17 17 l2 2 M19 5 l-2 2 M7 17 l-2 2" /></>) },
];

// Mobile bottom tabs (per Marquee Mobile.dc.html): Home, Search, Listen, Live, You.
export const MOBILE_TABS: NavItemDef[] = [
  PRIMARY_NAV[0],
  FOOTER_NAV[0],
  { id: 'music', label: 'Listen', path: '/music', icon: PRIMARY_NAV[3].icon },
  PRIMARY_NAV[5],
  { id: 'settings', label: 'You', path: '/profiles', icon: I(<><circle cx="12" cy="8" r="4" /><path d="M4 21 a8 8 0 0 1 16 0" /></>) },
];
