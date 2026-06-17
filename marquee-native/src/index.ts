import { registerPlugin } from '@capacitor/core';
import type { MarqueeNativePlugin } from './definitions';

/**
 * Resolves to the native implementation on iOS/Android, and lazy-loads the
 * web implementation (MarqueeNativeWeb) in the browser / PWA.
 */
export const MarqueeNative = registerPlugin<MarqueeNativePlugin>('MarqueeNative', {
  web: () => import('./web').then((m) => new m.MarqueeNativeWeb()),
});

export * from './definitions';
