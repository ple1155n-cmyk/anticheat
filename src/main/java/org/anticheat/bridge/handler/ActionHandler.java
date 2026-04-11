package org.anticheat.bridge.handler;

import com.google.gson.JsonObject;
import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.minimessage.MiniMessage;
import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Bukkit;
import org.bukkit.Location;
import org.bukkit.entity.Player;

import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

public class ActionHandler {

    private final AntiCheatBridge plugin;
    private final Map<UUID, Location> lastSafeLocations = new ConcurrentHashMap<>();
    private final MiniMessage miniMessage = MiniMessage.miniMessage();

    public ActionHandler(AntiCheatBridge plugin) {
        this.plugin = plugin;
    }

    /**
     * Updates the last known safe location for a player.
     * This should be called from the main thread or safely stored.
     */
    public void updateSafeLocation(Player player, Location location) {
        lastSafeLocations.put(player.getUniqueId(), location.clone());
    }

    /**
     * Handles an incoming verdict from the anti-cheat engine.
     * Guaranteed to be called on the main server thread.
     * 
     * verdict format: {"player": "UUID", "verdict": "FLAG", "type": "SPEED", "vl": 1.5, "cancelMove": true}
     */
    public void handleVerdict(JsonObject verdict) {
        String uuidStr = verdict.get("player").getAsString();
        UUID uuid = UUID.fromString(uuidStr);
        Player player = Bukkit.getPlayer(uuid);

        if (player == null || !player.isOnline()) return;

        String result = verdict.get("verdict").getAsString();
        if (!result.equalsIgnoreCase("FLAG")) return;

        String type = verdict.get("type").getAsString();
        double vl = verdict.get("vl").getAsDouble();
        boolean cancelMove = verdict.get("cancelMove").getAsBoolean();

        // 1. Cancel movement if required
        if (cancelMove && lastSafeLocations.containsKey(uuid)) {
            player.teleport(lastSafeLocations.get(uuid));
        }

        // 2. Alert administrators using Paper's Adventure API (MiniMessage)
        String alertRaw = plugin.getConfigManager().getAlertFormat()
                .replace("{player}", player.getName())
                .replace("{type}", type)
                .replace("{vl}", String.valueOf(vl));
        
        // Convert legacy color codes to MiniMessage if any, or just parse
        // For simplicity, we assume the config uses <color> tags or we pre-process legacy
        Component alertComponent = parseFormattedMessage(alertRaw);
        
        Bukkit.getConsoleSender().sendMessage(alertComponent);
        for (Player online : Bukkit.getOnlinePlayers()) {
            if (online.hasPermission(plugin.getConfigManager().getAlertPermission())) {
                online.sendMessage(alertComponent);
            }
        }
    }

    /**
     * Helper to handle both legacy codes (&) and MiniMessage tags.
     */
    private Component parseFormattedMessage(String message) {
        // Convert legacy & codes to MiniMessage tags or modern equivalents
        String modernized = message.replace("&8", "<dark_gray>")
                                .replace("&7", "<gray>")
                                .replace("&b", "<aqua>")
                                .replace("&f", "<white>")
                                .replace("&c", "<red>");
        
        return miniMessage.deserialize(modernized);
    }
}
