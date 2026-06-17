/** 44×26 settings switch — off rgba(236,239,247,.18), on var(--ac); 20px knob. */
export function Toggle({ on, onChange }: { on: boolean; onChange: (v: boolean) => void }) {
  return (
    <button
      data-focusable
      tabIndex={0}
      role="switch"
      aria-checked={on}
      onClick={() => onChange(!on)}
      style={{
        width: 44,
        height: 26,
        borderRadius: 999,
        border: 0,
        cursor: 'pointer',
        padding: 0,
        background: on ? 'var(--ac)' : 'rgba(236,239,247,.18)',
        transition: 'background .2s',
        position: 'relative',
        flex: '0 0 auto',
      }}
    >
      <span
        style={{
          position: 'absolute',
          top: 3,
          left: 3,
          width: 20,
          height: 20,
          borderRadius: '50%',
          background: '#0b0c11',
          transform: on ? 'translateX(18px)' : 'translateX(0)',
          transition: 'transform .2s',
        }}
      />
    </button>
  );
}
