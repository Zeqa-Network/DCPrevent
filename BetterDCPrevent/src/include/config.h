#pragma once
#include "../pch.h"

class Config {
public:
    Config() {
        LoadConfiguration();
    }

    ~Config() {
        SaveConfiguration();
    }

    int GetLeftDebounceTime() const { return leftDebounceTime; }
    void SetLeftDebounceTime(int time) { leftDebounceTime = time; }

    int GetRightDebounceTime() const { return rightDebounceTime; }
    void SetRightDebounceTime(int time) { rightDebounceTime = time; }

    bool IsLinkDebounces() const { return linkDebounces; }
    void SetLinkDebounces(bool link) { linkDebounces = link; }

    bool IsHiddenToTray() const { return isHiddenToTray; }
    void SetHiddenToTray(bool hidden) { isHiddenToTray = hidden; }

	bool GetOnlyApplyToMinecraftWindow() const { return onlyApplyToMinecraftWindow; }
    void SetOnlyApplyToMinecraftWindow(bool hidden) { onlyApplyToMinecraftWindow = hidden; }

    int leftDebounceTime = 50;
    int rightDebounceTime = 50;
    bool linkDebounces = false;
    bool isHiddenToTray = false;
    bool onlyApplyToMinecraftWindow = true;

    void CreateConfigDirectory() {
        std::filesystem::path configPath = GetConfigPath();
        if (!std::filesystem::exists(configPath)) {
            std::filesystem::create_directories(configPath);
        }
    }

    void SaveConfiguration() {
        CreateConfigDirectory();
        std::filesystem::path configFilePath = GetConfigPath() / L"config.txt";
        
        std::wofstream configFile(configFilePath);
        if (configFile.is_open()) {
            configFile << leftDebounceTime << L'\n';
            configFile << rightDebounceTime << L'\n';
            configFile << (linkDebounces ? 1 : 0) << L'\n';
            configFile << (isHiddenToTray ? 1 : 0) << L'\n';
			configFile << (onlyApplyToMinecraftWindow ? 1 : 0) << L'\n';

            configFile.close();
        }
    }

    void LoadConfiguration() {
        CreateConfigDirectory();
        std::filesystem::path configFilePath = GetConfigPath() / L"config.txt";
        
        std::wifstream configFile(configFilePath);
        if (configFile.is_open()) {
            configFile >> leftDebounceTime;
            configFile >> rightDebounceTime;
            int linkDebouncesInt;
            int isHiddenToTrayInt;
			int onlyApplyToMinecraftWindowInt;
            configFile >> linkDebouncesInt;
            configFile >> isHiddenToTrayInt;
			configFile >> onlyApplyToMinecraftWindowInt;
            
            linkDebounces = (linkDebouncesInt != 0);
            isHiddenToTray = (isHiddenToTrayInt != 0);
			onlyApplyToMinecraftWindow = (onlyApplyToMinecraftWindowInt != 0);
            
            configFile.close();
        }
    }

    std::filesystem::path GetConfigPath() const {
        wchar_t appdataPath[MAX_PATH];
        if (GetEnvironmentVariableW(L"APPDATA", appdataPath, MAX_PATH)) {
            return std::filesystem::path(appdataPath) / L"BetterDCPrevent/Zeqa";
        } else {
            return L"";
        }
    }
};