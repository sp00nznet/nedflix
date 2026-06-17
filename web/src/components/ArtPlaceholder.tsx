import { useState, type CSSProperties } from 'react';
import { font, gradFor } from '../theme';

/**
 * Artwork with a labelled gradient fallback (the loading/empty state for ALL art).
 * Pass a real `src` to show artwork; the gradient stays behind it and on error.
 */
export function ArtPlaceholder({
  label,
  src,
  grad,
  seed,
  style,
}: {
  label?: string;
  src?: string;
  grad?: string;
  seed?: string;
  style?: CSSProperties;
}) {
  const [broken, setBroken] = useState(false);
  const background = grad ?? gradFor(seed ?? label ?? 'art');
  return (
    <div
      style={{
        position: 'absolute',
        inset: 0,
        background,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        ...style,
      }}
    >
      {src && !broken ? (
        <img
          src={src}
          alt={label ?? ''}
          loading="lazy"
          onError={() => setBroken(true)}
          style={{ width: '100%', height: '100%', objectFit: 'cover' }}
        />
      ) : label ? (
        <span
          style={{
            font: `500 10px/1 ${font.mono}`,
            letterSpacing: '.2em',
            color: 'rgba(236,239,247,.32)',
            textTransform: 'uppercase',
          }}
        >
          {label}
        </span>
      ) : null}
    </div>
  );
}
