// server.js
import express from 'express';
const app = express();
const port = 5000; // Backend runs on port 5000

// Fake endpoint for settings
app.get('/api/settings', (req, res) => {
  res.json({
    wifi: {
      ssid: 'MyWifi',
      password: 'password123helloworld!',
    },
    ntp: {
      ntpServer: 'pool.ntp.org',
    },
  });
});

// You can add other routes here as needed
app.listen(port, () => {
  console.log(`Backend is running at http://localhost:${port}`);
});
