import 'bootstrap/dist/css/bootstrap.min.css';
import { Tab, Tabs, Button } from 'react-bootstrap';
import { SettingsContext, createSettings } from './settings-context';
import { useContext, useEffect } from 'preact/hooks';
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

  const loadSettings = async () => {
    const response = await fetch('/api/settings');
    if (response.ok) {
      const data = await response.json();
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

  useEffect(() => {
    loadSettings();  // Initial data load
  }, []);

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
            <Button className="mx-2" onClick={saveSettings}>Save</Button>
            <Button
              variant="secondary"
              className="mx-2"
              onClick={loadSettings}
            >
              Reload Saved
            </Button>
            <Button variant="danger" className="mx-2">Reboot</Button>
          </div>
        </div>
        <SettingsDebug />
      </SettingsContext.Provider>
    </>
  );
}
