package org.anticheat.bridge.listener;

import com.comphenix.protocol.PacketType;
import com.comphenix.protocol.ProtocolLibrary;
import com.comphenix.protocol.events.ListenerPriority;
import com.comphenix.protocol.events.PacketAdapter;
import com.comphenix.protocol.events.PacketContainer;
import com.comphenix.protocol.events.PacketEvent;
import org.anticheat.bridge.AntiCheatBridge;
import org.anticheat.bridge.context.ContextGatherer;
import org.anticheat.bridge.network.MovementData;
import org.bukkit.Bukkit;
import org.bukkit.entity.Player;

import java.util.Map;

public class PacketListener {

    private final AntiCheatBridge plugin;
    private final ContextGatherer contextGatherer;

    public PacketListener(AntiCheatBridge plugin) {
        this.plugin = plugin;
        this.contextGatherer = new ContextGatherer(plugin);
    }

    public void register() {
        ProtocolLibrary.getProtocolManager().addPacketListener(new PacketAdapter(
                plugin,
                ListenerPriority.HIGHEST,
                PacketType.Play.Client.POSITION,
                PacketType.Play.Client.POSITION_LOOK,
                PacketType.Play.Client.LOOK
        ) {
            @Override
            public void onPacketReceiving(PacketEvent event) {
                handleMovementPacket(event);
            }
        });
    }

    private void handleMovementPacket(PacketEvent event) {
        Player player = event.getPlayer();
        PacketContainer packet = event.getPacket();

        // Extract raw movement data using ProtocolLib's PacketContainer
        // Ensuring compatibility across 1.20 and 1.21 using safe index reading
        double x = packet.getDoubles().readSafely(0);
        double y = packet.getDoubles().readSafely(1);
        double z = packet.getDoubles().readSafely(2);
        float yaw = packet.getFloat().readSafely(0);
        float pitch = packet.getFloat().readSafely(1);
        boolean onGround = packet.getBooleans().readSafely(0);

        // Movement context (async gathering)
        contextGatherer.gatherContext(player).thenAccept(context -> {
            // Update last safe location on the main thread
            Bukkit.getScheduler().runTask(plugin, () -> {
                plugin.getActionHandler().updateSafeLocation(player, player.getLocation());
            });

            // Aggregate data
            int ping = (int) context.getOrDefault("ping", 0);
            String blockBelow = (String) context.getOrDefault("blockBelow", "AIR");
            
            // Safe cast for potions map
            @SuppressWarnings("unchecked")
            Map<String, Integer> potions = (Map<String, Integer>) context.get("potions");

            // Build and send to engine
            MovementData data = MovementData.builder()
                    .player(player.getUniqueId().toString())
                    .action("MOVE")
                    .x(x)
                    .y(y)
                    .z(z)
                    .yaw(yaw)
                    .pitch(pitch)
                    .onGround(onGround)
                    .ping(ping)
                    .blockBelow(blockBelow)
                    .potions(potions)
                    .build();

            plugin.getEngineClient().sendData(data);
        });
    }
}
