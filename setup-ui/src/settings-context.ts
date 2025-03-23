import { createContext } from 'preact';
import { useState } from 'preact/hooks';

interface WifiSettings {
    ssid: string;
    password: string;
}

interface NtpSettings {
    server: string;
}

interface Settings {
    wifi: WifiSettings;
    ntp: NtpSettings;
}

interface SettingsContextType extends Settings {
    setWifi: (wifi: WifiSettings) => void;
    setNtp: (ntp: NtpSettings) => void;
}

export const SettingsContext = createContext<SettingsContextType>({
    wifi: { ssid: '', password: '' },
    ntp: { server: '' },
    setWifi: () => {},
    setNtp: () => {},
});

const initialSettings: Settings = {
    wifi: { ssid: '', password: '' },
    ntp: { server: 'pool.ntp.org' },
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

    return {
        ...settings,
        setWifi,
        setNtp,
    };
};
