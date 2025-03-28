import { createContext } from 'preact';
import { useState } from 'preact/hooks';

const MQTT_PORT = 1883;

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
}

interface SettingsContextType extends Settings {
    setWifi: (wifi: WifiSettings) => void;
    setNtp: (ntp: NtpSettings) => void;
    setMqtt: (mqtt: MqttSettings) => void;
}

export const SettingsContext = createContext<SettingsContextType>({
    wifi: { ssid: '', password: '' },
    ntp: { server: '' },
    mqtt: { broker: '', port: MQTT_PORT, user: '', password: '', topic: '' },
    setWifi: () => {},
    setNtp: () => {},
    setMqtt: () => {},
});

const initialSettings: Settings = {
    wifi: { ssid: '', password: '' },
    ntp: { server: 'pool.ntp.org' },
    mqtt: { broker: '', port: MQTT_PORT, user: '', password: '', topic: '' },
};

// Create settings to be provided to the context.
export const createSettings = () => {
    const [settings, setSettings] = useState<Settings>(initialSettings);

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

    return {
        ...settings,
        setWifi,
        setNtp,
        setMqtt, // Return MQTT setter
    };
};
