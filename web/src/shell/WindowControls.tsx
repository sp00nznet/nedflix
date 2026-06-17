import { colors } from '../theme';
import { isDesktop, minimizeApp, toggleMaximizeApp, quitApp, dragRegion, noDragRegion } from '../api/desktop';

// Frameless-window chrome for the desktop shell: a thin draggable strip across the very
// top (won't block content) + a draggable cluster with minimize / maximize / close at the
// top-right. Renders nothing on plain web.
export function WindowControls() {
  if (!isDesktop()) return null;

  const Btn = ({ onClick, title, danger, children }: { onClick: () => void; title: string; danger?: boolean; children: React.ReactNode }) => (
    <button
      onClick={onClick}
      title={title}
      aria-label={title}
      style={{ ...noDragRegion, width: 44, height: 30, border: 0, background: 'transparent', color: colors.ink3, cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
      onMouseEnter={(e) => { e.currentTarget.style.background = danger ? '#e0484d' : 'rgba(236,239,247,.12)'; e.currentTarget.style.color = '#fff'; }}
      onMouseLeave={(e) => { e.currentTarget.style.background = 'transparent'; e.currentTarget.style.color = colors.ink3; }}
    >
      {children}
    </button>
  );

  return (
    <>
      {/* thin drag handle along the top edge — does not cover interactive content */}
      <div style={{ ...dragRegion, position: 'fixed', top: 0, left: 0, right: 0, height: 8, zIndex: 999 }} />
      {/* window controls, top-right; the cluster background is also draggable */}
      <div style={{ ...dragRegion, position: 'fixed', top: 0, right: 0, height: 30, display: 'flex', alignItems: 'center', zIndex: 1000, paddingLeft: 24, background: 'linear-gradient(90deg,transparent,rgba(11,12,17,.5))', borderBottomLeftRadius: 10 }}>
        <Btn onClick={() => minimizeApp()} title="Minimize"><svg width="11" height="11" viewBox="0 0 12 12"><rect x="1" y="5.5" width="10" height="1" fill="currentColor" /></svg></Btn>
        <Btn onClick={() => toggleMaximizeApp()} title="Maximize"><svg width="11" height="11" viewBox="0 0 12 12" fill="none" stroke="currentColor"><rect x="1.5" y="1.5" width="9" height="9" /></svg></Btn>
        <Btn onClick={() => quitApp()} title="Close" danger><svg width="11" height="11" viewBox="0 0 12 12" stroke="currentColor" strokeWidth="1.3"><path d="M2 2 L10 10 M10 2 L2 10" /></svg></Btn>
      </div>
    </>
  );
}
