import { Card, Form } from 'react-bootstrap';
import { SettingsContext } from './settings-context';
import { useContext } from 'preact/hooks';

export function WifiSettings() {
  const { wifi, setWifi } = useContext(SettingsContext)!;

  const handleSsidChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setWifi({
      ...wifi,
      ssid: event.currentTarget.value,
    });
  };

  const handlePasswordChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setWifi({
      ...wifi,
      password: event.currentTarget.value,
    });
  };  

  return (
    <>
      <Card className="m-3">
        <Card.Body>
          <Card.Title>Wifi Settings</Card.Title>
          <Form>
            <Form.Group className="mb-3">
              <Form.Label>SSID</Form.Label>
              <Form.Control type="text" placeholder="" value={wifi.ssid} onChange={handleSsidChange} />
            </Form.Group>
            <Form.Group className="mb-3">
              <Form.Label>Password</Form.Label>
              <Form.Control type="password" placeholder="Password" value={wifi.password} onChange={handlePasswordChange} />
            </Form.Group>
          </Form>
        </Card.Body>
      </Card>
    </>
  );
};
