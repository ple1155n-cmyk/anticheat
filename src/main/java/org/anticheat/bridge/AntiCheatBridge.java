package org.anticheat.bridge;

import org.anticheat.bridge.config.ConfigManager;
import org.anticheat.bridge.handler.ActionHandler;
import org.anticheat.bridge.listener.PacketListener;
import org.anticheat.bridge.network.EngineClient;
import org.anticheat.bridge.process.EngineLauncher;
import org.bukkit.Bukkit;
import org.bukkit.plugin.java.JavaPlugin;

import lombok.Getter;

@Getter
public class AntiCheatBridge extends JavaPlugin {

    private static AntiCheatBridge instance;
    private ConfigManager configManager;
    private EngineClient engineClient;
    private ActionHandler actionHandler;
    private PacketListener packetListener;
    private EngineLauncher engineLauncher;

    @Override
    public void onEnable() {
        instance = this;
        
        // 1. Initialize Configuration
        this.configManager = new ConfigManager(this);
        
        // 2. Initialize Action Handler (Executioner)
        this.actionHandler = new ActionHandler(this);
        
        // 3. Initialize Engine Launcher & Start Background Process
        this.engineLauncher = new EngineLauncher(this);
        this.engineLauncher.startEngine();
        
        // 4. Initialize Engine Client (TCP Socket) with a safety delay
        // We wait 40 ticks (2 seconds) to ensure the C++ server has finished its initialization
        Bukkit.getScheduler().runTaskLater(this, () -> {
            this.engineClient = new EngineClient(this);
            this.engineClient.connect();
            
            // 6. Register standard Movement Listener
            Bukkit.getPluginManager().registerEvents(new org.anticheat.bridge.listener.MovementListener(this), this);
            
            getLogger().info("AntiCheatBridge successfully linked to external engine.");
        }, 40L);
        
        getLogger().info("AntiCheatBridge startup initiated. External engine is launching...");
    }

    @Override
    public void onDisable() {
        // Graceful shutdown of socket connections
        if (engineClient != null) {
            engineClient.disconnect();
        }
        
        // Kill the background engine process
        if (engineLauncher != null) {
            engineLauncher.stopEngine();
        }
        
        getLogger().info("AntiCheatBridge has been disabled.");
    }

    public static AntiCheatBridge getInstance() {
        return instance;
    }
}
