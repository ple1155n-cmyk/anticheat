package org.anticheat.bridge.context;

import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Bukkit;
import org.bukkit.Location;
import org.bukkit.Material;
import org.bukkit.entity.Player;
import org.bukkit.potion.PotionEffect;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

public class ContextGatherer {

    private final AntiCheatBridge plugin;

    public ContextGatherer(AntiCheatBridge plugin) {
        this.plugin = plugin;
    }

    /**
     * Gathers server-side context for a player safely.
     * 
     * @param player The player to gather context for.
     * @return A CompletableFuture containing the context map.
     */
    public CompletableFuture<Map<String, Object>> gatherContext(Player player) {
        CompletableFuture<Map<String, Object>> future = new CompletableFuture<>();
        
        // Ping is safe to read off-main
        int ping = player.getPing();
        
        // Potion effects
        Map<String, Integer> potionAmplifiers = new HashMap<>();
        for (PotionEffect effect : player.getActivePotionEffects()) {
            potionAmplifiers.put(effect.getType().getName(), effect.getAmplifier());
        }

        // Block retrieval MUST be on the main thread for Paper/Bukkit
        Bukkit.getScheduler().callSyncMethod(plugin, () -> {
            if (!player.isOnline()) {
                future.completeExceptionally(new Exception("Player went offline"));
                return null;
            }
            
            Location loc = player.getLocation();
            Material blockBelow = loc.clone().subtract(0, 0.1, 0).getBlock().getType();
            
            Map<String, Object> context = new HashMap<>();
            context.put("ping", ping);
            context.put("potions", potionAmplifiers);
            context.put("blockBelow", blockBelow.name());
            
            future.complete(context);
            return null;
        });

        return future;
    }
}
