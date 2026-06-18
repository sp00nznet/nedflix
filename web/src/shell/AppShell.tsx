import { Outlet, useLocation } from 'react-router-dom';
import { colors } from '../theme';
import { NavRail } from './NavRail';
import { BottomTabBar } from './BottomTabBar';
import { DpadHint } from './DpadHint';
import { WindowControls } from './WindowControls';
import { MiniPlayer } from '../components/MiniPlayer';
import { useAudioPlayer } from '../state/audioPlayer';
import { useIsMobile } from './useIsMobile';

/**
 * Root layout: nav rail (desktop) or bottom tab bar (mobile) + scrolling main.
 * The video player and the profiles gate render full-bleed (no chrome).
 */
export function AppShell() {
  const { pathname } = useLocation();
  const isMobile = useIsMobile();
  const { current } = useAudioPlayer();
  const fullBleed = pathname.startsWith('/watch') || pathname === '/profiles';
  const miniShown = !!current && !fullBleed;

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
          paddingBottom: `calc(${isMobile ? '58px + env(safe-area-inset-bottom)' : '0px'} + ${miniShown ? '64px' : '0px'})`,
        }}
      >
        <Outlet />
      </main>
      <MiniPlayer />
      {isMobile ? <BottomTabBar /> : !miniShown && <DpadHint />}
    </div>
  );
}
