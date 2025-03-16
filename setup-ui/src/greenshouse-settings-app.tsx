import 'bootstrap/dist/css/bootstrap.min.css';
import { Tab, Tabs, Card, Button} from 'react-bootstrap';

export function GreenhouseSettingsApp() {

  return (
    <>
      <div className="d-flex flex-column" style={{ width: '100vw' }}>
         {/* Title */}
        <div className="bg-primary text-white p-3 text-left">
          <h1>&#x1f345; Greenhouse</h1>
        </div>
        {/* Settings categories tabs */}
        <Tabs defaultActiveKey="wifi">
            <Tab eventKey="wifi" title="Wifi">
              <Card className="m-3">
                <Card.Body>
                    <Card.Title>Wifi Settings</Card.Title>
                    <Card.Text>Configure your Wifi network.</Card.Text>
                    <Button variant="primary">Save Wifi Settings</Button>
                </Card.Body>
              </Card>            
            </Tab>
            <Tab eventKey="ntp" title="NTP">
              <Card className="m-3">
                <Card.Body>
                    <Card.Title>NTP Settings</Card.Title>
                    <Card.Text>Configure NTP server.</Card.Text>
                    <Button variant="primary">Save NTP Settings</Button>
                </Card.Body>
              </Card>
            </Tab>        
        </Tabs>
        {/*  Bottom action bar  */}
        <div className="bg-dark text-white p-3 text-center">
            <Button variant="secondary" className="mx-2">Save</Button>
            <Button variant="danger" className="mx-2">Reboot</Button>
            <Button variant="info" className="mx-2">Reload from Saved</Button>
        </div>      
      </div>
    </>
  )
}
