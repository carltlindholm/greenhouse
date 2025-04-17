import { Card, Form } from 'react-bootstrap';
import { SettingsContext } from './settings-context';
import { useContext, useState, useEffect } from 'preact/hooks';

export function WifiSettings({ onPasswordsMatchChange }: { onPasswordsMatchChange?: (match: boolean) => void }) {
  const { wifi, setWifi } = useContext(SettingsContext)!;
  const [password, setPassword] = useState(wifi.password || '');
  const [passwordAgain, setPasswordAgain] = useState('');
  const passWordsMatch = password === passwordAgain;

  useEffect(() => {
    if (onPasswordsMatchChange) {
      onPasswordsMatchChange(passWordsMatch);
    }
  }, [passWordsMatch, onPasswordsMatchChange]);

  useEffect(() => {
    if (wifi.password !== password) {
      setPassword(wifi.password || '');
    }
    if (wifi.password !== passwordAgain) {
      setPasswordAgain(wifi.password || '');
    }
  }, [wifi.password]);

  const handleSsidChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setWifi({
      ...wifi,
      ssid: event.currentTarget.value,
    });
  };

  const handlePasswordInputChange = (value: string, isPasswordAgain: boolean) => {
    if (isPasswordAgain) {
      setPasswordAgain(value);
    } else {
      setPassword(value);
    }
    const othervalue = isPasswordAgain ? password : passwordAgain;
    if (value === othervalue) {
      setWifi({
        ...wifi,
        password: value,
      });
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
            <Form.Group className="mb-3">
              <Form.Label>Password</Form.Label>
              <Form.Control
                type="password"
                placeholder="Password"
                value={password}
                onChange={(e) => handlePasswordInputChange(e.currentTarget.value, false)}
              />
            </Form.Group>
            <Form.Group className="mb-3">
              <Form.Label>Password again</Form.Label>
              <Form.Control
                type="password"
                placeholder="Password again"
                value={passwordAgain}
                onChange={(e) => handlePasswordInputChange(e.currentTarget.value, true)}
                className={!passWordsMatch ? 'bg-warning' : ''}
              />
            </Form.Group>
          </Form>
        </Card.Body>
      </Card>
    </>
  );
};
