import { api } from './http';

// Window controls for the frameless Electron shell (no-ops on plain web).
export const isDesktop = (): boolean => typeof window !== 'undefined' && 'nedflixDesktop' in window;
export const minimizeApp = () => api.post('/api/app/minimize').catch(() => {});
export const toggleMaximizeApp = () => api.post('/api/app/toggle-maximize').catch(() => {});
export const quitApp = () => api.post('/api/app/quit').catch(() => {});

// CSS for Electron window dragging (typed escape hatch — not in CSSProperties).
export const dragRegion = { WebkitAppRegion: 'drag' } as React.CSSProperties;
export const noDragRegion = { WebkitAppRegion: 'no-drag' } as React.CSSProperties;
