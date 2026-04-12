package org.anticheat.bridge.listener;

import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Material;
import org.bukkit.entity.Player;
import org.bukkit.event.EventHandler;
import org.bukkit.event.Listener;
import org.bukkit.event.block.Action;
import org.bukkit.event.player.PlayerInteractEvent;
import org.bukkit.inventory.ItemStack;

public class FireworkListener implements Listener {

    private final AntiCheatBridge plugin;

    public FireworkListener(AntiCheatBridge plugin) {
        this.plugin = plugin;
    }

    @EventHandler
    public void onFireworkUse(PlayerInteractEvent event) {
        // 1. Logic: Only trigger on RIGHT_CLICK_AIR or RIGHT_CLICK_BLOCK
        if (event.getAction() != Action.RIGHT_CLICK_AIR && event.getAction() != Action.RIGHT_CLICK_BLOCK) {
            return;
        }

        Player player = event.getPlayer();
        
        // 2. Multi-hand Support: Get the item in the hand that clicked
        ItemStack item = event.getItem();
        if (item == null || item.getType() != Material.FIREWORK_ROCKET) {
            return;
        }

        // 3. State Check: Player must be gliding AND wearing an Elytra
        boolean isGliding = player.isGliding();
        boolean wearingElytra = player.getInventory().getChestplate() != null && 
                               player.getInventory().getChestplate().getType() == Material.ELYTRA;

        if (isGliding && wearingElytra) {
            // 4. Action: Send FIREWORK packet
            String packet = "FIREWORK|" + player.getName() + "\n";
            plugin.getEngineClient().sendRaw(packet);
        }
    }
}
