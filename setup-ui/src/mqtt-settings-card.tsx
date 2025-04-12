import { Card, Form } from 'react-bootstrap';
import { MqttSettings } from './settings-context';

export function MqttSettingsCard({ mqtt, setMqtt }: { mqtt: MqttSettings; setMqtt: (mqtt: MqttSettings) => void }) {

  const handleBrokerChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMqtt({
      ...mqtt,
      broker: event.currentTarget.value,
    });
  };

  const handlePortChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMqtt({
      ...mqtt,
      port: parseInt(event.currentTarget.value, 10) || 0,
    });
  };

  const handleUserChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMqtt({
      ...mqtt,
      user: event.currentTarget.value,
    });
  };

  const handlePasswordChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMqtt({
      ...mqtt,
      password: event.currentTarget.value,
    });
  };

  const handleTopicChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMqtt({
      ...mqtt,
      topic: event.currentTarget.value,
    });
  };

  const handleDeviceIdChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMqtt({
      ...mqtt,
      deviceId: event.currentTarget.value,
    });
  };

  return (
    <>
      <Card className="m-3">
        <Card.Body>
          <Card.Title>MQTT Settings</Card.Title>
          <Form>
            <div className="d-flex mb-3">
              <Form.Group className="me-2 flex-fill">
                <Form.Label>Broker</Form.Label>
                <Form.Control
                  type="text"
                  placeholder="Broker address"
                  value={mqtt.broker}
                  onChange={handleBrokerChange}
                />
              </Form.Group>
              <Form.Group className="flex-fill">
                <Form.Label>Port</Form.Label>
                <Form.Control
                  type="number"
                  placeholder="Port"
                  value={mqtt.port}
                  onChange={handlePortChange}
                />
              </Form.Group>
            </div>
            <Form.Group className="mb-3">
              <Form.Label>User</Form.Label>
              <Form.Control
                type="text"
                placeholder="User"
                value={mqtt.user}
                onChange={handleUserChange}
              />
            </Form.Group>
            <Form.Group className="mb-3">
              <Form.Label>Password</Form.Label>
              <Form.Control
                type="password"
                placeholder="Password"
                value={mqtt.password}
                onChange={handlePasswordChange}
              />
            </Form.Group>
            <Form.Group className="mb-3">
              <Form.Label>Device ID</Form.Label>
              <Form.Control
                type="text"
                placeholder="Device ID"
                value={mqtt.deviceId}
                onChange={handleDeviceIdChange}
              />
            </Form.Group>
            <Form.Group className="mb-3">
              <Form.Label>Topic</Form.Label>              
              <Form.Control
                type="text"
                placeholder="Topic"
                value={mqtt.topic}
                onChange={handleTopicChange}
              />
              <Form.Text className="text-muted">
                Hint: losant/{'{device_id}'}/state
              </Form.Text>
            </Form.Group>
          </Form>
        </Card.Body>
      </Card>
    </>
  );
};
