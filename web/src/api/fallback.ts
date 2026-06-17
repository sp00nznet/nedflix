// Fixture fallback gate. Enabled in dev (`npm run dev`) and in preview builds compiled
// with VITE_ALLOW_MOCK=1 (e.g. the desktop demo shell), so surfaces render prototype data
// when the host doesn't implement the Marquee API. The real server build leaves it OFF —
// failures surface instead of showing fake data.
export const ALLOW_MOCK = import.meta.env.DEV || import.meta.env.VITE_ALLOW_MOCK === '1';

export async function orFallback<T>(p: Promise<T>, fb: () => T): Promise<T> {
  try {
    return await p;
  } catch (e) {
    if (ALLOW_MOCK) return fb();
    throw e;
  }
}
