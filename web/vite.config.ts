import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// Dev server proxies the nedflix Express API + auth so the SPA runs same-origin.
// Override the backend with VITE_PROXY_TARGET (default http://localhost:3000).
const target = process.env.VITE_PROXY_TARGET ?? 'http://localhost:3000';

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  // Built assets are served by server.js from web/dist at the site root.
  base: '/',
  build: { outDir: 'dist', sourcemap: true },
  server: {
    proxy: {
      '/api': { target, changeOrigin: true, secure: false },
      '/auth': { target, changeOrigin: true, secure: false },
      '/login.html': { target, changeOrigin: true, secure: false },
    },
  },
});
