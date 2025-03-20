import { createContext } from 'preact';
import { useState } from 'preact/hooks';

interface WifiSettings {
    ssid: string;
    password: string;
}

interface NtpSettings {
    ntpServer: string;
}

interface Settings {
    wifi: WifiSettings;
    ntp: NtpSettings;
}

interface SettingsContextType extends Settings {
    setWifi: (wifi: WifiSettings) => void;
    setNtp: (ntp: NtpSettings) => void;
}

export const SettingsContext = createContext<SettingsContextType | undefined>(undefined);

const initialSettings: Settings = {
    wifi: { ssid: '<ssid>', password: '' },
    ntp: { ntpServer: '' },
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
