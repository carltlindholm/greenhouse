# Project config notes

```
npm create vite@latest my-preact-app --template preact-ts
npm create vite@latest setup-ui --template preact-ts
cd setup-ui/
npm install
cp ~/my-react-app/vite.config.ts  .
npm install vite --save-dev
npm install vite-plugin-singlefile --save-dev

npm install bootstrap react-bootstrap
```

# Development

Run devserver 

`npm run dev`

Make distro

`npm run build`

# Fakeserver

`npm install express`

Then 

1. edit fakserver/server.json as server
2. add to vite config
    ```
      server: {
        proxy: {
          // Proxy /api requests to fakeserver
          '/api': 'http://localhost:5000',
        },
      },  
    ```

`npm install concurrently --save-dev` 

 then edit package.json +  "dev": "concurrently \"vite\" \"node fakeserver/server.js\"",