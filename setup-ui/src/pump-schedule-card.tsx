import './pump-schedule-card.css';
import { Card, Form, Button, Table } from 'react-bootstrap';
import { PumpSchedule, ScheduledPumping, TriggerTime } from './settings-context';

const BAD_DURATION_SECONDS = 1200; // 20 minutes
const WARNING_DURATION_SECONDS = 300; // 5 minutes

interface PumpScheduleCardProps {
  pumpSchedule: PumpSchedule;
  setPumpSchedule: (pumpSchedule: PumpSchedule) => void;
}

export function PumpScheduleCard({ pumpSchedule, setPumpSchedule }: PumpScheduleCardProps) {
  const clamp = (value: number, min: number, max: number) => Math.min(Math.max(value, min), max);

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

  const handleTimeChange = (index: number, field: 'start' | 'end', timeField: keyof TriggerTime, rawValue: string) => {
    const value = parseInt(rawValue, 10) || 0;
    const clampedValue =
      timeField === 'hour' ? clamp(value, 0, 23) : clamp(value, 0, 59);
    const updatedPump = [...pumpSchedule.pump];
    updatedPump[index][field][timeField] = clampedValue;
    setPumpSchedule({ ...pumpSchedule, pump: updatedPump });
  };

  const calculateDuration = (start: TriggerTime, end: TriggerTime) => {
    const startSeconds = start.hour * 3600 + start.minute * 60 + start.second;
    const endSeconds = end.hour * 3600 + end.minute * 60 + end.second;
    const durationSeconds = endSeconds - startSeconds;

    const minutes = Math.floor(Math.abs(durationSeconds) / 60);
    const seconds = Math.abs(durationSeconds) % 60;
    const formattedDuration = `${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;

    let durationClass = '';
    if (durationSeconds < 0 || durationSeconds > BAD_DURATION_SECONDS) {
      durationClass = 'table-danger';
    } else if (durationSeconds > WARNING_DURATION_SECONDS) {
      durationClass = 'table-warning';
    }

    return { formattedDuration, durationClass };
  };

  const handleTidyUp = () => {
    let updatedPump = pumpSchedule.pump
      .filter((pumping) => {
        // Remove rows with all zeroes
        const isStartZero = pumping.start.hour === 0 && pumping.start.minute === 0 && pumping.start.second === 0;
        const isEndZero = pumping.end.hour === 0 && pumping.end.minute === 0 && pumping.end.second === 0;
        return !(isStartZero && isEndZero);
      })
      .map((pumping) => {
        // Fix bad durations by setting end time to 1 minute after start
        const startSeconds = pumping.start.hour * 3600 + pumping.start.minute * 60 + pumping.start.second;
        const endSeconds = pumping.end.hour * 3600 + pumping.end.minute * 60 + pumping.end.second;
        if (endSeconds <= startSeconds || endSeconds - startSeconds > BAD_DURATION_SECONDS) {
          const newEndSeconds = startSeconds + 60;
          return {
            ...pumping,
            end: {
              hour: Math.floor(newEndSeconds / 3600),
              minute: Math.floor((newEndSeconds % 3600) / 60),
              second: newEndSeconds % 60,
            },
          };
        }
        return pumping;
      })
      .sort((a, b) => {
        // Order by start time ascending
        const aStartSeconds = a.start.hour * 3600 + a.start.minute * 60 + a.start.second;
        const bStartSeconds = b.start.hour * 3600 + b.start.minute * 60 + b.start.second;
        return aStartSeconds - bStartSeconds;
      });

    // Remove overlapping durations
    updatedPump = updatedPump.filter((pumping, index, array) => {
      if (index === 0) return true;
      const prevEndSeconds =
        array[index - 1].end.hour * 3600 +
        array[index - 1].end.minute * 60 +
        array[index - 1].end.second;
      const currentStartSeconds =
        pumping.start.hour * 3600 +
        pumping.start.minute * 60 +
        pumping.start.second;
      return currentStartSeconds >= prevEndSeconds;
    });

    setPumpSchedule({ ...pumpSchedule, pump: updatedPump });
  };

  return (
    <>
      <Card className="m-3">
        <Card.Body>
          <Card.Title>Pump Schedule</Card.Title>
          <Form>
            <Form.Group className="mb-3 d-flex justify-content-start align-items-end gap-3">
              <Form.Label className="x-col-sm-1">UTC Offset</Form.Label>
              <div>
                <Form.Control
                  type="number"
                  className="me-1 number-input"
                  placeholder="3"
                  value={pumpSchedule.utcOffset}
                  onChange={handleUtcOffsetChange}
                />
              </div>
            </Form.Group>
          </Form>
          <Table striped bordered hover>
            <thead>
              <tr>
                <th className="header-time-start">Start Time (HH:MM:SS)</th>
                <th>End Time (HH:MM:SS)</th>
                <th>Duration</th>
                <th></th>
              </tr>
            </thead>
            <tbody>
              {pumpSchedule.pump.map((pumping, index) => {
                const { formattedDuration, durationClass } = calculateDuration(pumping.start, pumping.end);
                return (
                  <tr key={index}>
                    <td>
                      <div className="d-flex justify-content-end align-items-start">
                        <Form.Control
                          type="number"
                          placeholder="HH"
                          value={pumping.start.hour.toString().padStart(2, '0')}
                          onChange={(e) =>
                            handleTimeChange(index, 'start', 'hour', e.currentTarget.value)
                          }
                          className="me-1 number-input"
                        />
                        <Form.Control
                          type="number"
                          placeholder="MM"
                          value={pumping.start.minute.toString().padStart(2, '0')}
                          onChange={(e) =>
                            handleTimeChange(index, 'start', 'minute', e.currentTarget.value)
                          }
                          className="me-1 number-input"
                        />
                        <Form.Control
                          type="number"
                          placeholder="SS"
                          value={pumping.start.second.toString().padStart(2, '0')}
                          onChange={(e) =>
                            handleTimeChange(index, 'start', 'second', e.currentTarget.value)
                          }
                          className="me-1 number-input"
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
                            handleTimeChange(index, 'end', 'hour', e.currentTarget.value)
                          }
                          className="me-1 number-input"
                        />
                        <Form.Control
                          type="number"
                          placeholder="MM"
                          value={pumping.end.minute.toString().padStart(2, '0')}
                          onChange={(e) =>
                            handleTimeChange(index, 'end', 'minute', e.currentTarget.value)
                          }
                          className="me-1 number-input"
                        />
                        <Form.Control
                          type="number"
                          placeholder="SS"
                          value={pumping.end.second.toString().padStart(2, '0')}
                          onChange={(e) =>
                            handleTimeChange(index, 'end', 'second', e.currentTarget.value)
                          }
                          className="me-1 number-input"
                        />
                      </div>
                    </td>
                    <td className={durationClass}>
                      {formattedDuration}
                    </td>
                    <td>
                      <Button variant="danger" onClick={() => handleRemoveRow(index)}>
                        &#x02716;
                      </Button>
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </Table>
          <Button variant="primary" className="mx-2" onClick={handleAddRow}>
            Add Row
          </Button>
          <Button variant="secondary" className="mx-2" onClick={handleTidyUp}>
            Tidy Up
          </Button>
        </Card.Body>
      </Card>
    </>
  );
}