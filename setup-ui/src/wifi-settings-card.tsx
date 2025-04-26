import { Card, Form } from 'react-bootstrap';
import { SettingsContext } from './settings-context';
import { useContext } from 'preact/hooks';
import { VerifiedPasswordControl } from './verified-password-control';

export function WifiSettings({ onPasswordsMatchChange }: { onPasswordsMatchChange?: (match: boolean) => void }) {
  const { wifi, setWifi } = useContext(SettingsContext)!;

  const handleSsidChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setWifi({
      ...wifi,
      ssid: event.currentTarget.value,
    });
  };

  const handlePasswordChange = (newPassword: string, passwordsMatch: boolean) => {
    if (passwordsMatch) {
      setWifi({
        ...wifi,
        password: newPassword,
      });
    }
    if (onPasswordsMatchChange) {
      onPasswordsMatchChange(passwordsMatch);
    }
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
            <VerifiedPasswordControl password={wifi.password} onPasswordChange={handlePasswordChange} />
          </Form>
        </Card.Body>
      </Card>
    </>
  );
}
