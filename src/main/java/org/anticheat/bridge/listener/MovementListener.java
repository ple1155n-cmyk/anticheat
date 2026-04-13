package org.anticheat.bridge.listener;

import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Material;
import org.bukkit.Location;
import org.bukkit.entity.Player;
import org.bukkit.event.EventHandler;
import org.bukkit.event.Listener;
import org.bukkit.event.player.PlayerMoveEvent;
import org.bukkit.GameMode;
import org.bukkit.potion.PotionEffect;
import org.bukkit.potion.PotionEffectType;

public class MovementListener implements Listener {

    private final AntiCheatBridge plugin;

    public MovementListener(AntiCheatBridge plugin) {
        this.plugin = plugin;
    }

    @SuppressWarnings("deprecation")
    @EventHandler
    public void onMove(PlayerMoveEvent event) {
        Player player = event.getPlayer();

        // 1. Creative/Spectator Bypass
        if (player.getGameMode() == GameMode.CREATIVE || player.getGameMode() == GameMode.SPECTATOR) {
            return;
        }

        Location from = event.getFrom();
        Location to = event.getTo();

        if (to == null) return;

        // Only process if the player actually moved (ignore rotation-only moves)
        if (from.getX() == to.getX() && from.getY() == to.getY() && from.getZ() == to.getZ() && 
            from.getYaw() == to.getYaw() && from.getPitch() == to.getPitch()) {
            return;
        }

        // 2. Liquid Detection
        boolean inLiquid = player.getLocation().getBlock().isLiquid() || player.getEyeLocation().getBlock().isLiquid();

        // 3. Get Speed Potion Level
        int speedLevel = 0;
        PotionEffect speedEffect = player.getPotionEffect(PotionEffectType.SPEED);
        if (speedEffect != null) {
            speedLevel = speedEffect.getAmplifier() + 1;
        }

        // 4. Server-Side Elytra Validation
        boolean isGliding = player.isGliding();
        boolean hasElytra = player.getInventory().getChestplate() != null && 
                          player.getInventory().getChestplate().getType() == Material.ELYTRA;
        boolean validElytra = isGliding && hasElytra;

        // 5. Format: POS|Player|X|Y|Z|Yaw|Pitch|OnGround|validElytra|speedLevel|inLiquid
        String packet = String.format("POS|%s|%.4f|%.4f|%.4f|%.2f|%.2f|%b|%b|%d|%b",
                player.getName(),
                to.getX(),
                to.getY(),
                to.getZ(),
                to.getYaw(),
                to.getPitch(),
                player.isOnGround(),
                validElytra,
                speedLevel,
                inLiquid
        );

        // 6. Send to C++ Engine
        plugin.getEngineClient().sendRaw(packet);
    }
}
