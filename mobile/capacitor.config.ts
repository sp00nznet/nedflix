import type { CapacitorConfig } from '@capacitor/cli';

// Marquee mobile shell. `webDir` is the copied web build (run `npm run copy:web`).
// To run against a live nedflix server instead of the bundled build, set
// `server.url` to the server origin (the SPA rides the session cookie there).
const config: CapacitorConfig = {
  appId: 'mov.marquee.app',
  appName: 'Marquee',
  webDir: 'www',
  backgroundColor: '#0b0c11',
  ios: { contentInset: 'always', backgroundColor: '#0b0c11' },
  android: { backgroundColor: '#0b0c11' },
  plugins: {
    SplashScreen: { backgroundColor: '#0b0c11', showSpinner: false, launchAutoHide: true },
    StatusBar: { style: 'DARK', backgroundColor: '#0b0c11' },
    // server: { url: 'https://your-nedflix-host', cleartext: false },
  },
};

export default config;
