package org.anticheat.bridge.config;

import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.configuration.file.FileConfiguration;

import lombok.Getter;

@Getter
public class ConfigManager {

    private final AntiCheatBridge plugin;
    
    private String engineHost;
    private int enginePort;
    private int engineTimeout;
    
    private boolean cancelMove;
    private String alertPermission;
    private String alertFormat;

    public ConfigManager(AntiCheatBridge plugin) {
        this.plugin = plugin;
        plugin.saveDefaultConfig();
        loadConfig();
    }

    public void loadConfig() {
        plugin.reloadConfig();
        FileConfiguration config = plugin.getConfig();

        this.engineHost = config.getString("engine.host", "127.0.0.1");
        this.enginePort = config.getInt("engine.port", 8080);
        this.engineTimeout = config.getInt("engine.timeout", 5000);

        this.cancelMove = config.getBoolean("punishments.cancel-move", true);
        this.alertPermission = config.getString("punishments.alert-permission", "anticheat.admin");
        this.alertFormat = config.getString("punishments.alert-format", "&8[&bAntiCheat&8] &7Player &f{player} &7flagged &b{type} &8(&fvl: {vl}&8)");
    }
}
