package org.anticheat.bridge.listener;

import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Location;
import org.bukkit.entity.Player;
import org.bukkit.event.EventHandler;
import org.bukkit.event.Listener;
import org.bukkit.event.player.PlayerMoveEvent;
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
        Location from = event.getFrom();
        Location to = event.getTo();

        if (to == null) return;

        // Only process if the player actually moved (ignore rotation-only moves)
        if (from.getX() == to.getX() && from.getY() == to.getY() && from.getZ() == to.getZ()) {
            return;
        }

        // 1. Get Speed Potion Level
        int speedLevel = 0;
        PotionEffect speedEffect = player.getPotionEffect(PotionEffectType.SPEED);
        if (speedEffect != null) {
            speedLevel = speedEffect.getAmplifier() + 1;
        }

        // 2. Format: MOVE|<PlayerName>|<X>|<Y>|<Z>|<isOnGround>|<SpeedLevel>
        String packet = String.format("MOVE|%s|%.4f|%.4f|%.4f|%b|%d",
                player.getName(),
                to.getX(),
                to.getY(),
                to.getZ(),
                player.isOnGround(),
                speedLevel
        );

        // 3. Send to C++ Engine
        plugin.getEngineClient().sendRaw(packet);
    }
}
