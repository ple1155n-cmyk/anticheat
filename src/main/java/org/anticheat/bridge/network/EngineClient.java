package org.anticheat.bridge.network;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Bukkit;
import org.bukkit.entity.Player;
import net.kyori.adventure.text.Component;
import net.kyori.adventure.text.format.NamedTextColor;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class EngineClient {

    private final AntiCheatBridge plugin;
    private final Gson gson = new Gson();
    private final ExecutorService networkExecutor = Executors.newSingleThreadExecutor();
    
    private Socket socket;
    private PrintWriter out;
    private BufferedReader in;
    private boolean connected = false;

    public EngineClient(AntiCheatBridge plugin) {
        this.plugin = plugin;
    }

    public void connect() {
        networkExecutor.submit(() -> {
            String host = plugin.getConfigManager().getEngineHost();
            int port = plugin.getConfigManager().getEnginePort();
            int timeout = plugin.getConfigManager().getEngineTimeout();

            try {
                this.socket = new Socket();
                this.socket.connect(new InetSocketAddress(host, port), timeout);
                this.out = new PrintWriter(socket.getOutputStream(), true);
                this.in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                this.connected = true;
                
                plugin.getLogger().info("Connected to external engine at " + host + ":" + port);
                
                // Start listening for verdicts
                startListening();
                
            } catch (IOException e) {
                plugin.getLogger().severe("Failed to connect to anti-cheat engine: " + e.getMessage());
                // Simple reconnection logic could be added here
            }
        });
    }

    private void startListening() {
        new Thread(() -> {
            try {
                String line;
                while (connected && (line = in.readLine()) != null) {
                    processVerdict(line);
                }
            } catch (SocketException e) {
                if (connected) {
                    plugin.getLogger().warning("Socket connection lost: " + e.getMessage());
                }
            } catch (IOException e) {
                plugin.getLogger().severe("Error reading from engine: " + e.getMessage());
            } finally {
                disconnect();
            }
        }, "AntiCheat-VerdictListener").start();
    }

    private void processVerdict(String line) {
        if (line == null || line.isEmpty()) return;

        // Support for new delimited format: KICK|<PlayerName>|<Reason>
        if (line.startsWith("KICK|")) {
            String[] parts = line.split("\\|");
            if (parts.length >= 3) {
                String playerName = parts[1];
                String reason = parts[2];

                Bukkit.getScheduler().runTask(plugin, () -> {
                    Player player = Bukkit.getPlayer(playerName);
                    if (player != null && player.isOnline()) {
                        player.kick(Component.text(" [", NamedTextColor.DARK_GRAY)
                                .append(Component.text("AntiCheat", NamedTextColor.AQUA))
                                .append(Component.text("] ", NamedTextColor.DARK_GRAY))
                                .append(Component.text(reason, NamedTextColor.GRAY)));
                    }
                });
                return;
            }
        }

        // Support for legacy JSON format
        try {
            JsonObject verdict = gson.fromJson(line, JsonObject.class);
            if (verdict.has("player")) {
                Bukkit.getScheduler().runTask(plugin, () -> {
                    plugin.getActionHandler().handleVerdict(verdict);
                });
            }
        } catch (Exception e) {
            plugin.getLogger().warning("Received malformed verdict from engine: " + line);
        }
    }

    public void sendData(Object data) {
        if (!connected || out == null) return;
        
        networkExecutor.submit(() -> {
            try {
                String json = gson.toJson(data);
                out.println(json);
            } catch (Exception e) {
                plugin.getLogger().warning("Failed to send data to engine: " + e.getMessage());
            }
        });
    }

    public void sendRaw(String message) {
        if (!connected || out == null) return;
        
        networkExecutor.submit(() -> {
            try {
                out.println(message);
            } catch (Exception e) {
                plugin.getLogger().warning("Failed to send raw data to engine: " + e.getMessage());
            }
        });
    }

    public void disconnect() {
        connected = false;
        try {
            if (socket != null && !socket.isClosed()) {
                socket.close();
            }
            if (in != null) in.close();
            if (out != null) out.close();
        } catch (IOException e) {
            // Ignore closure errors
        }
        
        networkExecutor.shutdown();
        try {
            if (!networkExecutor.awaitTermination(3, TimeUnit.SECONDS)) {
                networkExecutor.shutdownNow();
            }
        } catch (InterruptedException e) {
            networkExecutor.shutdownNow();
        }
    }
}
