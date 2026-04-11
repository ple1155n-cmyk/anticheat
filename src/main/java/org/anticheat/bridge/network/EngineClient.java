package org.anticheat.bridge.network;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import org.anticheat.bridge.AntiCheatBridge;
import org.bukkit.Bukkit;

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

    private void processVerdict(String json) {
        try {
            JsonObject verdict = gson.fromJson(json, JsonObject.class);
            if (verdict.has("player")) {
                // Route back to main thread for execution
                Bukkit.getScheduler().runTask(plugin, () -> {
                    plugin.getActionHandler().handleVerdict(verdict);
                });
            }
        } catch (Exception e) {
            plugin.getLogger().warning("Received malformed verdict from engine: " + json);
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
