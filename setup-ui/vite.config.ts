import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'
import { viteSingleFile } from 'vite-plugin-singlefile'

// https://vite.dev/config/
export default defineConfig({
  plugins: [preact(), viteSingleFile()],
  build: {
    assetsInlineLimit: 100000000, // Inline everything
    chunkSizeWarningLimit: 100000000, // Large enough to avoid chunking warnings
  },
  server: {
    proxy: {
      // Proxy /api requests to fakeserver
      '/api': 'http://localhost:5000',
    },
  },  
})
