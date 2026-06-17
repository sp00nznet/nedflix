// Active-profile context: holds the current profile and applies its accent to --ac on
// the app root. Backed by localStorage until the profiles API lands (Phase 5).
import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useMemo,
  useState,
  type ReactNode,
} from 'react';
import { ACCENTS, DEFAULT_ACCENT, type AccentName } from '../theme';

export interface Profile {
  id: string;
  name: string;
  initial: string;
  color: string; // avatar color (may be 'var(--ac)')
  accent: AccentName;
}

const FALLBACK: Profile = {
  id: 'default',
  name: 'Ned',
  initial: 'N',
  color: 'var(--ac)',
  accent: DEFAULT_ACCENT,
};

interface Ctx {
  profile: Profile;
  setProfile: (p: Profile) => void;
}
const ProfileCtx = createContext<Ctx>({ profile: FALLBACK, setProfile: () => {} });

export function ActiveProfileProvider({ children }: { children: ReactNode }) {
  const [profile, setProfileState] = useState<Profile>(() => {
    try {
      const raw = localStorage.getItem('marquee-active-profile');
      if (raw) return { ...FALLBACK, ...JSON.parse(raw) };
    } catch {
      /* ignore */
    }
    return FALLBACK;
  });

  const setProfile = useCallback((p: Profile) => {
    setProfileState(p);
    try {
      localStorage.setItem('marquee-active-profile', JSON.stringify(p));
    } catch {
      /* ignore */
    }
  }, []);

  // Apply accent → --ac on the root element.
  useEffect(() => {
    const root = document.getElementById('marquee-root');
    const hex = ACCENTS[profile.accent] ?? ACCENTS[DEFAULT_ACCENT];
    if (root) root.style.setProperty('--ac', hex);
  }, [profile.accent]);

  const value = useMemo(() => ({ profile, setProfile }), [profile, setProfile]);
  return <ProfileCtx.Provider value={value}>{children}</ProfileCtx.Provider>;
}

export const useActiveProfile = () => useContext(ProfileCtx);
