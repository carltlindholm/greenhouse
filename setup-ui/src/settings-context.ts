import { createContext } from 'preact';
import { useState } from 'preact/hooks';

const MQTT_PORT = 1883;
const UTC_OFFSET = 3;

export interface WifiSettings {
  ssid: string;
  password: string;
}

export interface NtpSettings {
  server: string;
}

export interface MqttSettings {
  broker: string;
  port: number; // Default: MQTT_PORT
  user: string;
  password: string;
  topic: string;
}

export interface Settings {
  wifi: WifiSettings;
  ntp: NtpSettings;
  mqtt: MqttSettings;
  pumpSchedule: PumpSchedule;
}

export interface TriggerTime {
  hour: number;
  minute: number;
  second: number;
}
export interface ScheduledPumping {
  start: TriggerTime;
  end: TriggerTime;
}

export interface PumpSchedule {
  pump: ScheduledPumping[];
  utcOffset: number;
}

interface SettingsContextType extends Settings {
  setWifi: (wifi: WifiSettings) => void;
  setNtp: (ntp: NtpSettings) => void;
  setMqtt: (mqtt: MqttSettings) => void;
  setPumpSchedule: (pumpSchedule: PumpSchedule) => void;
}

const INITIAL_SETTINGS:  Settings = {
  wifi: { ssid: '', password: '' },
  ntp: { server: 'pool.ntp.org' },
  mqtt: { broker: '', port: MQTT_PORT, user: '', password: '', topic: '' },
  pumpSchedule: {pump: [], utcOffset: UTC_OFFSET},
};

export const SettingsContext = createContext<SettingsContextType>({
  ...INITIAL_SETTINGS,
  setWifi: () => { },
  setNtp: () => { },
  setMqtt: () => { },
  setPumpSchedule: () => { },
});

// Create settings to be provided to the context.
export function createSettings() : SettingsContextType {
  const [settings, setSettings] = useState<Settings>(INITIAL_SETTINGS);

  const setWifi = (wifi: WifiSettings) => {
    setSettings((prevSettings) => ({
      ...prevSettings,
      wifi,
    }));
  };

  const setNtp = (ntp: NtpSettings) => {
    setSettings((prevSettings) => ({
      ...prevSettings,
      ntp,
    }));
  };

  const setMqtt = (mqtt: MqttSettings) => {
    setSettings((prevSettings) => ({
      ...prevSettings,
      mqtt,
    }));
  };
  const setPumpSchedule = (pumpSchedule: PumpSchedule) => {
    setSettings((prevSettings) => ({
      ...prevSettings,
      pumpSchedule,
    }));
  };
  return {
    ...settings,
    setWifi,
    setNtp,
    setMqtt,
    setPumpSchedule,
  };
};
