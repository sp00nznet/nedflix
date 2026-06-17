import { useLocation, useNavigate } from 'react-router-dom';
import { colors, font } from '../theme';
import { MOBILE_TABS, type NavItemDef } from './nav';

function active(pathname: string, item: NavItemDef): boolean {
  if (item.path === '/') return pathname === '/';
  return pathname === item.path || pathname.startsWith(item.path + '/');
}

/** Mobile bottom tab bar (<768px). Padded with the safe-area inset. */
export function BottomTabBar() {
  const navigate = useNavigate();
  const { pathname } = useLocation();
  return (
    <nav
      style={{
        position: 'fixed',
        left: 0,
        right: 0,
        bottom: 0,
        display: 'flex',
        justifyContent: 'space-around',
        alignItems: 'center',
        height: 'calc(58px + env(safe-area-inset-bottom))',
        paddingBottom: 'env(safe-area-inset-bottom)',
        background: 'rgba(14,15,21,.92)',
        backdropFilter: 'blur(14px)',
        borderTop: '1px solid rgba(236,239,247,.08)',
        zIndex: 40,
      }}
    >
      {MOBILE_TABS.map((item, i) => {
        const on = active(pathname, item);
        return (
          <button
            key={`${item.id}-${i}`}
            data-focusable
            tabIndex={0}
            onClick={() => navigate(item.path)}
            style={{
              flex: 1,
              height: '100%',
              border: 0,
              background: 'none',
              cursor: 'pointer',
              color: on ? 'var(--ac)' : colors.navInactive,
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              justifyContent: 'center',
              gap: 4,
              touchAction: 'manipulation',
            }}
          >
            {item.icon}
            <span style={{ font: `600 10px/1 ${font.ui}` }}>{item.label}</span>
          </button>
        );
      })}
    </nav>
  );
}
