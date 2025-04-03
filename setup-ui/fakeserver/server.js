// server.js
import express from 'express';
const app = express();
const port = 5000; // Backend runs on port 5000

let userSavedSettings = undefined;

// Middleware to parse incoming JSON request bodies
app.use(express.json()); // This is necessary for parsing JSON payloads

// Fake endpoint for settings
app.get('/api/settings', (req, res) => {
  res.json(userSavedSettings || {
    "wifi": {
      "ssid": "mywifi" + Math.floor(Math.random() * 1000),
      "password": "pass",
    },
    "ntp": {
      "server": "pool.ntp.org"
    },
    "mqtt": {
      "broker": "",
      "port": 1883,
      "user": "",
      "password": "",
      "topic": ""
    },
    "pumpSchedule": {
      "pump": [],
      "utcOffset": 3
    }
  });
});

app.post('/api/save-settings', (req, res) => {
  console.log("Saving settings", req.body);
  userSavedSettings = req.body;
  res.sendStatus(200);
});

app.post('/api/reboot', (req, res) => {
  console.log("Aieee....", req.body);
  userSavedSettings = undefined;
  res.sendStatus(200);
});

app.listen(port, () => {
  console.log(`Backend is running at http://localhost:${port}`);
});
