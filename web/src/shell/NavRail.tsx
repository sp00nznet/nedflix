import { useLocation, useNavigate } from 'react-router-dom';
import { colors, font } from '../theme';
import { useActiveProfile } from '../state/activeProfile';
import { BrandGlyph } from './BrandMark';
import { PRIMARY_NAV, FOOTER_NAV, type NavItemDef } from './nav';
import { dragRegion } from '../api/desktop';

function isActive(pathname: string, item: NavItemDef): boolean {
  if (item.path === '/') return pathname === '/';
  return pathname === item.path || pathname.startsWith(item.path + '/');
}

function RailButton({ item, active, onClick, compact }: { item: NavItemDef; active: boolean; onClick: () => void; compact?: boolean }) {
  return (
    <button
      data-focusable
      tabIndex={0}
      onClick={onClick}
      title={item.label}
      style={{
        position: 'relative',
        width: 74,
        height: compact ? 54 : 60,
        border: 0,
        background: 'none',
        cursor: 'pointer',
        borderRadius: 14,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        gap: 6,
        color: active ? 'var(--ac)' : colors.navInactive,
        transition: 'background .2s,color .2s',
      }}
    >
      {active && !compact && (
        <span
          style={{
            position: 'absolute',
            left: 0,
            top: '50%',
            transform: 'translateY(-50%)',
            width: 3,
            height: 24,
            borderRadius: '0 3px 3px 0',
            background: 'var(--ac)',
          }}
        />
      )}
      {item.icon}
      <span style={{ font: `600 9px/1 ${font.mono}`, letterSpacing: '.12em' }}>{item.label}</span>
    </button>
  );
}

export function NavRail() {
  const navigate = useNavigate();
  const { pathname } = useLocation();
  const { profile } = useActiveProfile();

  return (
    <nav
      style={{
        flex: '0 0 98px',
        height: '100%',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        padding: '18px 0',
        gap: 4,
        background: `linear-gradient(180deg,${colors.bg2a},${colors.bg2b})`,
        borderRight: '1px solid rgba(236,239,247,.07)',
      }}
    >
      {/* brand area doubles as the window drag handle on desktop */}
      <div style={{ ...dragRegion, marginBottom: 14, paddingTop: 8, width: '100%', display: 'flex', justifyContent: 'center' }}>
        <BrandGlyph size={30} />
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
        {PRIMARY_NAV.map((item) => (
          <RailButton key={item.id} item={item} active={isActive(pathname, item)} onClick={() => navigate(item.path)} />
        ))}
      </div>

      <div style={{ flex: 1 }} />

      <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4 }}>
        {FOOTER_NAV.map((item) => (
          <RailButton key={item.id} item={item} active={isActive(pathname, item)} onClick={() => navigate(item.path)} compact />
        ))}
        <button
          data-focusable
          tabIndex={0}
          onClick={() => navigate('/profiles')}
          title={profile.name}
          style={{
            marginTop: 6,
            width: 40,
            height: 40,
            border: 0,
            padding: 0,
            cursor: 'pointer',
            borderRadius: '50%',
            background: profile.color,
            color: colors.bg0,
            font: `700 15px/1 ${font.ui}`,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            boxShadow: '0 0 0 2px rgba(236,239,247,.12)',
          }}
        >
          {profile.initial}
        </button>
      </div>
    </nav>
  );
}
