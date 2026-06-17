# Marquee — brand assets

The Marquee mark: a lowercase **m** monogram on a dark "theatre-tile," with a coral **bulb dot**
(the marquee light) at the upper right. Accent coral is **#fb7159**; tile is a dark vertical
gradient (#1b1e29 → #0d0e14) on ink **#0b0c11**. Wordmark is lowercase `marquee` followed by the
coral bulb dot. Type: **Schibsted Grotesk** (display), **JetBrains Mono** (technical).

## Files
| File | Use |
|---|---|
| `icon.svg` | Master, scalable. Outline the `m` before shipping if the font isn't guaranteed. |
| `icon-512.png` / `icon-192.png` | PWA / web app icons (transparent rounded corners). |
| `icon-maskable-512.png` | Android **maskable** icon (full-bleed; mark inside the safe zone). |
| `apple-touch-icon-180.png` | iOS home screen (full-bleed; iOS applies its own mask). |
| `favicon-32.png` / `favicon-16.png` | Browser tab (border dropped, flat dark tile for contrast). |
| `icon-inverse-512.png` | Coral-on-dark inverse, for light surfaces / stickers. |
| `og-image.png` | 1200×630 social / link-preview card. |

## Drop-in `<head>`
```html
<link rel="icon" type="image/png" sizes="32x32" href="/brand/favicon-32.png">
<link rel="icon" type="image/png" sizes="16x16" href="/brand/favicon-16.png">
<link rel="apple-touch-icon" sizes="180x180" href="/brand/apple-touch-icon-180.png">
<link rel="manifest" href="/brand/manifest.webmanifest">
<meta name="theme-color" content="#0b0c11">
<meta property="og:image" content="/brand/og-image.png">
<meta property="og:title" content="Marquee">
<meta property="og:description" content="Your library, now showing.">
```

## Don'ts
- Don't recolor the bulb dot or add a gradient to it — coral is the one constant.
- Don't capitalize or letter-space the wordmark; it's lowercase with the dot on the baseline-right.
- Don't place the dark mark on a busy photo without the tile behind it.

> PNGs were rasterized with a grotesk fallback for the `m`. For pixel-canonical output, re-export
> from `icon.svg` with **Schibsted Grotesk** installed (or after outlining the glyph).
