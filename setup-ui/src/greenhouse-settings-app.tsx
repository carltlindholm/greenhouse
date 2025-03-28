import 'bootstrap/dist/css/bootstrap.min.css';
import { Tab, Tabs, Button } from 'react-bootstrap';
import { SettingsContext, createSettings } from './settings-context';
import { useContext } from 'preact/hooks';
import { WifiSettings } from './wifi-settings-card';
import { NtpSettings } from './ntp-settings-card';
import { MqttSettingsCard } from './mqtt-settings-card';

const SettingsDebug = () => {
  const settings = useContext(SettingsContext);  // Access wifi settings

  return (
    <pre>
      {JSON.stringify(settings, null, 2)}
    </pre>
  );
};

export function GreenhouseSettingsApp() {
  const globalSettings = createSettings();

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
          </Tabs>
          {/*  Bottom action bar  */}
          <div className="bg-dark text-white p-3 text-center">
            <Button variant="secondary" className="mx-2">Save</Button>
            <Button variant="danger" className="mx-2">Reboot</Button>
            <Button variant="info" className="mx-2">Reload from Saved</Button>
          </div>
        </div>
        <SettingsDebug />
      </SettingsContext.Provider>
    </>
  )
}
