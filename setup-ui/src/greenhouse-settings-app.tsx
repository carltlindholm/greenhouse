import 'bootstrap/dist/css/bootstrap.min.css';
import { Tab, Tabs, Button } from 'react-bootstrap';
import { SettingsContext, createSettings } from './settings-context';
import { useContext, useEffect, useState } from 'preact/hooks';
import { WifiSettings } from './wifi-settings-card';
import { NtpSettings } from './ntp-settings-card';
import { MqttSettingsCard } from './mqtt-settings-card';
import { PumpScheduleCard } from './pump-schedule-card';

const SettingsDebug = () => {
  const settings = useContext(SettingsContext); // Access all settings

  return (
    <pre>
      {JSON.stringify(settings, null, 2)}
    </pre>
  );
};

export function GreenhouseSettingsApp() {
  const [globalSettings, setSettings] = createSettings();
  const [hasEdits, setHasEdits] = useState(false);
  const [loadedStateJson, setLoadedStateJson] = useState('');

  useEffect(() => {
    loadSettings();  // Initial data load
  }, []);

  useEffect(() => {
    // Check for changes in settings every 250ms.
    // I never got the useEffect to properly unset edits on load
    // but on the other hand, this also works if you go back to the
    // initial state with an edit.
    const timer = setTimeout(() => {
      console.log('Check for edits');
      const newHasEdits = JSON.stringify(globalSettings) !== loadedStateJson;
      if (hasEdits !== newHasEdits) {
        setHasEdits(newHasEdits);
      }
    }, 250);
    return () => { clearTimeout(timer); };
  }, [globalSettings]);

  const loadSettings = async () => {
    const response = await fetch('/api/settings');
    if (response.ok) {
      const data = await response.json();
      setLoadedStateJson(JSON.stringify(data)); // Save loaded state
      setSettings(data);  // Update settings in context      
    }
  };

  const saveSettings = async () => {
    const response = await fetch('/api/save-settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(globalSettings),
    });
    if (!response.ok) {
      alert('Failed to save settings. Please try again.');
    }
  };

  const restartEsp = async () => {
    const response = await fetch('/api/reboot', {
      method: 'POST',
    });
    if (!response.ok) {
      alert('Failed to restart. Please try again.');
    }
  };

  return (
    <>
      <SettingsContext.Provider value={globalSettings}>
        <div className="d-flex flex-column" style={{ width: '100vw' }}>
          {/* Title */}
          <div className="bg-primary text-white p-3 text-left">
            <h1>&#x1f345; Greenhouse</h1>
          </div>
          {/* Settings categories tabs */}
          <Tabs defaultActiveKey="wifi">
            <Tab eventKey="wifi" title="Wifi">
              <WifiSettings />
            </Tab>
            <Tab eventKey="ntp" title="NTP">
              <NtpSettings />
            </Tab>
            <Tab eventKey="mqtt" title="MQTT">
              <MqttSettingsCard
                mqtt={globalSettings.mqtt}
                setMqtt={globalSettings.setMqtt}
              />
            </Tab>
            <Tab eventKey="pump-schedule" title="Pump Schedule">
              <PumpScheduleCard
                pumpSchedule={globalSettings.pumpSchedule}
                setPumpSchedule={globalSettings.setPumpSchedule}
              />
            </Tab>
          </Tabs>
          {/*  Bottom action bar  */}
          <div className="bg-dark text-white p-3 text-center">
            <Button
              className="mx-2"
              onClick={saveSettings}
              disabled={!hasEdits}
            >
              Save
            </Button>
            <Button
              variant="secondary"
              className="mx-2"
              onClick={loadSettings}
            >
              Reload Saved
            </Button>
            <Button 
              className="mx-2"
              onClick={restartEsp}
              disabled={hasEdits}
            >
              Restart
            </Button>
          </div>
        </div>
        <SettingsDebug />
      </SettingsContext.Provider>
    </>
  );
}
