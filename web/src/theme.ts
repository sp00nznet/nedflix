// Marquee design tokens — ported from design/README.md + the prototype logic class.
// Accent ships as a CSS variable `--ac` set on the app root from the active profile.

export const colors = {
  bg0: '#0b0c11', // app background, knob fill
  bg1: '#101220', // raised panel (player cards)
  bg2a: '#14161f', // nav rail gradient top
  bg2b: '#0e0f15', // nav rail gradient bottom
  panel: '#191c27', // now-playing/player card top
  ink0: '#f6f8fd', // brightest headings
  ink1: '#eceef4', // primary text
  ink2: '#c1c6d3', // secondary text
  ink3: '#8c91a2', // tertiary
  ink4: '#626878', // muted / mono labels
  navInactive: '#7c8193',
  liveRed: '#ec5c4c',
  acInk: '#ffffff', // on-accent text/icons
} as const;

export const hairline = (a = 0.08) => `rgba(236,239,247,${a})`;
export const overlay = (a: number) => `rgba(11,12,17,${a})`;

// Accent presets — `--ac`. Default ships as coral.
export const ACCENTS = {
  coral: '#fb7159',
  iris: '#7378f2',
  violet: '#9a6cf0',
  indigo: '#5a63e0',
  mint: '#23b187',
  blue: '#4f7cf5',
} as const;
export type AccentName = keyof typeof ACCENTS;
export const DEFAULT_ACCENT: AccentName = 'coral';

// Poster/art placeholder gradients (8, 155deg, dark). Keep as loading/empty fallback.
export const GRAD = {
  indigo: 'linear-gradient(155deg,#2b3350 0%,#171a2b 55%,#0c0d15 100%)',
  plum: 'linear-gradient(155deg,#3c2438 0%,#231527 55%,#120b12 100%)',
  rust: 'linear-gradient(155deg,#45301f 0%,#2a1c12 55%,#140d08 100%)',
  forest: 'linear-gradient(155deg,#22361f 0%,#152213 55%,#0b120a 100%)',
  teal: 'linear-gradient(155deg,#1f3b3a 0%,#132423 55%,#0a1312 100%)',
  wine: 'linear-gradient(155deg,#43222a 0%,#28151a 55%,#140a0c 100%)',
  sand: 'linear-gradient(155deg,#3a3326 0%,#241f16 55%,#12100b 100%)',
  slate: 'linear-gradient(155deg,#2c2f38 0%,#191b21 55%,#0c0d10 100%)',
} as const;
export type GradName = keyof typeof GRAD;
export const GRAD_LIST = Object.values(GRAD);

// Deterministic gradient for an id (stable placeholder until real artwork arrives).
export function gradFor(key: string): string {
  let h = 0;
  for (let i = 0; i < key.length; i++) h = (h * 31 + key.charCodeAt(i)) >>> 0;
  return GRAD_LIST[h % GRAD_LIST.length];
}

export const radius = {
  chip: 7,
  button: 12,
  card: 13,
  panel: 18,
  pill: 999,
} as const;

export const shadow = {
  card: '0 12px 30px -14px rgba(0,0,0,.75)',
  cardFocus: (ac = 'var(--ac)') =>
    `0 0 0 2px ${colors.bg0},0 0 0 4px ${ac},0 22px 46px -14px rgba(0,0,0,.85)`,
  accentBtn: '0 12px 30px -12px rgba(251,113,89,.55)',
  focusRing: 'box-shadow:0 0 0 2px var(--ac)',
} as const;

export const font = {
  ui: "'Schibsted Grotesk', system-ui, sans-serif",
  mono: "'JetBrains Mono', ui-monospace, monospace",
} as const;
