import { SettingsContext } from './settings-context';
import { useContext } from 'preact/hooks';

export function SettingsDebugCard() {
  const settings = useContext(SettingsContext); // Access all settings

  return (
    <>
      <style>
          pre {`{
            max-height: 300px;
            white-space: pre-wrap;
          }`}
      </style>
      <pre>
        {JSON.stringify(settings, null, 2)}
      </pre>
    </>
  );
};
