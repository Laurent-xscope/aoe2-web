import { resolve } from 'path';
import { defineConfig } from 'vite';

export default defineConfig({
  base: './',
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    sourcemap: true,
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'index.html'),
        canvas: resolve(__dirname, 'canvas.html'),
      },
    },
  },
  server: {
    port: 3000,
    open: true
  }
});
