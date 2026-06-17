import { Outlet, useLocation } from 'react-router-dom';
import { colors } from '../theme';
import { NavRail } from './NavRail';
import { BottomTabBar } from './BottomTabBar';
import { DpadHint } from './DpadHint';
import { WindowControls } from './WindowControls';
import { useIsMobile } from './useIsMobile';

/**
 * Root layout: nav rail (desktop) or bottom tab bar (mobile) + scrolling main.
 * The video player and the profiles gate render full-bleed (no chrome).
 */
export function AppShell() {
  const { pathname } = useLocation();
  const isMobile = useIsMobile();
  const fullBleed = pathname.startsWith('/watch') || pathname === '/profiles';

  if (fullBleed) {
    return (
      <div id="marquee-root" style={{ height: '100dvh', width: '100%', background: colors.bg0 }}>
        <WindowControls />
        <Outlet />
      </div>
    );
  }

  return (
    <div
      id="marquee-root"
      style={{
        display: 'flex',
        height: '100dvh',
        width: '100%',
        background: colors.bg0,
        overflow: 'hidden',
      }}
    >
      <WindowControls />
      {!isMobile && <NavRail />}
      <main
        style={{
          flex: 1,
          height: '100%',
          overflowY: 'auto',
          overflowX: 'hidden',
          paddingBottom: isMobile ? 'calc(58px + env(safe-area-inset-bottom))' : 0,
        }}
      >
        <Outlet />
      </main>
      {isMobile ? <BottomTabBar /> : <DpadHint />}
    </div>
  );
}
