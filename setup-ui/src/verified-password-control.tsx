import { Form } from 'react-bootstrap';
import { useState, useEffect } from 'preact/hooks';

export function VerifiedPasswordControl({
  password,
  onPasswordChange,
}: {
  password: string;
  onPasswordChange: (newPassword: string, passwordsMatch: boolean) => void;
}) {
  const [passwordOnce, setPasswordOnce] = useState(password);
  const [passwordAgain, setPasswordAgain] = useState(password);

  useEffect(() => {
    setPasswordOnce(password);
    setPasswordAgain(password);
  }, [password]);

  const handlePasswordsChange = (newPasswordOnce: string, newPasswordAgain: string) => {
    const newPasswordsMatch = newPasswordOnce === newPasswordAgain;
    if (newPasswordOnce !== passwordOnce) {
      setPasswordOnce(newPasswordOnce);
    }
    if (newPasswordAgain !== passwordAgain) {
      setPasswordAgain(newPasswordAgain);
    }
    onPasswordChange(newPasswordOnce, newPasswordsMatch);
  };

  return (
    <>
      <Form.Group className="mb-3">
        <Form.Label>Password</Form.Label>
        <Form.Control
          type="password"
          placeholder="Password"
          value={passwordOnce}
          onChange={(e) => handlePasswordsChange(e.currentTarget.value, passwordAgain)}
        />
      </Form.Group>
      <Form.Group className="mb-3">
        <Form.Label>Password again</Form.Label>
        <Form.Control
          type="password"
          placeholder="Password again"
          value={passwordAgain}
          onChange={(e) => handlePasswordsChange(passwordOnce, e.currentTarget.value)}
          className={passwordOnce !== passwordAgain ? 'bg-warning' : ''}
        />
        <Form.Text className={passwordOnce !== passwordAgain ? 'text-danger' : 'text-success'}>
          {passwordOnce !== passwordAgain ? 'Passwords do not match' : 'Passwords match'}
         </Form.Text>
      </Form.Group>
    </>
  );
}
