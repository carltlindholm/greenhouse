import { Card, Form } from 'react-bootstrap';
import { SettingsContext } from './settings-context';
import { useContext } from 'preact/hooks';

export function NtpSettings() {
  const { ntp, setNtp } = useContext(SettingsContext)!;

  const handleServerChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setNtp({
      ...ntp,
      server: event.currentTarget.value,
    });
  };

  return (
    <>
      <Card className="m-3">
        <Card.Body>
          <Card.Title>NTP Settings</Card.Title>
          <Form>
            <Form.Group className="mb-3">
              <Form.Label>Server</Form.Label>
              <Form.Control type="text" placeholder="" value={ntp.server} onChange={handleServerChange} />
            </Form.Group>
          </Form>
        </Card.Body>
      </Card>
    </>
  );
};
