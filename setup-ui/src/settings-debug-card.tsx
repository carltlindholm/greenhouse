import { SettingsContext } from './settings-context';
import { useContext, useState } from 'preact/hooks';

export function SettingsDebugCard() {
  const settings = useContext(SettingsContext);
  const [isLocked, setIsLocked] = useState(true);

  const toggleLock = () => {
    setIsLocked((prev) => !prev);
  };

  const maskSensitiveData = (data: any): any => {
    if (typeof data === 'object' && data !== null) {
      return Object.fromEntries(
        Object.entries(data).map(([key, value]) => [
          key,
          key === 'password' && isLocked
        ? '•'.repeat(String(value).length)
        : maskSensitiveData(value),
        ])
      );
    }
    return data;
  };

  return (
    <div style={{ position: 'relative' }}>
      <button
        onClick={toggleLock}
        style={{
          position: 'absolute',
          top: '10px',
          right: '10px',
          background: 'none',
          border: 'none',
          fontFamily: 'sans-serif',
          color: 'black',
          fontSize: '24px',
          cursor: 'pointer',
        }}
        aria-label={isLocked ? 'Lock' : 'Unlock'}
      >
        {isLocked ? String.fromCodePoint(0x1F512) : String.fromCodePoint(0x1F513)}
      </button>
      <pre
        style={{
          maxHeight: '646px',
          whiteSpace: 'pre-wrap',
        }}
      >
        {JSON.stringify(maskSensitiveData(settings), null, 2)}
      </pre>
    </div>
  );
}
