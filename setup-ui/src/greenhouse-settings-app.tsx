import 'bootstrap/dist/css/bootstrap.min.css';
import { Tab, Tabs, Button, Modal, Toast } from 'react-bootstrap';
import { SettingsContext, createSettings } from './settings-context';
import { useEffect, useState } from 'preact/hooks';
import { WifiSettings } from './wifi-settings-card';
import { NtpSettings } from './ntp-settings-card';
import { MqttSettingsCard } from './mqtt-settings-card';
import { PumpScheduleCard } from './pump-schedule-card';
import { SettingsDebugCard } from './settings-debug-card';

export function GreenhouseSettingsApp() {
  const [globalSettings, setSettings] = createSettings();
  const [hasEdits, setHasEdits] = useState(false);
  const [loadedStateJson, setLoadedStateJson] = useState('');
  const [isScheduleTidied, setIsScheduleTidied] = useState(true); // Track tidiness
  const [showTidyDialog, setShowTidyDialog] = useState(false); // Track dialog visibility
  const [showToast, setShowToast] = useState(false); // Track toaster visibility
  const [wifiPasswordsMatch, setWifiPasswordsMatch] = useState(true); // Track WiFi password match
  const [mqttPasswordsMatch, setMqttPasswordsMatch] = useState(true); // Track MQTT password match

  useEffect(() => {
    loadSettings();  // Initial data load
  }, []);

  useEffect(() => {
    // Check for changes in settings every 250ms.
    // I never got the useEffect to properly unset edits on load
    // but on the other hand, this also works if you go back to the
    // initial state with an edit.
    const timer = setTimeout(() => {
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
    if (!isScheduleTidied) {
      setShowTidyDialog(true); // Show dialog if schedule is not tidied
      return;
    }
    const newData = JSON.stringify(globalSettings);
    const response = await fetch('/api/save-settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: newData,
    });
    if (!response.ok) {
      alert('Failed to save settings. Please try again.');
    } else {
      setLoadedStateJson(newData);
      setHasEdits(false); // Reset edit state
      setShowToast(true); // Show success toaster
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

  const handleTidyDialogClose = () => setShowTidyDialog(false);

  return (
    <>
      <SettingsContext.Provider value={globalSettings}>
        <div className="d-flex flex-column" style={{ width: '100vw' }}>
          {/* Title */}
          <div className="text-white p-3 text-left" style={{ backgroundColor: '#207b20' }}>
            <h1>&#x1f345; Greenhouse</h1>
          </div>
          <div style={{ minHeight: '704px' }}>
            {/* Settings categories tabs */}
            <Tabs defaultActiveKey="wifi">
              <Tab eventKey="wifi" title="Wifi">
                <WifiSettings onPasswordsMatchChange={setWifiPasswordsMatch} />
              </Tab>
              <Tab eventKey="ntp" title="NTP">
                <NtpSettings />
              </Tab>
              <Tab eventKey="mqtt" title="MQTT">
                <MqttSettingsCard
                  mqtt={globalSettings.mqtt}
                  setMqtt={globalSettings.setMqtt}
                  onPasswordsMatchChange={setMqttPasswordsMatch}
                />
              </Tab>
              <Tab eventKey="pump-schedule" title="Pump Schedule">
                <PumpScheduleCard
                  pumpSchedule={globalSettings.pumpSchedule}
                  setPumpSchedule={globalSettings.setPumpSchedule}
                  onScheduleTidyChange={setIsScheduleTidied} // Pass tidiness callback
                />
              </Tab>
            <Tab eventKey="debug" title="Debug">
              <SettingsDebugCard />
              </Tab>
            </Tabs>
          </div>
          {/*  Bottom action bar  */}
          <div className="bg-dark text-white p-3 text-center">
            <Button
              className="mx-2"
              onClick={saveSettings}
              disabled={!hasEdits || !wifiPasswordsMatch || !mqttPasswordsMatch}
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
      </SettingsContext.Provider>
      {/* Tidy Schedule Dialog */}
      <Modal show={showTidyDialog} onHide={handleTidyDialogClose}>
        <Modal.Header closeButton>
          <Modal.Title>Messy Pump Schedule &#x1F62E;</Modal.Title>
        </Modal.Header>
        <Modal.Body>
          Please fix issues with the pump schedule before saving. Tip: Press "Tidy Up"
        </Modal.Body>
        <Modal.Footer>
          <Button variant="secondary" onClick={handleTidyDialogClose}>
            Close
          </Button>
        </Modal.Footer>
      </Modal>
      <Toast
        onClose={() => setShowToast(false)}
        show={showToast}
        delay={3000}
        autohide
        style={{
          position: 'fixed',
          top: '4px',
          right: '10px',
          zIndex: 1050,
        }}
      >
        <Toast.Header>
          <strong className="me-auto">Save</strong>
        </Toast.Header>
        <Toast.Body>
          Settings saved successfully!
        </Toast.Body>
      </Toast>
    </>
  );
}
