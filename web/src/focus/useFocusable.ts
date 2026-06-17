import { useCallback } from 'react';

/**
 * Marks an element as part of the spatial-focus graph. Spread the returned props
 * onto any interactive element so the focus engine + gamepad can reach it.
 *
 *   const focus = useFocusable();
 *   <button {...focus}>…</button>
 *
 * The visible focus ring (`box-shadow: 0 0 0 2px var(--ac)`) is the primary 10-foot
 * affordance — apply it via the `:focus-visible` / `[data-focusable]:focus` rules in
 * index.css, or a component-level focus style.
 */
export function useFocusable() {
  // No per-element registration needed: spatialNav queries the DOM by [data-focusable].
  // The hook exists so call sites are declarative and future-proof (e.g. roving tabindex).
  const ref = useCallback((_el: HTMLElement | null) => {}, []);
  return { tabIndex: 0, 'data-focusable': true, ref } as const;
}
