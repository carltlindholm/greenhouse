import './pump-schedule-card.css';
import { Card, Form, Button, Table } from 'react-bootstrap';
import { PumpSchedule, ScheduledPumping, TriggerTime } from './settings-context';

interface PumpScheduleCardProps {
  pumpSchedule: PumpSchedule;
  setPumpSchedule: (pumpSchedule: PumpSchedule) => void;
}

export function PumpScheduleCard({ pumpSchedule, setPumpSchedule }: PumpScheduleCardProps) {
  const handleUtcOffsetChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const newUtcOffset = parseInt(event.currentTarget.value, 10) || 0;
    setPumpSchedule({ ...pumpSchedule, utcOffset: newUtcOffset });
  };

  const handleAddRow = () => {
    const newPump: ScheduledPumping = {
      start: { hour: 0, minute: 0, second: 0 },
      end: { hour: 0, minute: 0, second: 0 },
    };
    setPumpSchedule({ ...pumpSchedule, pump: [...pumpSchedule.pump, newPump] });
  };

  const handleRemoveRow = (index: number) => {
    const updatedPump = pumpSchedule.pump.filter((_, i) => i !== index);
    setPumpSchedule({ ...pumpSchedule, pump: updatedPump });
  };

  const handleTimeChange = (index: number, field: 'start' | 'end', timeField: keyof TriggerTime, value: number) => {
    const updatedPump = [...pumpSchedule.pump];
    updatedPump[index][field][timeField] = value;
    setPumpSchedule({ ...pumpSchedule, pump: updatedPump });
  };

  return (
    // TODO: Clean up after copilot.
    <>
      <Card className="m-3">
        <Card.Body>
          <Card.Title>Pump Schedule</Card.Title>
          <Form>
            <Form.Group className="mb-3">
              <Form.Label>UTC Offset</Form.Label>
              <Form.Control
                type="number"
                placeholder="UTC Offset"
                value={pumpSchedule.utcOffset}
                onChange={handleUtcOffsetChange}
              />
            </Form.Group>
          </Form>
          <Table striped bordered hover>
            <thead>
              <tr>
                <th className="header-time-start">Start Time (HH:MM:SS)</th>
                <th>End Time (HH:MM:SS)</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              {pumpSchedule.pump.map((pumping, index) => (
                <tr key={index}>
                  <td>
                    <div className="row justify-content-end">
                    <Form.Control
                      type="number"
                      placeholder="HH"
                      value={pumping.start.hour.toString().padStart(2, '0')}
                      onChange={(e) =>
                      handleTimeChange(index, 'start', 'hour', parseInt(e.currentTarget.value, 10) || 0)
                      }
                      className="me-1 time-input"
                    />
                    <Form.Control
                      type="number"
                      placeholder="MM"
                      value={pumping.start.minute.toString().padStart(2, '0')}
                      onChange={(e) =>
                      handleTimeChange(index, 'start', 'minute', parseInt(e.currentTarget.value, 10) || 0)
                      }
                      className="me-1 time-input"
                    />
                    <Form.Control
                      type="number"
                      placeholder="SS"
                      value={pumping.start.second.toString().padStart(2, '0')}
                      onChange={(e) =>
                      handleTimeChange(index, 'start', 'second', parseInt(e.currentTarget.value, 10) || 0)
                      }
                      className="me-1 time-input"
                    />
                    </div>
                  </td>
                  <td>
                    <div className="d-flex align-items-start">
                      <Form.Control
                        type="number"
                        placeholder="HH"
                        value={pumping.end.hour.toString().padStart(2, '0')}
                        onChange={(e) =>
                          handleTimeChange(index, 'end', 'hour', parseInt(e.currentTarget.value, 10) || 0)
                        }
                        className="me-1 time-input"
                      />
                      <Form.Control
                        type="number"
                        placeholder="MM"
                        value={pumping.end.minute.toString().padStart(2, '0')}
                        onChange={(e) =>
                          handleTimeChange(index, 'end', 'minute', parseInt(e.currentTarget.value, 10) || 0)
                        }
                        className="me-1 time-input"
                      />
                      <Form.Control
                        type="number"
                        placeholder="SS"
                        value={pumping.end.second.toString().padStart(2, '0')}
                        onChange={(e) =>
                          handleTimeChange(index, 'end', 'second', parseInt(e.currentTarget.value, 10) || 0)
                        }
                        className="me-1 time-input"
                      />
                    </div>
                  </td>
                  <td>
                    <Button variant="danger" onClick={() => handleRemoveRow(index)}>
                      Remove
                    </Button>
                  </td>
                </tr>
              ))}
            </tbody>
          </Table>
          <Button variant="primary" onClick={handleAddRow}>
            Add Row
          </Button>
        </Card.Body>
      </Card>
    </>
  );
}